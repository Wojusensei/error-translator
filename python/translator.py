import requests

SERVER = "http://localhost:8888"

def translate():
    text = input("Enter error message: ")
    try:
        r = requests.get(f"{SERVER}/translate", params={"q": text})
        print(r.json())
    except:
        print("[ERROR] Server not running qwq")

if __name__ == "__main__":
    translate()