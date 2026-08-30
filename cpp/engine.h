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

// 两轮匹配：先在提取出的关键词里找（命中位置最早者优先），整段找不到再退回全文匹配
std::string translate(const std::string& input, const std::vector<Rule>& rules) {
    std::string target = extract_keywords(input);

    const Rule* best = nullptr;
    size_t bp = std::string::npos;
    for (const Rule& r : rules) {
        size_t end = 0;
        size_t p = rule_hit(r, target, end);
        if (p != std::string::npos && (bp == std::string::npos || p < bp)) {
            bp = p;
            best = &r;
        }
    }
    if (!best) {
        for (const Rule& r : rules) {
            size_t end = 0;
            if (rule_hit(r, input, end) != std::string::npos) {
                best = &r;
                break;
            }
        }
    }

    if (!best) return "{\"found\":false}";
    return "{\"found\":true,\"lang\":\"" + json_escape(best->lang) +
           "\",\"match\":\"" + json_escape(best->match) +
           "\",\"level\":\"" + json_escape(best->level) +
           "\",\"explain\":\"" + json_escape(best->explain) + "\"}";
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
