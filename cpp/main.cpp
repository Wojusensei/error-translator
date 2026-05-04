#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <windows.h>

struct Rule { std::string lang, match, level, explain; };

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
std::vector<Rule> load_rules(const std::string& path){
    std::vector<Rule> rules;
    std::ifstream f(path);
    if(!f.is_open()) return rules;
    std::string line;
    while(std::getline(f,line)){
        if(line.empty()||line[0]=='#') continue;
        size_t a=line.find('|'), b=line.find('|',a+1), c=line.find('|',b+1);
        if(a==std::string::npos||b==std::string::npos||c==std::string::npos) continue;
        Rule r;
        r.lang=line.substr(0,a); r.match=line.substr(a+1,b-a-1);
        r.level=line.substr(b+1,c-b-1); r.explain=line.substr(c+1);
        rules.push_back(r);
    }
    return rules;
}
void save_history(const std::deque<std::string>& h){
    std::ofstream f("history.txt"); for(auto& s:h) f<<s<<"\n----\n";
}

int main(){
    SetConsoleOutputCP(65001);
    auto rules = load_rules("data/errors.txt");
    if(rules.empty()) rules = load_rules("D:\\桌面\\error-translator\\data\\errors.txt");
    if(rules.empty()){ wprintln(L"[FAIL] Cannot read errors.txt qwq"); return 1; }

    wprintln(L"=======================================================");
    wprintln(L"    Error Translator (C++)  DA\x2606ZE");
    wprintln(L"=======================================================");
    wprintln(L"  Paste an error, get plain explanation.");
    wprintln(L"  Commands: 'quit' | 'history'");
    wprintln(L"");

    std::deque<std::string> history;
    std::string line;
    while(true){
        wprint(L"> "); std::getline(std::cin, line);
        if(line=="quit"||line=="exit") break;
        if(line=="history"){
            wprintln(L""); wprintln(L"  === History ===");
            for(auto& h:history) wprintln(L"  "+tow(h));
            wprintln(L"  ==============="); wprintln(L""); continue;
        }
        if(line.empty()) continue;
        history.push_back(line); if(history.size()>20) history.pop_front();
        save_history(history);

        const Rule* best = nullptr; size_t best_pos = std::string::npos;
        for(auto& r:rules){
            size_t p = line.find(r.match);
            if(p!=std::string::npos && (best_pos==std::string::npos || p<best_pos)){
                best_pos=p; best=&r;
            }
        }
        wprintln(L"");
        if(best){
            std::wstring emoji = L"🔴";
            if(best->level=="warning") emoji=L"🟡"; else if(best->level=="fatal") emoji=L"💀";
            wprintln(L"  "+emoji+L" Language: "+tow(best->lang));
            wprintln(L"  "+emoji+L" Level   : "+tow(best->level));
            wprintln(L"  ----------------------------------------");
            wprintln(L"  "+tow(best->explain));
            wprintln(L"  ----------------------------------------");
        }else{
            wprintln(L"  ❓ No match found. Try full error message, or help expand the library 一w一");
        }
        wprintln(L"");
    }
    wprintln(L"Bye DA\x2606ZE"); return 0;
}