# Error Translator

一款玩具式多语言程序报错翻译工具，可以**粘贴报错信息，返回解释和修复建议**

目前支持 37 个类目（30 种编程语言 + HTTP/CI/CD/Git/Docker/Kubernetes/Nginx 等工具链），共收录 2382 条报错类型，加上多个匹配别名能识别 2900+ 种真实报错变体，当然过于复杂的没做匹配

这是个很烂的模版化小项目，后端代码都是写着玩的，做复习用，有大部分报错并未收录，请谅解

---

## ✨ 核心特性

- **语言自动识别**：粘贴报错后按结构特征（Traceback、`error[E]`、`panic:`、`Run-time error '` 等）先判断语言，再在该语言的规则内匹配，识别失败才回退全库——大幅减少跨语言误报
- **Top-3 候选**：主结果之外附带最多两个"其他可能"，点一下即切换
- **出错位置提取**：Python Traceback 自动取最后一条异常对应的 `File:line`，JS/V8 堆栈取最后一个 `at` 帧，一键复制
- **规则类别**：全部规则自动归类为 编译/链接、网络、数据库、权限、内存/资源、并发/异步、依赖/环境、文件/IO、配置、运行时 十类，侧栏可筛选
- **离线可用**：网页版内嵌完整规则快照，不起服务双击打开也能用（本地引擎模式）

---

## 🚀 快速开始

### 方式一：网页版

1. 运行 `cpp/translator.exe`
2. 浏览器地址栏输入 `http://127.0.0.1:8888`
3. 把报错信息粘贴到输入框，点 **诊断**
4. 语言、错误等级、类别、解释、出错位置（如有）全部出现

> 服务只监听 `127.0.0.1`，仅本机可访问；端口被占用会直接报错退出而不是假启动。

### 方式二：命令行版

运行 `cpp/translator.exe` 后直接在黑窗口输入报错关键词如 NameError

### 方式三：纯前端离线

不起 exe，直接双击打开 `js/index.html`（需与 `app.js`、`app.css` 在同一目录）。页面进入"本地引擎模式"，用内嵌的规则快照在浏览器里匹配，同样支持识别与候选。

---

## 📚 支持的类目与规则数

| 类目 | 规则数 | 类目 | 规则数 |
|---|---|---|---|
| Python | 126 | Java | 115 |
| C++ | 118 | C# | 110 |
| Go | 98 | Rust | 97 |
| C | 96 | PHP | 94 |
| TypeScript | 91 | Kotlin | 92 |
| Swift | 87 | Dart/Flutter | 87 |
| Ruby | 88 | SQL | 84 |
| Shell | 74 | R | 80 |
| Scala | 74 | Lua | 49 |
| Perl | 72 | Objective-C | 49 |
| Julia | 44 | Elixir | 41 |
| Groovy | 37 | Haskell | 21 |
| Git | 61 | Docker | 62 |
| PowerShell | 30 | VBA | 31 |
| MATLAB | 27 | HTTP（状态码） | 27 |
| CI/CD | 21 | 包管理器 | 15 |
| 前端构建 | 12 | Kubernetes | 30 |
| Nginx | 22 | Make/CMake | 18 |

热门语言（Python/JS/Java/C++/C#/Go/Rust/TS）都在 90~130 条，覆盖从语法错误到框架（Django/Spring/React/EF Core/MyBatis）、构建工具（webpack/pip/Maven）、运行时（ASan/JVM/Tokio）的高频与常见冷门报错；工具链类目收录 K8s/Nginx/Git/Docker/CI 的日常排障报错。

---

## 🏗 项目结构

