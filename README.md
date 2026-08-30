# Error Translator

一款玩具式多语言程序报错翻译工具，可以**粘贴报错信息，返回解释和修复建议**

目前支持 22 种编程语言，共收录 1434 条报错类型，加上多个匹配别名能识别 1784+ 种真实报错变体，当然过于复杂的没做匹配

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
| Python      | 80 | `NameError`、`IndexError`、`JSONDecodeError`、`pip` 安装报错                    |
| JavaScript  | 80 | `ReferenceError`、`CORS` 跨域、`Unhandled promise rejection`、`npm ERESOLVE`    |
| Java        | 80 | `NullPointerException`、`cannot find symbol`、`OutOfMemoryError`、Maven/Gradle  |
| C++         | 80 | `undefined reference`、`Segmentation fault`、`C2065`、`heap-use-after-free`     |
| C           | 80 | `implicit declaration`、`incompatible pointer types`、`double free`             |
| Go          | 80 | `undefined:`、`declared and not used`、`nil pointer dereference`、`panic`       |
| C#          | 80 | `NullReferenceException`、`CS1061`、`KeyNotFoundException`、`SqlException`      |
| Ruby        | 80 | `NoMethodError`、`uninitialized constant`、`FrozenError`、`PG::ConnectionBad`   |
| PHP         | 80 | `Parse error`、`Undefined variable`、`Allowed memory size`、`PDOException`      |
| Rust        | 80 | `mismatched types`、`borrow of moved value`、`unwrap` on `Err`、Cargo 报错      |
| TypeScript  | 80 | `TS2304`、`TS2339`、`TS2322`、`@types` 缺失                                     |
| Kotlin      | 80 | `Unresolved reference`、`val cannot be reassigned`、`lateinit` 未初始化         |
| Swift       | 80 | `No such module`、`Unexpectedly found nil`、`cannot convert value of type`      |
| Dart/Flutter| 80 | `Null check operator`、`setState()` 时机、`RenderFlex overflowed`               |
| SQL         | 80 | `ERROR 1064`、唯一键/外键冲突、`relation does not exist`（MySQL/PG/SQLite/MSSQL）|
| Shell       | 80 | `syntax error near unexpected token`、`command not found`、`curl` 网络报错      |
| R           | 80 | `object not found`、`there is no package called`、ggplot2/dplyr 系列报错        |
| Scala       | 79 | `type mismatch`、`not found: value`、`Task not serializable`、sbt/Spark 报错    |
| Lua         | 49 | `attempt to index a nil value`、`attempt to call a nil value`、模块加载失败     |
| Perl        | 72 | `Can't call method on an undefined value`、`requires explicit package name`     |
| Objective-C | 49 | `unrecognized selector`、`key value coding-compliant`、AutoLayout 约束冲突       |
| Julia       | 44 | `MethodError`、`UndefVarError`、`BoundsError`、`InexactError`、Pkg 报错         |

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
