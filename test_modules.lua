-- Quick smoke test for core json / xml / base64 modules
print("Lua version:", _VERSION)

print("\n=== base64 ===")
local s = "Hello, Lua 5.1!"
local e = base64.encode(s)
print("encode:", e)
print("decode:", base64.decode(e))
print("isvalid:", base64.isvalid(e))

print("\n=== json ===")
local t = { name = "test", nums = {1, 2, 3}, ok = true }
local j = json.encode(t)
print("encode:", j)
print("pretty:\n" .. json.encode_pretty(t))
local t2 = json.decode(j)
print("decode name:", t2.name, "nums[2]:", t2.nums[2])
local ok, err = json.decode_safe("{ bad json")
print("decode_safe:", ok, err)

print("\n=== xml ===")
local root = xml.parse([[
<root id="r1">
  <item id="1">A</item>
  <item id="2">B</item>
</root>
]])
print("root name:", root.name)
print("root attrs.id:", root.attrs and root.attrs.id)
local nodes = xml.path(root, "item[@id=2]")
if nodes[1] then
  print("path item[@id=2] content:", nodes[1].content)
else
  print("path: no match")
end

print("\nAll tests done.")
