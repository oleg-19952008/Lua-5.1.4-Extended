# Lua 5.1.4 Extended

**Язык / Language:** Русский · [English](README.en.md) · [简体中文](README.zh-CN.md)

Модифицированная сборка Lua 5.1.4 с дополнительными модулями JSON, XML и Base64, интегрированными непосредственно в стандартную систему инициализации библиотек Lua.

Интеграция и модификация исходного кода выполнялись с использованием нейросетей. Контроль процесса сборки, проверка работоспособности и тестирование осуществлялись человеком.

Это **не** официальный релиз Lua и **не** оригинальная сборка PUC-Rio. Проект сохраняет основу Lua 5.1.4 и расширяет её дополнительными функциями.

## Возможности

| Модуль | Назначение | API |
| --- | --- | --- |
| `json` | Кодирование и разбор JSON | `encode`, `encode_pretty`, `decode`, `*_safe`, `null` |
| `xml` | Разбор XML и поиск узлов | `parse`, `path`, `*_safe` |
| `base64` | Кодирование и декодирование Base64 | `encode`, `decode`, `encode_url`, `decode_url`, `isvalid`, `*_safe` |

Модули регистрируются в `luaL_openlibs()`. После открытия стандартных библиотек они доступны как глобальные таблицы `json`, `xml` и `base64`; отдельный вызов `require()` не нужен.

## Платформы

| Платформа | Статус |
| --- | --- |
| Windows | Проект Visual Studio 2022 предусмотрен в репозитории |
| Linux x86-64 | Сборка и тесты проверены в Debian 13 с GCC 14.2 |
| macOS | Не проверялась |

### Linux: проверенная сборка

В исходном `src/Makefile` есть цель `linux`, которая включает Readline. В проверенном окружении отсутствовали заголовки и библиотеки Readline, поэтому штатная цель `linux` не использовалась.

Вместо неё была применена общая цель `generic`, которая собирает Lua без Readline:

```sh
make -C src clean
make -C src generic -j2
```

После сборки исполняемые файлы находятся в `src/`:

```
src/lua
src/luac
src/liblua.a
```

Это именно обход зависимости от Readline, а не исправление или изменение ядра ради Linux. В таком варианте интерактивная консоль работает без интеграции с GNU Readline (например, без его расширенного редактирования строки и истории).

Запуск тестов:

```sh
./src/lua test_modules.lua
./src/lua test_regressions.lua
```

Результат проверки в Debian 13 / GCC 14.2:

`test_modules.lua`:

```
All tests done.
```

`test_regressions.lua`:

```
regression tests: OK
```

Дополнительно проверялись простые операции JSON encode/decode, Base64 encode/decode и XML parse/path. Smoke test завершился сообщением:

```
Linux smoke test: OK
```

Во время компиляции GCC вывел несколько предупреждений в старом коде Lua и XML-библиотеке; они не остановили сборку и не помешали указанным тестам.

> **Важно:** Linux-проверка подтверждает сборку и работу перечисленных тестов на Debian 13 x86-64 с использованием `make generic`. Она **не** означает, что проверена штатная цель `make linux` с Readline или другие дистрибутивы и архитектуры.

### Windows: Visual Studio 2022

1. Установите workload **Desktop development with C++** (MSVC v143 и Windows SDK).
2. Откройте `lua51.sln`.
3. Выберите конфигурацию `Release` и платформу `x64` (либо `Win32`).
4. Выполните **Build → Build Solution** или нажмите `Ctrl+Shift+B`.

В проекте предусмотрены конфигурации `Debug` / `Release` и `Win32` / `x64`.

### macOS

macOS пока не тестировалась. В `src/Makefile` присутствует цель `macosx`, однако наличие такой цели само по себе не является подтверждением успешной сборки на актуальной macOS. Требуется отдельная проверка с Clang и системными инструментами разработчика.

## Использование

После сборки запустите `lua` (Linux) или `lua.exe` (Windows). Дополнительные модули доступны сразу:

```lua
print(_VERSION)

local encoded = json.encode({ message = "Hello" })
print(encoded)

print(base64.encode("Hello"))

local doc = xml.parse("<root><item>value</item></root>")
print(doc.name)
```

## API модулей

### JSON — `json`

Основан на cJSON.

