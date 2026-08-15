# Error Translator

一款玩具式多语言程序报错翻译工具，可以**粘贴报错信息，返回解释和修复建议**

目前支持 10 种编程语言，覆盖 75 种报错类型，能识别 200 多种真实报错变体，当然过于复杂的没做匹配

这是个很烂的模版化小项目，甚至只支持了 Windows ，后端代码都是写着玩的，做复习用，有大部分报错并未收录，请谅解

---

## 🚀 快速开始

### 方式一：网页版

1. 运行 `cpp/translator.exe`
2. 浏览器地址栏输入 `http://localhost:8888`
3. 把报错信息粘贴到输入框
4. 点 **Translate DA☆ZE** 按钮
5. 语言，错误等级，解释等出现

### 方式二：命令行版

运行 `cpp/translator.exe` 后直接在黑窗口输入报错关键词如NameError

---

## 支持的语言和报错类型

| 语言 | 收录报错数 | 常见可识别报错举例 |
|-------------|----|--------------------------------------------------------------------------------|
| Python      | 18 | `NameError`、`IndexError`、`IndentationError`、`FileNotFoundError`              |
| JavaScript  | 8  | `ReferenceError`、`TypeError`、`is not defined`                                 |
| Java        | 9  | `NullPointerException`、`ArrayIndexOutOfBoundsException`、`cannot find symbol`  |
| C++         | 7  | `undefined reference`、`Segmentation fault`、`was not declared`                 |
| C           | 7  | `implicit declaration`、`dereferencing pointer`、`incompatible pointer`         |
| Go          | 7  | `undefined:`、`cannot use`、`syntax error`、`imported and not used`             |
| C#          | 5  | `NullReferenceException`、`IndexOutOfRangeException`、`; expected`              |
| Ruby        | 5  | `NoMethodError`、`NameError`、`ArgumentError`                                   |
| PHP         | 4  | `Parse error`、`Fatal error`、`Undefined variable`                              |
| Rust        | 5  | `mismatched types`、`cannot borrow`、`use of moved value`                       |

---
当然都是最为普通的报错，也就是没啥用的意思

## 项目结构

```
error-translator/
|
├── cpp/ ← C++ 核心服务器（主程序）
│ ├── main.cpp ← 源代码
│ └── translator.exe ← 编译好的可执行文件
|
├── python/ ← Python 辅助脚本
│ ├── build_errors.py ← 解释库构建器
│ └── translator.py ← Python 调用示例
|
├── js/ ← 网页前端
│ ├── index.html ← 网页界面（双击或用服务器打开）
│ └── translator.js ← Node.js 调用示例
|
├── java/ ← Java 示例
│ ├── ErrorMatcher.java ← 独立匹配引擎
│ └── Translator.java ← 调用服务器的示例
|
├── go/ ← Go 示例
│ └── matcher.go ← 独立匹配引擎
|
├── csharp/ ← C# 示例
│ └── Translator.cs ← 调用服务器的示例
|
├── data/ ← 数据文件
│ └── errors.txt ← 报错规则库（核心数据）
└── README.md ← 本说明文档
```

---

## 从源码编译

### 需要准备
- g++ 编译器(MinGW 或 MSYS2)
- Windows 系统(使用了 WinHTTP 和 Winsock)

### 编译命令
```bash
g++ -o cpp/translator.exe cpp/main.cpp -lwinhttp -lws2_32 -static -mconsole
```
### 运行
cpp/translator.exe




