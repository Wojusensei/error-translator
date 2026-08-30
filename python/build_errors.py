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
SAMPLES = {'Python': "NameError: name 'data' is not defined", 'JavaScript': "TypeError: Cannot read properties of undefined (reading 'map')", 'Java': 'java.lang.NullPointerException: Cannot invoke "String.length()" because "s" is null', 'C++': "undefined reference to `main'", 'C': "warning: implicit declaration of function 'malloc'", 'Go': './main.go:8:16: undefined: parseJSON', 'C#': 'System.NullReferenceException: Object reference not set to an instance of an object.', 'Ruby': "NoMethodError: undefined method `name' for nil:NilClass", 'PHP': 'Fatal error: Uncaught Error: Call to undefined function get_data()', 'Rust': 'error[E0382]: borrow of moved value: `buffer`', 'TypeScript': "error TS2339: Property 'name' does not exist on type 'User'.", 'Kotlin': 'e: file.kt:10:5 Unresolved reference: onClick', 'Swift': 'Fatal error: Unexpectedly found nil while unwrapping an Optional value', 'Dart/Flutter': 'Null check operator used on a null value', 'SQL': 'ERROR 1064 (42000): You have an error in your SQL syntax', 'Shell': "syntax error near unexpected token `fi'", 'R': "Error: object 'df' not found", 'Scala': 'type mismatch;\n found   : String\n required: Int', 'Lua': "attempt to index a nil value (global 'config')", 'Perl': 'Can\'t call method "get" on an undefined value at script.pl line 12.', 'Objective-C': '-[UIView setFoo:]: unrecognized selector sent to instance 0x0', 'Julia': 'MethodError: no method matching +(::String, ::Int64)', 'Groovy': 'groovy.lang.MissingMethodException: No signature of method: Script.foo()', 'Elixir': '** (MatchError) no match of right hand side value: :error', 'Git': 'fatal: not a git repository (or any of the parent directories): .git', 'Docker': 'Cannot connect to the Docker daemon at unix:///var/run/docker.sock. Is the docker daemon running?'}


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
            lines.append(f"{name}|{match}|{level}|{explain}")
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
