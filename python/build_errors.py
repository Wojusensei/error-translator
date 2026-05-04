import json, os

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
json_path = os.path.join(BASE, "data", "errors.json")
txt_path  = os.path.join(BASE, "data", "errors.txt")

with open(json_path, "r", encoding="utf-8") as f:
    data = json.load(f)

lines = []
for lang_key, lang_info in data.get("languages", {}).items():
    name = lang_info.get("name", lang_key)
    for pat_key, pat in lang_info.get("patterns", {}).items():
        match = pat.get("match", "")
        level = pat.get("level", "error")
        explain = pat.get("explain", "")
        # 用 | 分隔，解释里的 | 替换成 /
        explain = explain.replace("|", "/")
        lines.append(f"{name}|{match}|{level}|{explain}")

with open(txt_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))

print(f"OK! {len(lines)} rules written to data/errors.txt")