"""从 data/errors.json 生成 data/errors.txt，并把规则注入 js/index.html。

errors.json 是唯一的数据源，改完数据跑一次本脚本即可同步所有副本：
    python3 python/build_errors.py
"""
import json, os, re

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
json_path = os.path.join(BASE, "data", "errors.json")
txt_path = os.path.join(BASE, "data", "errors.txt")
html_path = os.path.join(BASE, "js", "index.html")

# 每种语言的示例报错，用于前端侧边栏一键填入
SAMPLES = {'Python': "NameError: name 'data' is not defined", 'JavaScript': "TypeError: Cannot read properties of undefined (reading 'map')", 'Java': 'java.lang.NullPointerException: Cannot invoke "String.length()" because "s" is null', 'C++': "undefined reference to `main'", 'C': "warning: implicit declaration of function 'malloc'", 'Go': './main.go:8:16: undefined: parseJSON', 'C#': 'System.NullReferenceException: Object reference not set to an instance of an object.', 'Ruby': "NoMethodError: undefined method `name' for nil:NilClass", 'PHP': 'Fatal error: Uncaught Error: Call to undefined function get_data()', 'Rust': 'error[E0382]: borrow of moved value: `buffer`', 'TypeScript': "error TS2339: Property 'name' does not exist on type 'User'.", 'Kotlin': 'e: file.kt:10:5 Unresolved reference: onClick', 'Swift': 'Fatal error: Unexpectedly found nil while unwrapping an Optional value', 'Dart/Flutter': 'Null check operator used on a null value', 'SQL': 'ERROR 1064 (42000): You have an error in your SQL syntax', 'Shell': "syntax error near unexpected token `fi'", 'R': "Error: object 'df' not found", 'Scala': 'type mismatch;\n found   : String\n required: Int', 'Lua': "attempt to index a nil value (global 'config')", 'Perl': 'Can\'t call method "get" on an undefined value at script.pl line 12.', 'Objective-C': '-[UIView setFoo:]: unrecognized selector sent to instance 0x0', 'Julia': 'MethodError: no method matching +(::String, ::Int64)', 'Groovy': 'groovy.lang.MissingMethodException: No signature of method: Script.foo()', 'Elixir': '** (MatchError) no match of right hand side value: :error', 'Git': 'fatal: not a git repository (or any of the parent directories): .git', 'Docker': 'Cannot connect to the Docker daemon at unix:///var/run/docker.sock. Is the docker daemon running?', 'PowerShell': "Get-ChildItem : Cannot find path 'C:\\nope' because it does not exist.", 'VBA': "Run-time error '1004': Application-defined or object-defined error", 'MATLAB': 'Index in position 1 exceeds array bounds (must not exceed 5).', 'Haskell': 'Prelude.head: empty list'}