```
error-translator/
|
├── cpp/ ← C++ 核心服务器（主程序）
│ ├── main.cpp ← 服务器入口（HTTP 路由/日志/超时/127.0.0.1）
│ ├── engine.h ← 匹配引擎（识别/排序/解析，平台无关）
│ ├── engine_test.cpp ← 引擎回归测试（跨平台可跑）
│ └── translator.exe ← 编译好的可执行文件
|
├── python/ ← Python 辅助脚本
│ ├── build_errors.py ← 数据构建器（生成 errors.txt + 注入 app.js）
│ ├── validate_errors.py ← 数据自检（构建前强制执行）
│ └── translator.py ← Python 调用示例（纯标准库）
|
├── js/ ← 网页前端
│ ├── index.html ← HTML 外壳
│ ├── app.js ← 前端逻辑 + 内嵌规则快照
│ ├── app.css ← 样式
│ └── translator.js ← Node.js 调用示例
|
├── go|java|csharp/ ← 各语言调用示例（全部可编译）
│
├── data/ ← 数据文件
│ ├── errors.json ← 报错规则库（唯一数据源）
│ └── errors.txt ← 由 errors.json 生成，服务器加载用
└── README.md ← 本说明文档
```

## 🔧 架构与数据流

```
data/errors.json ──(validate_errors.py 自检)──▶ build_errors.py
                                                    │
                        ┌───────────────────────────┼──────────────────┐
                        ▼                           ▼                  ▼
                 data/errors.txt            js/app.js 内嵌规则    （三份保持同步）
                        │                           │
                        ▼                           ▼
              cpp/main.cpp 加载 ──▶ engine.h    浏览器本地引擎
                        │        （语言识别 → 候选排序 → 两轮匹配）
                        ▼
              GET /translate?q=…  →  JSON（found/lang/level/category/
                                      explain/source/alternatives）
```

匹配过程：先按 `detect_language()` 识别语言 → 在该语言的规则子集里按"命中位置最早"排序取前 3 → 无命中再回退全库 → 输出主结果与 alternatives。

## 🌐 API

```
GET /translate?q=<报错原文>        # URL 编码，GET / 返回网页
```

响应示例：

```json
{
  "found": true,
  "lang": "Python",
  "match": "NameError||name .* is not defined",
  "level": "error",
  "category": "运行时",
  "explain": "你用了还没有定义的变量。……",
  "source": { "file": "app.py", "line": 12 },
  "alternatives": [
    { "lang": "Ruby", "match": "NameError||uninitialized constant", "level": "error", "explain": "……" }
  ]
}
```

- `source` 仅当输入是 Python Traceback 或 JS/V8 堆栈时出现
- `alternatives` 最多 2 个，与主结果一起构成 top-3 候选
- 未命中返回 `{"found":false}`

---

## ✏️ 想改数据 / 加报错？

1. 编辑 `data/errors.json`。每条规则包含：
   - `match`：多个匹配别名用 `||` 分隔，支持一个 `.*` 通配符（`name .* is not defined`）
   - `level`：error / warning / fatal
   - `explain`：中文解释（不能含 `|`、换行、引号、反斜杠）
2. 跑 `python3 python/validate_errors.py` 自检（构建时也会自动跑），冲突和格式问题会当场报出来
3. 跑 `python3 python/build_errors.py`，同步生成 `data/errors.txt` 并刷新 `js/app.js` 内嵌规则
4. 跑引擎测试确认没改坏（见下）
5. 重新编译 exe（见下）

## 🧪 引擎测试

```
g++ -o engine_test cpp/engine_test.cpp && ./engine_test data/errors.txt
```

内容：全库规则格式校验 + 60 余组真实报错的匹配回归（含语言识别优先级、
Traceback/堆栈源定位、边界输入、排序稳定性等），输出 ALL TESTS PASSED DA☆ZE 即通过。

---

## 🛠 从源码编译

### 需要准备
- g++ 编译器(MinGW 或 MSYS2)
- Windows 系统(使用了 Winsock)

### 编译命令
```bash
g++ -o cpp/translator.exe cpp/main.cpp -lws2_32 -static -mconsole
```
### 运行
cpp/translator.exe

---

## ⚠️ 已知限制

- 服务器仅支持 Windows（WinSock）；匹配引擎本身平台无关（engine.h），命令行测试在 macOS/Linux 可直接编译
- 单线程 accept，同一时刻只处理一个请求（诊断本身微秒级，实际无感）
- 识别失败时回退全库匹配，个别跨语言同名词（如 `TypeError`）可能给出相近但不对应语言的解释——此时参考"其他可能"候选
