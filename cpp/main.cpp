#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

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
    while(!w.empty()&&w.back()==L'\0') w.pop_back(); return w;
}

struct Rule { std::string lang, match, level, explain; };

std::vector<Rule> load_rules() {
    std::vector<Rule> rules;
    std::ifstream f("data/errors.txt");
    if(!f.is_open()) f.open("D:\\桌面\\error-translator\\data\\errors.txt");
    if(!f.is_open()) return rules;
    std::string line;
    while(std::getline(f,line)){
        if(line.empty()||line[0]=='#') continue;
        size_t a=line.find('|'), b=line.find('|',a+1), c=line.find('|',b+1);
        if(a==std::string::npos||b==std::string::npos||c==std::string::npos) continue;
        Rule r; r.lang=line.substr(0,a); r.match=line.substr(a+1,b-a-1);
        r.level=line.substr(b+1,c-b-1); r.explain=line.substr(c+1); rules.push_back(r);
    }
    return rules;
}

std::string translate(const std::string& input, const std::vector<Rule>& rules) {
    const Rule* best=nullptr; size_t bp=std::string::npos;
    for(auto& r:rules){ size_t p=input.find(r.match); if(p!=std::string::npos&&(bp==std::string::npos||p<bp)){bp=p; best=&r;} }
    if(!best) return "{\"found\":false}";
    std::string json="{\"found\":true,\"lang\":\""+best->lang+"\",\"match\":\""+best->match+"\",\"level\":\""+best->level+"\",\"explain\":\""+best->explain+"\"}";
    return json;
}

/* 简易URL解码 */
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

/* HTTP 服务器 */
void run_server(const std::vector<Rule>& rules) {
    /* 初始化 Winsock */
    WSADATA wsa; WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr={}; addr.sin_family=AF_INET; addr.sin_port=htons(8888); addr.sin_addr.s_addr=INADDR_ANY;
    bind(sock,(sockaddr*)&addr,sizeof(addr));
    listen(sock,SOMAXCONN);
    wprintln(L"  Server running at http://localhost:8888  DA\x2606ZE");
    wprintln(L"  Open your browser and go to that address.");
    wprintln(L"  Press Ctrl+C in this window to stop.");

    while(true){
        SOCKET client = accept(sock,NULL,NULL);
        char buf[8192]={0}; recv(client,buf,8192,0);
        std::string req(buf);

        /* 解析请求 */
        std::string response;
        if(req.find("GET /translate?q=")!=std::string::npos){
            size_t s=req.find("GET /translate?q=")+18;
            size_t e=req.find(" ",s);
            std::string query = url_decode(req.substr(s,e-s));
            std::string json = translate(query, rules);
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\n\r\n"+json;
        }else if(req.find("GET / ")!=std::string::npos||req.find("GET / HTTP")!=std::string::npos){
            /* 返回网页 */
            std::string html = read_file("js/index.html");
            if(html.empty()) html = read_file("D:\\桌面\\error-translator\\js\\index.html");
            if(html.empty()) html = "<h1>Cannot load index.html qwq</h1>";
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"+html;
        }else{
            response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404";
        }
        send(client,response.c_str(),response.size(),0);
        closesocket(client);
    }
    closesocket(sock);
    WSACleanup();
}

int main(){
    SetConsoleOutputCP(65001);
    auto rules = load_rules();
    if(rules.empty()){ wprintln(L"[FAIL] Cannot read errors.txt qwq"); return 1; }
    wprintln(L"=======================================================");
    wprintln(L"    Error Translator Server (C++)  DA\x2606ZE");
    wprintln(L"=======================================================");
    wprintln(L"");
    run_server(rules);
    return 0;
}