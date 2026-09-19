// error-translator 的匹配引擎（平台无关部分）。
// 被 main.cpp 的服务器和 engine_test.cpp 的测试共用。
//
// data/errors.txt 的行格式：
//   语言|别名1||别名2||...||等级|解释
// 别名之间用 || 分隔，等级和解释用单个 | 分隔。别名里支持 ".*" 通配符
// （匹配任意字符串），例如 "name .* is not defined"。
#ifndef ERROR_TRANSLATOR_ENGINE_H
#define ERROR_TRANSLATOR_ENGINE_H

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Rule {
    std::string lang, match, level, explain, category;
};

std::string json_escape(const std::string& s);

std::vector<Rule> load_rules_from(const std::string& path) {
    std::vector<Rule> rules;
    std::ifstream f(path);
    if (!f.is_open()) return rules;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // 行格式：语言|别名1||别名2|...||等级|解释|类别（五字段）。
        // 从右往左依次取类别、解释、等级，语言和首个 | 之间剩下的全是匹配别名。
        size_t p1 = line.find('|');
        size_t b3 = line.rfind('|');            // 解释|类别
        if (p1 == std::string::npos || b3 == std::string::npos || b3 <= p1 + 1) continue;
        size_t b2 = line.rfind('|', b3 - 1);    // 等级|解释
        if (b2 == std::string::npos || b2 <= p1) continue;
        size_t b1 = line.rfind('|', b2 - 1);    // 别名|等级
        if (b1 == std::string::npos || b1 <= p1) continue;
        Rule r;
        r.lang = line.substr(0, p1);
        r.match = line.substr(p1 + 1, b1 - p1 - 1);
        r.level = line.substr(b1 + 1, b2 - b1 - 1);
        r.explain = line.substr(b2 + 1, b3 - b2 - 1);
        r.category = line.substr(b3 + 1);
        if (r.match.empty() || r.explain.empty() || r.category.empty()) continue;
        rules.push_back(r);
    }
    return rules;
}

std::vector<Rule> load_rules() {
    std::vector<Rule> rules = load_rules_from("data/errors.txt");
    if (rules.empty()) rules = load_rules_from("D:\\桌面\\error-translator\\data\\errors.txt");
    return rules;
}

// Python Traceback 里的出错位置（最后一个 File "x", line N）
struct SourceRef {
    bool ok = false;
    std::string file;
    int line = 0;
};

SourceRef find_python_source(const std::string& input) {
    SourceRef s;
    if (input.find("Traceback (most recent call last)") == std::string::npos) return s;
    size_t from = 0;
    while (true) {
        size_t f = input.find("File \"", from);
        if (f == std::string::npos) break;
        size_t name_start = f + 6;
        size_t name_end = input.find('"', name_start);
        if (name_end == std::string::npos) break;
        if (input.compare(name_end, 8, "\", line ") == 0) {
            size_t digits = name_end + 8;
            int line = 0;
            while (digits < input.size() && input[digits] >= '0' && input[digits] <= '9') {
                line = line * 10 + (input[digits] - '0');
                digits++;
            }
            if (line > 0) {
                s.ok = true;
                s.file = input.substr(name_start, name_end - name_start);
                s.line = line;
            }
            from = digits;
        } else {
            from = name_end;
        }
    }
    return s;
}

// JS/V8 堆栈里的出错位置（最后一个 at 框架，形如 app.js:42:17）
SourceRef find_js_source(const std::string& input) {
    SourceRef s;
    static const char* exts[] = {".js", ".mjs", ".cjs", ".ts", ".tsx", ".jsx"};
    size_t from = 0;
    while (true) {
        size_t at = input.find("\n    at ", from);
        if (at == std::string::npos) break;
        from = at + 8;
        size_t line_end = input.find('\n', from);
        if (line_end == std::string::npos) line_end = input.size();
        std::string frame = input.substr(from, line_end - from);
        // "fn (app.js:42:17)" 或 "app.js:42:17"
        size_t open = frame.rfind('(');
        size_t close = frame.find(')', open == std::string::npos ? 0 : open);
        std::string loc = (open != std::string::npos && close != std::string::npos)
            ? frame.substr(open + 1, close - open - 1) : frame;
        size_t colon2 = loc.rfind(':');
        if (colon2 == std::string::npos || colon2 == 0) continue;
        size_t colon1 = loc.rfind(':', colon2 - 1);
        if (colon1 == std::string::npos) continue;
        std::string line_str = loc.substr(colon1 + 1, colon2 - colon1 - 1);
        if (line_str.empty() || line_str.find_first_not_of("0123456789") != std::string::npos) continue;
        std::string file = loc.substr(0, colon1);
        bool has_ext = false;
        for (const char* e : exts) {
            size_t n = strlen(e);
            if (file.size() >= n && file.compare(file.size() - n, n, e) == 0) { has_ext = true; break; }
        }
        if (has_ext) {
            s.ok = true;
            s.file = file;
            s.line = std::atoi(line_str.c_str());
        }
    }
    return s;
}

