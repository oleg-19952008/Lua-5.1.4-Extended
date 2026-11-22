# Lua 5.1.4 Extended（扩展版）
**语言 / Language:** [Русский](README.md) · [English](README.en.md) · 简体中文
这是 Lua 5.1.4 的修改版构建，额外集成了 JSON、XML 和 Base64 模块，并直接接入 Lua 标准库的初始化系统。

源代码的集成与修改借助神经网络（AI）完成；构建过程的控制、功能验证与测试由人工执行。

本项目**不是** Lua 官方发行版，也**不是** PUC-Rio 的原始构建。项目保留 Lua 5.1.4 内核，并在其基础上扩展了额外功能。

## 功能特性

| 模块 | 用途 | API |
| --- | --- | --- |
| `json` | JSON 编码与解析 | `encode`、`encode_pretty`、`decode`、`*_safe`、`null` |
| `xml` | XML 解析与节点查找 | `parse`、`path`、`*_safe` |
| `base64` | Base64 编码与解码 | `encode`、`decode`、`encode_url`、`decode_url`、`isvalid`、`*_safe` |

这些模块在 `luaL_openlibs()` 中注册。打开标准库之后，即可通过全局表 `json`、`xml` 和 `base64` 使用它们，无需单独调用 `require()`。

## 平台支持

| 平台 | 状态 |
| --- | --- |
| Windows | 仓库中已提供 Visual Studio 2022 工程 |
| Linux x86-64 | 已在 Debian 13 + GCC 14.2 环境下验证构建与测试 |
| macOS | 尚未测试 |

### Linux：已验证的构建方式

原始 `src/Makefile` 中包含启用 Readline 的 `linux` 目标。在已验证的环境中缺少 Readline 的头文件与库，因此未使用标准的 `linux` 目标。

取而代之的是通用的 `generic` 目标，它会在不启用 Readline 的情况下构建 Lua：

```sh
make -C src clean
make -C src generic -j2
```

构建完成后，可执行文件位于 `src/` 目录：

```
src/lua
src/luac
src/liblua.a
```

这只是对 Readline 依赖的规避方案，并非针对 Linux 修复或改动内核。在这种配置下，交互式控制台可以正常工作，但不具备 GNU Readline 集成（例如没有其增强的行编辑与历史记录功能）。

运行测试：

```sh
./src/lua test_modules.lua
./src/lua test_regressions.lua
```

在 Debian 13 / GCC 14.2 下的验证结果：

`test_modules.lua`：

```
All tests done.
```

`test_regressions.lua`：

```
regression tests: OK
```

此外还检查了基本的 JSON encode/decode、Base64 encode/decode 以及 XML parse/path 操作。冒烟测试（smoke test）以如下信息结束：

```
Linux smoke test: OK
```

编译过程中，GCC 针对 Lua 旧代码和 XML 库输出了若干警告；这些警告没有中断构建，也未影响上述测试的通过。

> **重要提示：** Linux 验证仅确认了在 Debian 13 x86-64 上使用 `make generic` 能够成功构建并通过所列测试。这**并不**意味着已验证带 Readline 的标准 `make linux` 目标，或其他发行版与架构。

### Windows：Visual Studio 2022

1. 安装 **使用 C++ 的桌面开发（Desktop development with C++）** 工作负载（MSVC v143 与 Windows SDK）。
2. 打开 `lua51.sln`。
3. 选择 `Release` 配置与 `x64` 平台（或 `Win32`）。
4. 执行 **生成 → 生成解决方案（Build → Build Solution）**，或按 `Ctrl+Shift+B`。

工程提供了 `Debug` / `Release` 配置以及 `Win32` / `x64` 平台。

### macOS

macOS 尚未测试。`src/Makefile` 中存在 `macosx` 目标，但仅有该目标本身并不能证明可以在当前版本的 macOS 上成功构建。仍需使用 Clang 及系统开发者工具进行单独验证。

## 使用方法

构建完成后运行 `lua`（Linux）或 `lua.exe`（Windows）。附加模块可直接使用：

```lua
print(_VERSION)

local encoded = json.encode({ message = "Hello" })
print(encoded)

print(base64.encode("Hello"))

local doc = xml.parse("<root><item>value</item></root>")
print(doc.name)
```

## 模块 API

### JSON — `json`

基于 cJSON。

| 函数 / 字段 | 说明 |
| --- | --- |
| `json.encode(value)` | 紧凑格式的 JSON 表示 |
| `json.encode_pretty(value)` | 带缩进美化的 JSON |
| `json.decode(string)` | 解析 JSON 字符串 |
| `json.encode_safe(value)` | `encode` 的安全版本 |
| `json.encode_pretty_safe(value)` | `encode_pretty` 的安全版本 |
| `json.decode_safe(string)` | `decode` 的安全版本 |
| `json.null` | 表示 JSON `null` 的特殊值 |