# 规则类别自动分类：按匹配串关键词归类，供前端筛选与展示。
# 检查顺序即优先级，第一个命中的类别生效；都没有就归"运行时"。
CATEGORY_RULES = [
    ("编译/链接", ["syntax", "unexpected", "expected", "parse", "compile", "unresolved", "undeclared",
                  "redeclaration", "duplicate", "in this scope", "indentation", "delimiter", "brace",
                  "semicolon", "unterminated", "does not name", "not declared", "redefinition",
                  "error[e", "error ts", "error cs", "cs0", "cs1", "c20", "c26", "c1083", "lnk", "aapt",
                  "ts2", "ts3", "ts7", "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "assembler"]),
    ("数据库", ["sqlstate", "sqlite", "mysql", "postgres", "pg::", "ora-", "django.db", "ecto",
               "table or view", "column", "constraint", "no such table", "no such column",
               "sql ", "connection is closed", "queued"]),
    ("网络", ["connection", "socket", "network", "dns", "getaddrinfo", "enotfound", "econnrefused",
             "econnreset", "etimedout", "ssl", "tls", "certificate", "http", "cors", "curl",
             "hostname", "refused", "proxy", "webhook", "broker", "request failed", "fetch"]),
    ("权限", ["permission", "denied", "eacces", "eperm", "access denied", "unauthorized",
             "forbidden", "401", "403", "sudoers", "privilege", "administrator", "read-only"]),
    ("内存/资源", ["out of memory", "oom", "heap", "memory allocation", "cannot allocate",
                 "bad_alloc", "memoryerror", "resources exhausted"]),
    ("并发/异步", ["thread", "goroutine", "mutex", "deadlock", "concurrent", "data race",
                 "async", "await", "coroutine", "promise", "future", "waitgroup", "suspension",
                 "yield across", "cancelled", "canceled"]),
    ("依赖/环境", ["no module named", "module not found", "cannot find module", "unresolved import",
                 "importerror", "err_module", "require", "dependency", "dependencies", "pip",
                 "npm err", "package", "gem ", "cargo", "composer", "@types", "unresolved reference",
                 "cannot find package", "version solving", "not installed", "missing dependency"]),
    ("文件/IO", ["file", "path", "directory", "enoent", "eisdir", "fopen", "ifstream",
               "file_get_contents", "open file", "no space", "disk", "stream", "eof"]),
    ("配置", ["improperlyconfigured", "configuration", "settings", "environment variable",
             "yaml", "toml", "ini ", "config file", "manifest", "tsconfig", "pubspec"]),
]

def categorize(match_text):
    text = match_text.lower()
    for category, keywords in CATEGORY_RULES:
        for kw in keywords:
            if kw in text:
                return category
    return "运行时"


def load_data():
    with open(json_path, "r", encoding="utf-8") as f:
        return json.load(f)


def build_txt(data):
    lines = []
    for lang_key, lang_info in data.get("languages", {}).items():
        name = lang_info.get("name", lang_key)
        for pat in lang_info.get("patterns", {}).values():
            match = pat.get("match", "")
            level = pat.get("level", "error")
            # 用 | 分隔字段，解释里的 | 会破坏格式，替换成 /
            explain = pat.get("explain", "").replace("|", "/")
            category = pat.get("category") or categorize(match + " " + explain)
            lines.append(f"{name}|{match}|{level}|{explain}|{category}")
    with open(txt_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    return len(lines)


def inject_html(data):
    rules = []
    for lang_key, lang_info in data.get("languages", {}).items():
        name = lang_info.get("name", lang_key)
        for pat in lang_info.get("patterns", {}).values():
            rules.append({
                "lang": name,
                "match": pat.get("match", ""),
                "level": pat.get("level", "error"),
                "explain": pat.get("explain", ""),
                "category": pat.get("category") or categorize(pat.get("match", "") + " " + pat.get("explain", "")),
            })
    rules_js = json.dumps(rules, ensure_ascii=False, separators=(",", ":"))

    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    # 标记包裹整个字面量（包括方括号/花括号），避免注入后出现双层括号。
    # 用 lambda 做替换，防止 re.subn 把 JSON 里的 \n 等转义解释成真实换行。
    html, n1 = re.subn(
        r"/\*RULES_DATA_START\*/.*?/\*RULES_DATA_END\*/",
        lambda m: "/*RULES_DATA_START*/" + rules_js + "/*RULES_DATA_END*/",
        html, flags=re.S)
    samples_js = json.dumps(SAMPLES, ensure_ascii=False)
    html, n2 = re.subn(
        r"/\*SAMPLES_START\*/.*?/\*SAMPLES_END\*/",
        lambda m: "/*SAMPLES_START*/" + samples_js + "/*SAMPLES_END*/",
        html, flags=re.S)

    with open(html_path, "w", encoding="utf-8") as f:
        f.write(html)
    return len(rules), n1, n2


if __name__ == "__main__":
    data = load_data()
    n_txt = build_txt(data)
    n_html, n1, n2 = inject_html(data)
    print(f"OK! {n_txt} rules -> data/errors.txt")
    print(f"OK! {n_html} rules injected into js/index.html (rules={n1}, samples={n2})")
