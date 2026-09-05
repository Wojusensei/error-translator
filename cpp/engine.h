// error-translator 的匹配引擎（平台无关部分）。
// 被 main.cpp 的服务器和 engine_test.cpp 的测试共用。
//
// data/errors.txt 的行格式：
//   语言|别名1||别名2||...||等级|解释
// 别名之间用 || 分隔，等级和解释用单个 | 分隔。别名里支持 ".*" 通配符
// （匹配任意字符串），例如 "name .* is not defined"。
#ifndef ERROR_TRANSLATOR_ENGINE_H
#define ERROR_TRANSLATOR_ENGINE_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Rule {
    std::string lang, match, level, explain;
};

std::string json_escape(const std::string& s);

std::vector<Rule> load_rules_from(const std::string& path) {
    std::vector<Rule> rules;
    std::ifstream f(path);
    if (!f.is_open()) return rules;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // 从右往左取解释和等级，语言和首个 | 之间剩下的全是匹配别名。
        // 不能只按前三个 | 切分：别名本身用 || 分隔，会把字段切错。
        size_t p1 = line.find('|');
        size_t last = line.rfind('|');
        if (p1 == std::string::npos || last == std::string::npos || last <= p1 + 1) continue;
        size_t prev = line.rfind('|', last - 1);
        if (prev == std::string::npos || prev <= p1) continue;
        Rule r;
        r.lang = line.substr(0, p1);
        r.match = line.substr(p1 + 1, prev - p1 - 1);
        r.level = line.substr(prev + 1, last - prev - 1);
        r.explain = line.substr(last + 1);
        if (r.match.empty() || r.explain.empty()) continue;
        rules.push_back(r);
    }
    return rules;
}

std::vector<Rule> load_rules() {
    std::vector<Rule> rules = load_rules_from("data/errors.txt");
    if (rules.empty()) rules = load_rules_from("D:\\桌面\\error-translator\\data\\errors.txt");
    return rules;
}

// 从一大段报错里提取第一行关键词：优先取 ": " 之后的正文、去掉 "[xx]" 后缀
std::string extract_keywords(const std::string& input) {
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

std::string rule_to_json(const Rule* r) {
    return "{\"found\":true,\"lang\":\"" + json_escape(r->lang) +
           "\",\"match\":\"" + json_escape(r->match) +
           "\",\"level\":\"" + json_escape(r->level) +
           "\",\"explain\":\"" + json_escape(r->explain) + "\"}";
}

std::string translate(const std::string& input, const std::vector<Rule>& rules) {
    std::string target = extract_keywords(input);

    std::vector<const Rule*> all;
    all.reserve(rules.size());
    for (const Rule& r : rules) all.push_back(&r);

    // 先按识别出的语言在子集内匹配，命中才用；否则回退全库，行为与旧版一致
    std::string lang = detect_language(input);
    if (!lang.empty()) {
        std::vector<const Rule*> subset;
        for (const Rule* r : all) {
            if (r->lang == lang) subset.push_back(r);
        }
        if (!subset.empty()) {
            const Rule* scoped = match_in(input, target, subset);
            if (scoped) return rule_to_json(scoped);
        }
    }

    const Rule* best = match_in(input, target, all);
    if (!best) return "{\"found\":false}";
    return rule_to_json(best);
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