示例：

```lua
local data = {
    name = "Lua",
    version = 5.1,
    enabled = true,
    values = {10, 20, 30}
}

local text = json.encode(data)
local decoded = json.decode(text)

print(decoded.name)
print(decoded.values[2])
```

由于 Lua 的 `nil` 无法作为表的值存储，因此使用 `json.null` 来表示 JSON 的 `null`：

```lua
local value = json.decode('{"result":null}')
if value.result == json.null then
    print("JSON null")
end
```

### XML — `xml`

基于 `ooxi/xml.c`。

| 函数 | 说明 |
| --- | --- |
| `xml.parse(string)` | 解析 XML 并返回根节点 |
| `xml.path(node, path)` | 按简化路径查找子节点 |
| `xml.parse_safe(string)` | `parse` 的安全版本 |
| `xml.path_safe(node, path)` | `path` 的安全版本 |

节点以 Lua 表表示，包含 `name`、`content`（存在文本内容时）、`attr`、`attrs` 和 `children` 字段。

示例：

```lua
local source = [[
<root id="main">
    <item id="1">First</item>
    <item id="2">Second</item>
</root>
]]

local root = xml.parse(source)
print(root.name)
print(root.attrs.id)

local items = xml.path(root, "item")
for i, item in ipairs(items) do
    print(item.attrs.id, item.content)
end
```

带属性谓词的查找：

```lua
local found = xml.path(root, "item[@id=2]")
if found[1] then
    print(found[1].content)
end
```

`xml.path` 是简化的类 XPath 查找，**并非**完整的 XPath 实现。支持以 `/` 分隔的路径片段，以及形如 `[@attribute=value]` 的属性相等谓词。不支持完整的 XPath 语法。

### Base64 — `base64`

基于 NibbleAndAHalf 库。

| 函数 | 说明 |
| --- | --- |
| `base64.encode(string)` | 编码为 Base64 |
| `base64.decode(string)` | 解码 Base64 |
| `base64.encode_url(string)` | URL 安全的 Base64，不带末尾填充（padding） |
| `base64.decode_url(string)` | 解码 URL 安全的 Base64 |
| `base64.isvalid(string)` | 校验 Base64 是否合法 |
| `base64.encode_safe(string)` | `encode` 的安全版本 |
| `base64.decode_safe(string)` | `decode` 的安全版本 |

示例：

```lua
local encoded = base64.encode("Hello, Lua!")
local decoded = base64.decode(encoded)
print(encoded)
print(decoded)

local url_encoded = base64.encode_url("https://example.com/")
print(base64.decode_url(url_encoded))
```

安全版本在成功时返回结果，出错时返回 `nil, err`。`encode_url` 与 `decode_url` 未注册单独的安全版本。

## 测试

仓库中包含：

- `test_modules.lua` —— JSON、XML 与 Base64 的基础检查；
- `test_regressions.lua` —— 回归检查，涵盖 `json.null` 处理、二进制字符串、循环表和非法输入等情况。

在 Linux 上运行：

```sh
./src/lua test_modules.lua
./src/lua test_regressions.lua
```

在 Windows 上运行（以 `Release | x64` 为例，假设二进制文件位于所示目录）：

```bat
bin\x64\Release\lua.exe test_modules.lua
bin\x64\Release\lua.exe test_regressions.lua
```

## 仓库结构

```
lua51-extended/
 ├── lua51.sln
 ├── liblua.vcxproj
 ├── lua.vcxproj
 ├── luac.vcxproj
 ├── src/
 │   ├── Lua 5.1.4 源码
 │   ├── ljsonlib.c
 │   ├── lxmllib.c
 │   ├── lbase64lib.c
 │   └── Makefile
 ├── deps/
 │   ├── cJSON/
 │   ├── xml/
 │   └── base64/
 ├── test_modules.lua
 ├── test_regressions.lua
 ├── LICENSE
 └── README.md
```

## 许可证

本项目采用 **zlib License**。许可证文本见 [LICENSE](LICENSE) 文件。

仓库中还包含第三方组件，它们保留各自原有的声明与许可证。其中，Lua 5.1.4 与 cJSON 含有 MIT 许可声明，而 `ooxi/xml.c` 与 NibbleAndAHalf Base64 则带有各自的类 zlib 声明。本项目的许可证不会取消或替代这些组件的许可证。在分发时，请保留 `src/` 与 `deps/` 源文件中原始的版权与许可声明。

## 文档撰写说明

本 `README.md` 文档在 ChatGPT（OpenAI）的协助下编写，依据为所提供的源代码以及 Linux 构建的验证结果。