// 从一大段报错里提取第一行关键词：优先取 ": " 之后的正文、去掉 "[xx]" 后缀
std::string extract_keywords(const std::string& input) {
    // Python Traceback：真正要解释的异常在最后一行，拿它当匹配目标
    if (input.find("Traceback (most recent call last)") != std::string::npos) {
        size_t last_char = input.find_last_not_of(" \t\r\n");
        if (last_char != std::string::npos) {
            size_t begin = input.rfind('\n', last_char);
            std::string last_line = (begin == std::string::npos)
                ? input.substr(0, last_char + 1)
                : input.substr(begin + 1, last_char - begin);
            if (!last_line.empty()) return last_line;
        }
    }
    std::string result = input;

    size_t colon = result.find(": ");
    if (colon != std::string::npos) {
        result = result.substr(colon + 2);
    }

    size_t bracket = result.find("[");
    if (bracket != std::string::npos) {
        result = result.substr(0, bracket);
        while (!result.empty() && result.back() == ' ') result.pop_back();
    }

    if (result.empty()) result = input;

    size_t nl = result.find('\n');
    if (nl != std::string::npos) result = result.substr(0, nl);

    return result;
}

// 单个别名在文本里找位置，".*" 代表中间隔任意字符
bool find_token(const std::string& text, const std::string& token, size_t& start, size_t& end) {
    size_t star = token.find(".*");
    if (star != std::string::npos) {
        std::string prefix = token.substr(0, star);
        std::string suffix = token.substr(star + 2);
        size_t p = text.find(prefix);
        if (p == std::string::npos) return false;
        size_t q = text.find(suffix, p + prefix.size());
        if (q == std::string::npos) return false;
        start = p;
        end = q + suffix.size();
        return true;
    }
    size_t p = text.find(token);
    if (p == std::string::npos) return false;
    start = p;
    end = p + token.size();
    return true;
}

// 语言自动识别：按结构性特征判断报错属于哪种语言，判不出返回空串（回退全库匹配）。
// 判出的语言名必须与 errors.txt 里的语言字段一致。
std::string detect_language(const std::string& input) {
    if (input.find("Traceback (most recent call last)") != std::string::npos) return "Python";
    if (input.find("error[E") != std::string::npos) return "Rust";
    if (input.find("error TS") != std::string::npos) return "TypeScript";
    if (input.find("error CS") != std::string::npos || input.find("System.") != std::string::npos) return "C#";
    if (input.find(".kt:") != std::string::npos ||
        input.find("Unresolved reference") != std::string::npos ||
        input.find("lateinit") != std::string::npos) return "Kotlin";
    if (input.find("MissingMethodException") != std::string::npos ||
        input.find("No such property") != std::string::npos ||
        input.find("Cannot get property") != std::string::npos) return "Groovy";
    if (input.find("java.lang.") != std::string::npos ||
        input.find("Exception in thread \"main\"") != std::string::npos ||
        input.find("Caused by: ") != std::string::npos) return "Java";
    if (input.find("panic:") != std::string::npos || input.find("goroutine ") != std::string::npos) return "Go";
    if (input.find("Fatal error: Uncaught") != std::string::npos) return "PHP";
    if (input.find("Run-time error '") != std::string::npos) return "VBA";
    if (input.find("SQLSTATE[") != std::string::npos || input.find("ORA-") != std::string::npos) return "SQL";
    if (input.find("RenderFlex") != std::string::npos ||
        input.find("Null check operator") != std::string::npos) return "Dart/Flutter";
    if (input.find("nginx: [emerg]") != std::string::npos ||
        input.find("nginx -t") != std::string::npos) return "Nginx";
    if (input.find("missing separator") != std::string::npos ||
        input.find("CMake Error") != std::string::npos ||
        input.find("No rule to make target") != std::string::npos) return "Make/CMake";
    if (input.find("CrashLoopBackOff") != std::string::npos || input.find("ImagePullBackOff") != std::string::npos ||
        input.find("kubectl") != std::string::npos) return "Kubernetes";
    if (input.find("Module build failed") != std::string::npos ||
        input.find("You may need an appropriate loader") != std::string::npos) return "前端构建";
    if (input.find("npm ERR!") != std::string::npos || input.find("ERR_PNPM_") != std::string::npos ||
        input.find("yarn install v") != std::string::npos) return "包管理器";
    if (input.find("Process completed with exit code") != std::string::npos ||
        input.find("The workflow is not valid") != std::string::npos) return "CI/CD";
    if (input.find("HTTP/1.") != std::string::npos || input.find("HTTP 4") != std::string::npos ||
        input.find("HTTP 5") != std::string::npos) return "HTTP";
    if (input.find("zsh:") != std::string::npos ||
        input.find("bash: line") != std::string::npos ||
        input.find("sh: ") != std::string::npos) return "Shell";
    if (input.find("\n    at ") != std::string::npos) {
        if (input.find(".ts") != std::string::npos || input.find(".tsx") != std::string::npos) return "TypeScript";
        if (input.find(".js") != std::string::npos || input.find(".mjs") != std::string::npos ||
            input.find("node:") != std::string::npos) return "JavaScript";
    }
    return "";
}

