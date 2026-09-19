"""调用本机 error-translator 服务的命令行示例（仅用标准库，无需 requests）"""
import json
import urllib.parse
import urllib.request

SERVER = "http://127.0.0.1:8888"


def translate(text: str) -> dict:
    url = f"{SERVER}/translate?q=" + urllib.parse.quote(text)
    with urllib.request.urlopen(url, timeout=3) as resp:
        return json.loads(resp.read().decode("utf-8"))


def show(r: dict) -> None:
    if not r.get("found"):
        print("未命中规则")
        return
    print(f"[{r['lang']}] {r['level']}  类别: {r.get('category', '-')}")
    src = r.get("source")
    if src:
        print(f"出错位置: {src['file']}:{src['line']}")
    print(r["explain"])
    for alt in r.get("alternatives", []):
        print(f"  其他可能: [{alt['lang']}] {alt['match'].split('||')[0]} - {alt['explain']}")


if __name__ == "__main__":
    text = input("Enter error message: ")
    try:
        show(translate(text))
    except Exception as e:
        print(f"[ERROR] Server not running qwq ({e})")
