local function eq(a, b, msg)
  assert(a == b, (msg or "values differ") .. ": " .. tostring(a) .. " ~= " .. tostring(b))
end

-- JSON null must survive object and array decoding and encode back to null.
local obj = json.decode('{"a":null,"b":7}')
eq(obj.a, json.null, "object null")
eq(obj.b, 7, "object number")
eq(json.encode(obj), '{"a":null,"b":7}', "null re-encode")
local arr = json.decode('[1,null,3]')
eq(arr[1], 1, "array first")
eq(arr[2], json.null, "array null")
eq(arr[3], 3, "array third")

-- Lua strings are length-aware; embedded NUL must not be truncated on encode.
local binary = "a\0b"
local encoded = json.encode(binary)
eq(encoded, '"a\\u0000b"', "binary JSON encoding")

-- Cyclic tables must fail safely instead of recursing indefinitely.
local cyclic = {}
cyclic.self = cyclic
local cycle_ok = pcall(json.encode, cyclic)
eq(cycle_ok, false, "cyclic JSON table rejection")

-- Base64 empty input and canonical padding.
eq(base64.isvalid(""), true, "empty Base64 validity")
eq(base64.decode(""), "", "empty Base64 decode")
eq(base64.encode(""), "", "empty Base64 encode")
eq(base64.decode(base64.encode("a\0b")), "a\0b", "binary Base64 roundtrip")
eq(base64.isvalid("AB=="), false, "noncanonical padding bits")
eq(base64.isvalid("YQ=="), true, "canonical padding")

-- XML path parser must reject malformed predicates, not silently ignore them.
local root = xml.parse("<root><item id='1'/><item id='2'/></root>")
eq(#xml.path(root, "item[@id='2']"), 1, "XML valid predicate")
local ok = pcall(xml.path, root, "item[@id]")
eq(ok, false, "XML malformed predicate rejection")

print("regression tests: OK")