// 遍历一条规则的所有别名，返回命中的最早位置，没命中返回 npos
size_t rule_hit(const Rule& r, const std::string& text, size_t& end_out) {
    const std::string& matches = r.match;
    size_t best = std::string::npos;
    size_t pos = 0;
    while (pos < matches.size()) {
        size_t next = matches.find("||", pos);
        std::string token = (next == std::string::npos) ? matches.substr(pos) : matches.substr(pos, next - pos);
        size_t start = 0, end = 0;
        if (!token.empty() && find_token(text, token, start, end)) {
            if (best == std::string::npos || start < best) {
                best = start;
                end_out = end;
            }
        }
        if (next == std::string::npos) break;
        pos = next + 2;
    }
    return best;
}

// 两轮匹配核心：先在提取出的关键词里找（命中位置最早者优先），整段找不到再退回全文匹配。
// 输入是规则子集，全量匹配时传全部规则。
const Rule* match_in(const std::string& input, const std::string& target, const std::vector<const Rule*>& subset) {
    const Rule* best = nullptr;
    size_t bp = std::string::npos;
    for (const Rule* r : subset) {
        size_t end = 0;
        size_t p = rule_hit(*r, target, end);
        if (p != std::string::npos && (bp == std::string::npos || p < bp)) {
            bp = p;
            best = r;
        }
    }
    if (!best) {
        for (const Rule* r : subset) {
            size_t end = 0;
            if (rule_hit(*r, input, end) != std::string::npos) {
                best = r;
                break;
            }
        }
    }
    return best;
}

// 在规则子集里收集所有命中并排序（按关键词最早位置，其次规则顺序），返回前 top_n 个。
// 空集时与 match_in 的两轮语义一致：第一轮无命中则按第二轮顺序取前 top_n。
std::vector<const Rule*> rank_matches(const std::string& input, const std::string& target,
                                      const std::vector<const Rule*>& subset, size_t top_n) {
    std::vector<std::pair<size_t, const Rule*>> hits;
    for (const Rule* r : subset) {
        size_t end = 0;
        size_t p = rule_hit(*r, target, end);
        if (p != std::string::npos) hits.push_back({p, r});
    }
    std::vector<const Rule*> ranked;
    if (!hits.empty()) {
        std::sort(hits.begin(), hits.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        for (auto& h : hits) ranked.push_back(h.second);
    } else {
        for (const Rule* r : subset) {
            size_t end = 0;
            if (rule_hit(*r, input, end) != std::string::npos) ranked.push_back(r);
        }
    }
    if (ranked.size() > top_n) ranked.resize(top_n);
    return ranked;
}

std::string rule_json_fields(const Rule* r) {
    return "\"lang\":\"" + json_escape(r->lang) +
           "\",\"match\":\"" + json_escape(r->match) +
           "\",\"level\":\"" + json_escape(r->level) +
           "\",\"explain\":\"" + json_escape(r->explain) +
           "\",\"category\":\"" + json_escape(r->category) + "\"";
}

// 取排序后的候选：识别到语言时优先在该语言的规则里排，无命中再全库排
std::vector<const Rule*> pick_candidates(const std::string& input, const std::string& target,
                                         const std::vector<const Rule*>& all) {
    std::string lang = detect_language(input);
    if (!lang.empty()) {
        std::vector<const Rule*> subset;
        for (const Rule* r : all) {
            if (r->lang == lang) subset.push_back(r);
        }
        if (!subset.empty()) {
            std::vector<const Rule*> ranked = rank_matches(input, target, subset, 3);
            if (!ranked.empty()) return ranked;
        }
    }
    return rank_matches(input, target, all, 3);
}

// 主结果语义与旧版完全一致（候选第一名），另附最多两个 alternatives
std::string translate(const std::string& input, const std::vector<Rule>& rules) {
    std::string target = extract_keywords(input);
    std::vector<const Rule*> all;
    all.reserve(rules.size());
    for (const Rule& r : rules) all.push_back(&r);

    std::vector<const Rule*> ranked = pick_candidates(input, target, all);
    if (ranked.empty()) return "{\"found\":false}";

    std::string out = "{\"found\":true," + rule_json_fields(ranked[0]);
    SourceRef src = find_python_source(input);
    if (!src.ok) src = find_js_source(input);
    if (src.ok) {
        out += ",\"source\":{\"file\":\"" + json_escape(src.file) +
               "\",\"line\":" + std::to_string(src.line) + "}";
    }
    if (ranked.size() > 1) {
        out += ",\"alternatives\":[";
        for (size_t i = 1; i < ranked.size(); i++) {
            if (i > 1) out += ",";
            out += "{" + rule_json_fields(ranked[i]) + "}";
        }
        out += "]";
    }
    return out + "}";
}

// 拼 JSON 响应前必须转义，否则解释里带引号/反斜杠时前端会解析失败
std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c >= 0x20) out += (char)c;  // 其余控制字符丢弃
        }
    }
    return out;
}

#endif
