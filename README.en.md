# Lua 5.1.4 Extended
**Language / 语言:** [Русский](README.md) · English · [简体中文](README.zh-CN.md)
A modified build of Lua 5.1.4 with additional JSON, XML and Base64 modules integrated directly into Lua's standard library initialization system.

The source code integration and modification were carried out using neural networks (AI). Build control, functionality verification and testing were performed by a human.

This is **not** an official Lua release and **not** the original PUC-Rio build. The project keeps the Lua 5.1.4 core and extends it with additional functions.

## Features

| Module | Purpose | API |
| --- | --- | --- |
| `json` | JSON encoding and parsing | `encode`, `encode_pretty`, `decode`, `*_safe`, `null` |
| `xml` | XML parsing and node lookup | `parse`, `path`, `*_safe` |
| `base64` | Base64 encoding and decoding | `encode`, `decode`, `encode_url`, `decode_url`, `isvalid`, `*_safe` |

The modules are registered in `luaL_openlibs()`. Once the standard libraries are opened, they are available as the global tables `json`, `xml` and `base64`; no separate `require()` call is needed.

## Platforms

| Platform | Status |
| --- | --- |
| Windows | A Visual Studio 2022 project is provided in the repository |
| Linux x86-64 | Build and tests verified on Debian 13 with GCC 14.2 |
| macOS | Not tested |

### Linux: verified build

The original `src/Makefile` contains a `linux` target that enables Readline. In the verified environment the Readline headers and libraries were missing, so the stock `linux` target was not used.

Instead, the general-purpose `generic` target was used, which builds Lua without Readline:

```sh
make -C src clean
make -C src generic -j2
```

After the build, the executables are located in `src/`:

```
src/lua
src/luac
src/liblua.a
```

This is strictly a workaround for the Readline dependency, not a fix or a modification of the core for Linux's sake. In this configuration the interactive console works without GNU Readline integration (for example, without its advanced line editing and history features).

Running the tests:

```sh
./src/lua test_modules.lua
./src/lua test_regressions.lua
```

Verification results on Debian 13 / GCC 14.2:

`test_modules.lua`:

```
All tests done.
```

`test_regressions.lua`:

```
regression tests: OK
```

Simple JSON encode/decode, Base64 encode/decode and XML parse/path operations were additionally checked. The smoke test finished with the message:

```
Linux smoke test: OK
```

During compilation GCC emitted several warnings in the legacy Lua code and in the XML library; they did not stop the build and did not prevent the tests listed above from passing.

> **Important:** the Linux verification confirms the build and the operation of the listed tests on Debian 13 x86-64 using `make generic`. It does **not** mean that the stock `make linux` target with Readline, or other distributions and architectures, have been verified.

### Windows: Visual Studio 2022

1. Install the **Desktop development with C++** workload (MSVC v143 and the Windows SDK).
2. Open `lua51.sln`.
3. Select the `Release` configuration and the `x64` platform (or `Win32`).
4. Run **Build → Build Solution**, or press `Ctrl+Shift+B`.

The project provides the `Debug` / `Release` configurations and the `Win32` / `x64` platforms.

### macOS

macOS has not been tested yet. `src/Makefile` contains a `macosx` target, but the mere presence of such a target is not proof of a successful build on a current macOS version. Separate verification with Clang and the system developer tools is required.

## Usage

After building, run `lua` (Linux) or `lua.exe` (Windows). The additional modules are available immediately:

```lua
print(_VERSION)

local encoded = json.encode({ message = "Hello" })
print(encoded)

print(base64.encode("Hello"))

local doc = xml.parse("<root><item>value</item></root>")
print(doc.name)
```

## Module API

### JSON — `json`

Based on cJSON.

| Function / field | Description |
| --- | --- |
| `json.encode(value)` | Compact JSON representation |
| `json.encode_pretty(value)` | Formatted (pretty-printed) JSON |
| `json.decode(string)` | Parses a JSON string |
| `json.encode_safe(value)` | Safe variant of `encode` |
| `json.encode_pretty_safe(value)` | Safe variant of `encode_pretty` |
| `json.decode_safe(string)` | Safe variant of `decode` |
| `json.null` | Special value representing JSON `null` |

Example:

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

`json.null` is used to represent JSON `null`, because Lua's `nil` cannot be stored as a table value:

```lua
local value = json.decode('{"result":null}')
if value.result == json.null then
    print("JSON null")
end
```

### XML — `xml`

Based on `ooxi/xml.c`.

| Function | Description |
| --- | --- |
| `xml.parse(string)` | Parses XML and returns the root node |
| `xml.path(node, path)` | Looks up child nodes by a simplified path |
| `xml.parse_safe(string)` | Safe variant of `parse` |
| `xml.path_safe(node, path)` | Safe variant of `path` |

A node is represented by a Lua table with the fields `name`, `content` (if there is textual content), `attr`, `attrs` and `children`.

Example:

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

Lookup with an attribute predicate:

```lua
local found = xml.path(root, "item[@id=2]")
if found[1] then
    print(found[1].content)
end
```

`xml.path` is a simplified XPath-like search, **not** a full XPath implementation. Path segments separated by `/` and an attribute-equality predicate of the form `[@attribute=value]` are supported. The complete XPath syntax is not supported.

### Base64 — `base64`

Based on the NibbleAndAHalf library.

| Function | Description |
| --- | --- |
| `base64.encode(string)` | Encodes to Base64 |
| `base64.decode(string)` | Decodes Base64 |
| `base64.encode_url(string)` | URL-safe Base64 without trailing padding |
| `base64.decode_url(string)` | Decodes URL-safe Base64 |
| `base64.isvalid(string)` | Checks whether the Base64 input is valid |
| `base64.encode_safe(string)` | Safe variant of `encode` |
| `base64.decode_safe(string)` | Safe variant of `decode` |

Example:

```lua
local encoded = base64.encode("Hello, Lua!")
local decoded = base64.decode(encoded)
print(encoded)
print(decoded)

local url_encoded = base64.encode_url("https://example.com/")
print(base64.decode_url(url_encoded))
```

The safe variants return either the result or `nil, err` on error. No separate safe variants are registered for `encode_url` and `decode_url`.

## Tests

The repository contains:

- `test_modules.lua` — basic checks for JSON, XML and Base64;
- `test_regressions.lua` — regression checks, including handling of `json.null`, binary strings, cyclic tables and malformed input.

Running on Linux:

```sh
./src/lua test_modules.lua
./src/lua test_regressions.lua
```

Running on Windows (example for `Release | x64`, assuming the binaries are placed in the indicated directory):

```bat
bin\x64\Release\lua.exe test_modules.lua
bin\x64\Release\lua.exe test_regressions.lua
```

## Repository structure

```
lua51-extended/
 ├── lua51.sln
 ├── liblua.vcxproj
 ├── lua.vcxproj
 ├── luac.vcxproj
 ├── src/
 │   ├── Lua 5.1.4 sources
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

## License

The project license is the **zlib License**. The license text is in the [LICENSE](LICENSE) file.

The repository also contains third-party components that retain their own original notices and licenses. In particular, Lua 5.1.4 and cJSON contain MIT license notices, while `ooxi/xml.c` and NibbleAndAHalf Base64 carry their own zlib-style notices. The project license does not cancel or replace the licenses of these components. When redistributing, preserve the original copyright and license notices in the source files under `src/` and `deps/`.

## Documentation authorship

This `README.md` documentation was prepared with the help of ChatGPT (OpenAI), based on the provided source code and the results of the Linux build verification.