| Функция / поле | Описание |
| --- | --- |
| `json.encode(value)` | Компактное JSON-представление |
| `json.encode_pretty(value)` | JSON с форматированием |
| `json.decode(string)` | Разбор JSON-строки |
| `json.encode_safe(value)` | Безопасный вариант `encode` |
| `json.encode_pretty_safe(value)` | Безопасный вариант `encode_pretty` |
| `json.decode_safe(string)` | Безопасный вариант `decode` |
| `json.null` | Специальное значение для JSON `null` |

Пример:

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

`json.null` используется для представления JSON `null`, поскольку Lua `nil` не может храниться как значение таблицы:

```lua
local value = json.decode('{"result":null}')
if value.result == json.null then
    print("JSON null")
end
```

### XML — `xml`

Основан на `ooxi/xml.c`.

| Функция | Описание |
| --- | --- |
| `xml.parse(string)` | Разбирает XML и возвращает корневой узел |
| `xml.path(node, path)` | Ищет дочерние узлы по упрощённому пути |
| `xml.parse_safe(string)` | Безопасный вариант `parse` |
| `xml.path_safe(node, path)` | Безопасный вариант `path` |

Узел представлен Lua-таблицей с полями `name`, `content` (если есть текстовое содержимое), `attr`, `attrs` и `children`.

Пример:

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

Поиск с предикатом атрибута:

```lua
local found = xml.path(root, "item[@id=2]")
if found[1] then
    print(found[1].content)
end
```

`xml.path` — упрощённый XPath-подобный поиск, а **не** полноценная реализация XPath. Поддерживаются сегменты пути через `/` и предикат равенства атрибута вида `[@attribute=value]`. Полный синтаксис XPath не поддерживается.

### Base64 — `base64`

Основан на библиотеке NibbleAndAHalf.

| Функция | Описание |
| --- | --- |
| `base64.encode(string)` | Кодирование в Base64 |
| `base64.decode(string)` | Декодирование Base64 |
| `base64.encode_url(string)` | URL-safe Base64 без завершающего padding |
| `base64.decode_url(string)` | Декодирование URL-safe Base64 |
| `base64.isvalid(string)` | Проверка корректности Base64 |
| `base64.encode_safe(string)` | Безопасный вариант `encode` |
| `base64.decode_safe(string)` | Безопасный вариант `decode` |

Пример:

```lua
local encoded = base64.encode("Hello, Lua!")
local decoded = base64.decode(encoded)
print(encoded)
print(decoded)

local url_encoded = base64.encode_url("https://example.com/")
print(base64.decode_url(url_encoded))
```

Safe-варианты возвращают результат либо `nil, err` при ошибке. Для `encode_url` и `decode_url` отдельные safe-варианты не зарегистрированы.

## Тесты

В репозитории находятся:

- `test_modules.lua` — базовая проверка JSON, XML и Base64;
- `test_regressions.lua` — регрессионные проверки, включая обработку `json.null`, бинарных строк, циклических таблиц и некорректного ввода.

Запуск на Linux:

```sh
./src/lua test_modules.lua
./src/lua test_regressions.lua
```

Запуск на Windows (пример для `Release | x64`, если бинарные файлы размещены в указанном каталоге):

```bat
bin\x64\Release\lua.exe test_modules.lua
bin\x64\Release\lua.exe test_regressions.lua
```

## Структура репозитория

```
lua51-extended/
 ├── lua51.sln
 ├── liblua.vcxproj
 ├── lua.vcxproj
 ├── luac.vcxproj
 ├── src/
 │   ├── исходники Lua 5.1.4
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
 ├── README.md
 ├── README.en.md
 └── README.zh-CN.md
```

## Лицензия

Лицензия проекта — **zlib License**. Текст лицензии находится в файле [LICENSE](LICENSE).

В репозитории также присутствуют сторонние компоненты, которые сохраняют собственные исходные уведомления и лицензии. В частности, Lua 5.1.4 и cJSON содержат MIT-лицензионные уведомления, а `ooxi/xml.c` и NibbleAndAHalf Base64 — собственные zlib-подобные уведомления. Лицензия проекта не отменяет и не заменяет лицензии этих компонентов. При распространении сохраняйте оригинальные copyright- и license-notices в исходных файлах `src/` и `deps/`.

## Авторство документации

Документация `README.md` подготовлена с помощью ChatGPT (OpenAI) на основе предоставленного исходного кода и результатов проверки Linux-сборки. Переводы на английский и китайский языки также выполнены с помощью ChatGPT (OpenAI).