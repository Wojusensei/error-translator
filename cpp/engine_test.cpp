// 匹配引擎的回归测试，跨平台可用（不需要 Windows）：
//   g++ -o engine_test cpp/engine_test.cpp && ./engine_test data/errors.txt
#include <iostream>
#include "engine.h"

static int g_fail = 0;

void expect(bool ok, const std::string& msg) {
    if (!ok) {
        std::cerr << "  [FAIL] " << msg << "\n";
        g_fail++;
    }
}

// 简化断言：translate 结果里包含 "lang":"xxx"
bool lang_is(const std::string& json, const std::string& lang) {
    return json.find("\"lang\":\"" + lang + "\"") != std::string::npos;
}

int main(int argc, char** argv) {
    std::string path = (argc > 1) ? argv[1] : "data/errors.txt";
    std::vector<Rule> rules = load_rules_from(path);
    std::cout << "loaded " << rules.size() << " rules from " << path << "\n";
    expect(rules.size() > 1000, "should load over 1000 rules");

    // 数据完整性：等级合法、解释非空、解释里不能有 || 和裸 |（会破坏格式）
    for (const Rule& r : rules) {
        expect(r.level == "error" || r.level == "warning" || r.level == "fatal",
               "bad level in [" + r.lang + "] " + r.match);
        expect(!r.explain.empty() && r.explain.size() > 8,
               "empty explain in [" + r.lang + "] " + r.match);
        expect(r.explain.find("||") == std::string::npos && r.explain.find('|') == std::string::npos,
               "explain contains pipe in [" + r.lang + "] " + r.match);
        expect(r.match.find("||") != std::string::npos || !r.match.empty(), "match empty");
        for (const char c : r.match) {
            expect(c != '\n' && c != '\r', "newline inside match");
        }
    }

    // 匹配回归：输入 -> 期望语言
    const char* tests[][2] = {
        {"Traceback (most recent call last):\n  File \"app.py\", line 3\nNameError: name 'x' is not defined", "Python"},
        {"TypeError: unsupported operand type(s) for +: 'int' and 'str'", "Python"},
        {"json.decoder.JSONDecodeError: Expecting value: line 1 column 1 (char 0)", "Python"},
        {"ERROR: Could not find a version that satisfies the requirement", "Python"},
        {"TypeError: Cannot read properties of undefined (reading 'map')\n    at render (app.js:42)", "JavaScript"},
        {"Uncaught ReferenceError: Cannot access 'user' before initialization", "JavaScript"},
        {"npm ERR! ERESOLVE unable to resolve dependency tree", "JavaScript"},
        {"java.lang.NullPointerException: Cannot invoke \"String.length()\" because \"s\" is null", "Java"},
        {"error: variable x might not have been initialized", "Java"},
        {"/usr/bin/ld: undefined reference to `foo()'", "C++"},
        {"terminate called after throwing an instance of 'std::out_of_range'", "C++"},
        {"main.c:12:5: warning: implicit declaration of function 'sleep'", "C"},
        {"fatal error: stdio.h: No such file or directory\ncompilation terminated.", "C"},
        {"./main.go:8:2: declared and not used: x", "Go"},
        {"panic: runtime error: invalid memory address or nil pointer dereference", "Go"},
        {"error CS1061: 'User' does not contain a definition for 'Name'", "C#"},
        {"NoMethodError: undefined method `save' for nil:NilClass", "Ruby"},
        {"Fatal error: Uncaught Error: Call to a member function query() on null", "PHP"},
        {"error[E0382]: borrow of moved value: `buffer`", "Rust"},
        {"error[E0308]: mismatched types\n --> src/main.rs:11:5", "Rust"},
        {"error TS2339: Property 'name' does not exist on type 'User'.", "TypeScript"},
        {"e: file.kt:10:5 Unresolved reference: onClick", "Kotlin"},
        {"java.lang.IllegalStateException: lateinit property adapter has not been initialized", "Kotlin"},
        {"Fatal error: Unexpectedly found nil while unwrapping an Optional value", "Swift"},
        {"Null check operator used on a null value", "Dart/Flutter"},
        {"A RenderFlex overflowed by 42 pixels on the bottom.", "Dart/Flutter"},
        {"ERROR 1064 (42000): You have an error in your SQL syntax near 'SELCT'", "SQL"},
        {"ERROR: duplicate key value violates unique constraint \"users_email_key\"", "SQL"},
        {"./deploy.sh: line 12: syntax error near unexpected token `fi'", "Shell"},
        {"curl: (7) Failed to connect to localhost port 8080: Connection refused", "Shell"},
        {"ls: cannot access '/data': No such file or directory", "Shell"},
        {"zsh: no matches found: [abc]", "Shell"},
        {"NameError", "Python"},
        {"ReferenceError: x is not defined", "JavaScript"},
        {"Error: object 'df' not found", "R"},
        {"Error in x == y : missing value where TRUE/FALSE needed", "R"},
        {"Error in library(tidyverse) : there is no package called 'tidyverse'", "R"},
        {"Error: could not find function \"group_by\"", "R"},
        {"type mismatch; found: String, required: Int", "Scala"},
        {"error: not found: value sqlContext", "Scala"},
        {"scala.MatchError: List() (of class scala.collection.immutable.Nil$)", "Scala"},
        {"org.apache.spark.SparkException: Task not serializable", "Scala"},
        {"lua: a.lua:12: attempt to index a nil value (global 'config')", "Lua"},
        {"lua: a.lua:15: attempt to call a nil value (field 'save')", "Lua"},
        {"Can't call method \"get\" on an undefined value at script.pl line 12.", "Perl"},
        {"Can't locate DBI.pm in @INC (you may need to install the DBI module)", "Perl"},
        {"Global symbol \"$coutn\" requires explicit package name at a.pl line 5.", "Perl"},
        {"-[UIView setFoo:]: unrecognized selector sent to instance 0x0", "Objective-C"},
        {"*** Terminating app due to uncaught exception 'NSInvalidArgumentException', reason: '-[Foo bar]'", "Objective-C"},
        {"MethodError: no method matching +(::String, ::Int64)", "Julia"},
        {"UndefVarError: df not defined", "Julia"},
        {"groovy.lang.MissingMethodException: No signature of method: Script.foo()", "Groovy"},
        {"java.lang.IllegalStateException: Cannot get property 'name' on null object", "Groovy"},
        {"** (MatchError) no match of right hand side value: :error", "Elixir"},
        {"** (KeyError) key :name not found in: %{}", "Elixir"},
        {"ImproperlyConfigured: The SECRET_KEY setting must not be empty.", "Python"},
        {"fixture 'client' not found\n> available fixtures: cache", "Python"},
        {"Module not found: Error: Can't resolve './config' in '/app/src'", "JavaScript"},
        {"***************************\nAPPLICATION FAILED TO START\nDescription:\nFailed to configure a DataSource", "Java"},
    };
    for (const auto& t : tests) {
        std::string json = translate(t[0], rules);
        expect(lang_is(json, t[1]),
               std::string("input [") + t[0] + "] expected lang " + t[1] + " got: " + json.substr(0, 80));
    }

    // 没有匹配时要返回 found:false，而不是崩溃或乱给结果
    expect(translate("hello world this is not an error", rules) == "{\"found\":false}", "no-match returns found:false");

    // JSON 转义：解释里的引号不能破坏 JSON
    std::vector<Rule> fake;
    fake.push_back({"Test", "boom||crash", "error", "he said \"hi\" \\ ok"});
    std::string json = translate("boom", fake);
    expect(json.find("\\\"hi\\\"") != std::string::npos, "quote escaped in json");
    expect(json.find("\\\\ ok") != std::string::npos, "backslash escaped in json");

    if (g_fail == 0) {
        std::cout << "ALL TESTS PASSED DA☆ZE\n";
        return 0;
    }
    std::cerr << g_fail << " test(s) failed\n";
    return 1;
}
