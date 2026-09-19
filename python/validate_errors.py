"""数据自检：在把 errors.json 生成为 errors.txt / index.html 之前检查质量与冲突。

用法：
    python3 python/validate_errors.py            # 输出报告，有 error 时退出码 1
    python3 python/validate_errors.py --strict   # warning 也视为失败（CI 用）

检查项：
- level 只能是 error / warning / fatal
- 别名：非空、无首尾空格、无换行、不含竖线、规则内不重复
- 解释：足够长、不含 | 换行 引号 反斜杠（会破坏 errors.txt 五字段与 JSON 输出）
- 跨语言：完全相同的别名出现在不同语言时，解释相同视为良性（警告），
  解释不同视为冲突（error）；不同语言间别名互为子串（>=5 字符）列入冲突清单
"""
import json
import os
import re
import sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JSON_PATH = os.path.join(BASE, "data", "errors.json")

VALID_LEVELS = {"error", "warning", "fatal"}
BAD_IN_EXPLAIN = ('|', '\n', '"', '\\')
# 异常类名（TypeError / IOException / KeyError: 等）：同名类在多语言里是常态，
# 引擎的语言自动识别负责区分，解释本来就该按语言各写各的
CLASSNAME_RE = re.compile(r"^[A-Z][A-Za-z0-9_]*(:)?$")
# C 和 C++ 共用 GCC/Clang 工具链，编译器消息天然一字不差
SHARED_TOOLCHAIN = {"C", "C++"}


def iter_rules(data):
    for lang_key, lang in data.get("languages", {}).items():
        name = lang.get("name", lang_key)
        for pid, pat in lang.get("patterns", {}).items():
            yield name, pid, pat


def check(data):
    errors, warnings = [], []

    alias_owner = {}   # 别名 -> (语言, 解释)，查跨语言完全重复
    for name, pid, pat in iter_rules(data):
        where = f"{name}/{pid}"

        level = pat.get("level")
        if level not in VALID_LEVELS:
            errors.append(f"{where}: level 非法 {level!r}")

        match = pat.get("match", "")
        raw_aliases = match.split("||")
        for raw in raw_aliases:
            if raw != raw.strip():
                warnings.append(f"{where}: 别名首尾有空格 {raw!r}")
        seen = set()
        for raw in raw_aliases:
            a = raw.strip()
            if not a:
                errors.append(f"{where}: 存在空别名")
                continue
            if a in seen:
                warnings.append(f"{where}: 规则内别名重复 {a!r}")
            seen.add(a)
            if "\n" in a or "|" in a:
                errors.append(f"{where}: 别名含换行或竖线 {a!r}")
            if a in alias_owner and alias_owner[a][0] != name:
                prev_lang, prev_explain = alias_owner[a]
                if prev_explain == pat.get("explain"):
                    warnings.append(f"跨语言重复别名（解释相同，良性）: {a!r} @ {prev_lang} / {name}")
                elif CLASSNAME_RE.match(a) or {prev_lang, name} <= SHARED_TOOLCHAIN:
                    warnings.append(f"跨语言同名（类名靠语言识别区分 / C 与 C++ 同编译器，良性）: "
                                    f"{a!r} @ {prev_lang} / {name}")
                else:
                    errors.append(f"跨语言别名冲突: {a!r} 同时属于 {prev_lang} 和 {name}，解释不同")
            else:
                alias_owner[a] = (name, pat.get("explain"))

        explain = pat.get("explain", "")
        if len(explain.strip()) < 8:
            errors.append(f"{where}: 解释太短")
        for c in BAD_IN_EXPLAIN:
            if c in explain:
                errors.append(f"{where}: 解释含非法字符（| / 换行 / 引号 / 反斜杠）")

    # 子串冲突：不同语言的普通别名（不含 .*）互为子串时，短别名可能抢到长别名的输入。
    # 检测语言失败回退全库匹配时才会发生，但值得人工过目。
    plain = []
    for name, pid, pat in iter_rules(data):
        for a in pat.get("match", "").split("||"):
            a = a.strip()
            if a and ".*" not in a and len(a) >= 5:
                plain.append((a, name))
    plain.sort(key=lambda x: -len(x[0]))
    substring_pairs = []
    for i in range(len(plain)):
        a, la = plain[i]
        for j in range(i + 1, len(plain)):
            b, lb = plain[j]
            if la == lb:
                continue
            if a in b or b in a:
                substring_pairs.append((a, la, b, lb))
                if len(substring_pairs) >= 200:
                    break
        if len(substring_pairs) >= 200:
            break
    return errors, warnings, substring_pairs


def main():
    strict = "--strict" in sys.argv
    with open(JSON_PATH, encoding="utf-8") as f:
        data = json.load(f)
    errors, warnings, substring_pairs = check(data)

    print(f"检查完成：{sum(len(l['patterns']) for l in data['languages'].values())} 条规则，"
          f"{len(data['languages'])} 个类目")
    for e in errors:
        print(f"[ERROR] {e}")
    for w in warnings:
        print(f"[WARN ] {w}")
    print(f"子串冲突（需人工过目，>=5 字符，前 200 条）：{len(substring_pairs)} 对")
    for a, la, b, lb in substring_pairs[:40]:
        print(f"  [SUB  ] {la!r} {a!r}  <->  {lb!r} {b!r}")
    print(f"汇总：error={len(errors)} warning={len(warnings)}")

    if errors or (strict and warnings):
        sys.exit(1)


if __name__ == "__main__":
    main()
