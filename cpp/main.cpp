#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

#include "engine.h"

// 只监听本机回环地址：这个服务没有任何鉴权，暴露到局域网会被任意调用
static const char* kBindAddress = "127.0.0.1";
static const int kPort = 8888;
// 客户端连上却不发数据会卡死单线程 accept 循环，读请求必须带超时
static const int kRecvTimeoutMs = 5000;
static const size_t kMaxRequestBytes = 64 * 1024;

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";
    f.seekg(0, std::ios::end); size_t n = f.tellg(); f.seekg(0);
    std::string s(n,'\0'); f.read(&s[0], n); return s;
}
void wprint(const std::wstring& s){ DWORD n; WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),s.c_str(),s.length(),&n,NULL); }
void wprintln(const std::wstring& s){ wprint(s); wprint(L"\n"); }
std::wstring tow(const std::string& s){
    if(s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,NULL,0);
    std::wstring w(len,L'\0'); MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&w[0],len);
    while(!w.empty()&&w.back()==L'\0') w.pop_back();
    return w;
}
void log_line(const std::string& s){ wprintln(tow(s)); }

std::string url_decode(const std::string& s){
    std::string r;
    for(size_t i=0;i<s.size();i++){
        if(s[i]=='%'&&i+2<s.size()){
            int v; std::string hex=s.substr(i+1,2);
            std::stringstream ss; ss<<std::hex<<hex; ss>>v;
            r+=(char)v; i+=2;
        }else if(s[i]=='+') r+=' ';
        else r+=s[i];
    }
    return r;
}

// 静态文件路由：URL 是写死的常量表，不接受用户传入的路径，不存在目录穿越问题
struct FileRoute { const char* url; const char* file; const char* type; };
static const FileRoute kFileRoutes[] = {
    {"/",        "js/index.html", "text/html; charset=utf-8"},
    {"/app.js",  "js/app.js",     "text/javascript; charset=utf-8"},
    {"/app.css", "js/app.css",    "text/css; charset=utf-8"},
};

// 取请求行里的目标（路径+查询串），解析失败返回空串
std::string request_target(const std::string& req) {
    size_t sp1 = req.find(' ');
    if (sp1 == std::string::npos) return "";
    size_t sp2 = req.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return "";
    return req.substr(sp1 + 1, sp2 - sp1 - 1);
}

// 读一个请求：带超时与大小上限，读到完整头部（或对端断开）为止
bool read_request(SOCKET client, std::string& out) {
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&kRecvTimeoutMs, sizeof(kRecvTimeoutMs));
    char buf[4096];
    while (out.size() < kMaxRequestBytes) {
        int n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0) break;
        out.append(buf, (size_t)n);
        if (out.find("\r\n\r\n") != std::string::npos) break;
    }
    return !out.empty();
}

// 统一拼响应：Content-Length + Connection: close，客户端能正确判断响应结束
std::string make_response(const std::string& type, const std::string& body) {
    std::ostringstream head;
    head << "HTTP/1.1 200 OK\r\n"
         << "Content-Type: " << type << "\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Access-Control-Allow-Origin: *\r\n"
         << "Connection: close\r\n\r\n";
    return head.str() + body;
}

// 从 JSON 响应里抠出 "lang" 字段，日志用
std::string lang_of(const std::string& json) {
    size_t p = json.find("\"lang\":\"");
    if (p == std::string::npos) return "-";
    size_t s = p + 8, e = json.find('"', s);
    return e == std::string::npos ? "-" : json.substr(s, e - s);
}

std::string now_hms() {
    time_t t = time(NULL);
    tm tmv{};
    localtime_s(&tmv, &t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return buf;
}

bool run_server(const std::vector<Rule>& rules) {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kPort);
    addr.sin_addr.s_addr = inet_addr(kBindAddress);
    if (addr.sin_addr.s_addr == INADDR_NONE || bind(sock,(sockaddr*)&addr,sizeof(addr)) != 0) {
        log_line("[FAIL] 绑定 " + std::string(kBindAddress) + ":" + std::to_string(kPort) +
                 " 失败，端口可能被占用 qwq");
        return false;
    }
    listen(sock,SOMAXCONN);
    log_line("=======================================================");
    log_line("    Error Translator Server (C++)  DA☆ZE");
    log_line("    loaded " + std::to_string(rules.size()) + " rules from data/errors.txt");
    log_line("    listening at http://127.0.0.1:8888 （仅本机可访问）");
    log_line("=======================================================");
    log_line("");

    while(true){
        SOCKET client = accept(sock,NULL,NULL);
        if (client == INVALID_SOCKET) continue;
        auto t0 = std::chrono::steady_clock::now();

        std::string req;
        std::string response;
        std::string what = "-";
        if (read_request(client, req)) {
            std::string target = request_target(req);
            std::string path = target.substr(0, target.find('?'));
            if (target.rfind("/translate?q=", 0) == 0) {
                std::string query = url_decode(target.substr(13));
                std::string json = translate(query, rules);
                response = make_response("application/json; charset=utf-8", json);
                what = "/translate -> " + lang_of(json);
            } else {
                for (const FileRoute& r : kFileRoutes) {
                    if (path != r.url) continue;
                    std::string body = read_file(r.file);
                    if (body.empty() && std::string(r.file) == "js/index.html")
                        body = read_file("D:\\桌面\\error-translator\\js\\index.html");
                    if (!body.empty()) {
                        response = make_response(r.type, body);
                        what = path;
                    }
                    break;
                }
                if (response.empty())
                    response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n"
                               "Content-Length: 3\r\nConnection: close\r\n\r\n404";
            }
        } else {
            response = "HTTP/1.1 408 Request Timeout\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            what = "(客户端无数据)";
        }

        send(client,response.c_str(),response.size(),0);
        closesocket(client);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
        log_line("[" + now_hms() + "] " + what + " (" + std::to_string(ms) + "ms)");
    }
    closesocket(sock);
    WSACleanup();
    return true;
}

int main(){
    SetConsoleOutputCP(65001);
    auto rules = load_rules();
    if(rules.empty()){ wprintln(L"[FAIL] Cannot read errors.txt qwq"); return 1; }
    if (!run_server(rules)) return 1;
    return 0;
}
