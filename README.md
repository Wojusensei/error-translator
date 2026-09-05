# Error Translator

一款玩具式多语言程序报错翻译工具，可以**粘贴报错信息，返回解释和修复建议**

目前支持 24 种编程语言，共收录 1434 条报错类型，加上多个匹配别名能识别 1784+ 种真实报错变体，当然过于复杂的没做匹配

这是个很烂的模版化小项目，甚至只支持了 Windows ，后端代码都是写着玩的，做复习用，有大部分报错并未收录，请谅解

---

## 🚀 快速开始

### 方式一：网页版

1. 运行 `cpp/translator.exe`
2. 浏览器地址栏输入 `http://localhost:8888`
3. 把报错信息粘贴到输入框
4. 点 **诊断** 按钮
5. 语言，错误等级，解释等出现

### 方式二：命令行版

运行 `cpp/translator.exe` 后直接在黑窗口输入报错关键词如NameError

---

## 支持的语言和报错类型

| 语言 | 收录报错数 | 常见可识别报错举例 |
|-------------|----|--------------------------------------------------------------------------------|
| Python      | 126 | `NameError`、`IndexError`、`JSONDecodeError`、`pip` 安装报错                    |
| JavaScript  | 102 | `ReferenceError`、`CORS` 跨域、`Unhandled promise rejection`、`npm ERESOLVE`    |
| Java        | 115 | `NullPointerException`、`cannot find symbol`、`OutOfMemoryError`、Maven/Gradle  |
| C++         | 118 | `undefined reference`、`Segmentation fault`、`C2065`、`heap-use-after-free`     |
| C           | 96 | `implicit declaration`、`incompatible pointer types`、`double free`             |
| Go          | 98 | `undefined:`、`declared and not used`、`nil pointer dereference`、`panic`       |
| C#          | 110 | `NullReferenceException`、`CS1061`、`KeyNotFoundException`、`SqlException`      |
| Ruby        | 88 | `NoMethodError`、`uninitialized constant`、`FrozenError`、`PG::ConnectionBad`   |
| PHP         | 94 | `Parse error`、`Undefined variable`、`Allowed memory size`、`PDOException`      |
| Rust        | 97 | `mismatched types`、`borrow of moved value`、`unwrap` on `Err`、Cargo 报错      |
| TypeScript  | 91 | `TS2304`、`TS2339`、`TS2322`、`@types` 缺失                                     |
| Kotlin      | 92 | `Unresolved reference`、`val cannot be reassigned`、`lateinit` 未初始化         |
| Swift       | 87 | `No such module`、`Unexpectedly found nil`、`cannot convert value of type`      |
| Dart/Flutter| 87 | `Null check operator`、`setState()` 时机、`RenderFlex overflowed`               |
| SQL         | 84 | `ERROR 1064`、唯一键/外键冲突、`relation does not exist`（MySQL/PG/SQLite/MSSQL）|
| Shell       | 74 | `syntax error near unexpected token`、`command not found`、`curl` 网络报错      |
| R           | 80 | `object not found`、`there is no package called`、ggplot2/dplyr 系列报错        |
| Scala       | 74 | `type mismatch`、`not found: value`、`Task not serializable`、sbt/Spark 报错    |
| Lua         | 49 | `attempt to index a nil value`、`attempt to call a nil value`、模块加载失败     |
| Perl        | 72 | `Can't call method on an undefined value`、`requires explicit package name`     |
| Objective-C | 49 | `unrecognized selector`、`key value coding-compliant`、AutoLayout 约束冲突       |
| Julia       | 44 | `MethodError`、`UndefVarError`、`BoundsError`、`InexactError`、Pkg 报错         |
| Groovy      | 37 | `No signature of method`、`No such property`、Gradle/Jenkins 管线报错           |
| Elixir      | 41 | `no match of right hand side`、`KeyError`、协议未实现、GenServer/mix 报错       |
| Git         | 61 | `not a git repository`、推送被拒、冲突/合并中断、index.lock、身份未配置          |
| Docker      | 62 | `Cannot connect to the Docker daemon`、端口占用、构建失败、compose/WSL2 报错     |
| PowerShell  | 30 | 执行策略禁用脚本、`is not recognized`、参数绑定、远程 WinRM、.NET 方法调用       |
| VBA         | 31 | `Run-time error '1004'`、`Object required`、下标越界、类型不匹配、Automation 错误 |
| MATLAB      | 27 | `Unrecognized function or variable`、下标越界、维度不一致、cell 用法、许可证       |
| Haskell     | 21 | `Variable not in scope`、`Couldn't match type`、`No instance for`、空列表崩溃      |
| HTTP        | 27 | 4xx/5xx 全系状态码：`401`、`403`、`404`、`429` 限流、`502 Bad Gateway`、CF 52x     |
| CI/CD       | 12 | GitHub Actions：退出码含义、runner 停机、action 解析失败、GITHUB_TOKEN 权限       |

---

当然都是最为普通的报错，也就是没啥用的意思

## 项目结构

```
error-translator/
|
├── cpp/ ← C++ 核心服务器（主程序）
│ ├── main.cpp ← 服务器入口（WinSock + HTTP）
│ ├── engine.h ← 匹配引擎（解析 + 匹配，平台无关）
│ ├── engine_test.cpp ← 引擎回归测试（跨平台可跑）
│ └── translator.exe ← 编译好的可执行文件
|
├── python/ ← Python 辅助脚本
│ ├── build_errors.py ← 数据构建器（见下方"改数据"）
│ └── translator.py ← Python 调用示例
|
├── js/ ← 网页前端
│ ├── index.html ← 网页界面（内嵌一份规则快照，离线也能用）
│ └── translator.js ← Node.js 调用示例
|
├── java/ ← Java 示例
│ └── Translator.java ← 调用服务器的示例
|
├── go/ ← Go 示例
│ └── translator.go ← 调用服务器的示例
|
├── csharp/ ← C# 示例
│ └── Translator.cs ← 调用服务器的示例
|
├── data/ ← 数据文件
│ ├── errors.json ← 报错规则库（唯一数据源）
│ └── errors.txt ← 由 errors.json 生成，服务器加载用
└── README.md ← 本说明文档
```

---

## 想改数据 / 加报错？

1. 编辑 `data/errors.json`，每条规则包含 `match`（多个匹配别名用 `||` 分隔，支持 `.*` 通配符）、`level`（error / warning / fatal）、`explain`（中文解释）
2. 运行 `python3 python/build_errors.py`，它会同步生成 `data/errors.txt` 并刷新 `js/index.html` 里内嵌的规则
3. 重新编译 exe（见下方命令）
4. 跑一下引擎测试确认没改坏：`g++ -o engine_test cpp/engine_test.cpp && ./engine_test data/errors.txt`

## 引擎测试

```
g++ -o engine_test cpp/engine_test.cpp && ./engine_test data/errors.txt
```

会校验数据格式（等级、解释、分隔符）并跑一组真实报错的匹配回归，输出 ALL TESTS PASSED DA☆ZE 即通过。

---

## 从源码编译

### 需要准备
- g++ 编译器(MinGW 或 MSYS2)
- Windows 系统(使用了 Winsock)

### 编译命令
```bash
g++ -o cpp/translator.exe cpp/main.cpp -lws2_32 -static -mconsole
```
### 运行
cpp/translator.exe
