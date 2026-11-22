--[[
  bench_base64.lua — speed test for core base64 module (Lua 5.1)
  Measures encode / decode throughput for several payload sizes.
]]

local function make_payload(n)
  -- deterministic binary-ish string of length n
  local t = {}
  for i = 1, n do
    t[i] = string.char((i * 17 + 31) % 256)
  end
  return table.concat(t)
end

local function time_it(label, niter, fn)
  -- warm-up
  for _ = 1, 3 do fn() end

  local t0 = os.clock()
  for _ = 1, niter do
    fn()
  end
  local dt = os.clock() - t0
  return dt
end

local sizes = {
  64,
  1024,          -- 1 KB
  16 * 1024,     -- 16 KB
  256 * 1024,    -- 256 KB
  1024 * 1024,   -- 1 MB
}

print(string.rep("=", 72))
print("base64 benchmark  |  Lua " .. _VERSION)
print(string.rep("=", 72))
print(string.format("%-10s %8s %10s %12s %12s %12s",
  "size", "iters", "op", "sec", "MB/s", "ops/s"))
print(string.rep("-", 72))

for _, size in ipairs(sizes) do
  local data = make_payload(size)
  local encoded = base64.encode(data)

  -- choose iterations so each case runs ~0.3–1s
  local niter
  if size <= 1024 then
    niter = 20000
  elseif size <= 16 * 1024 then
    niter = 2000
  elseif size <= 256 * 1024 then
    niter = 200
  else
    niter = 50
  end

  -- encode
  local dt_enc = time_it("encode", niter, function()
    base64.encode(data)
  end)
  local mb_enc = (size * niter) / (1024 * 1024) / dt_enc
  local ops_enc = niter / dt_enc

  print(string.format("%-10s %8d %10s %12.4f %12.1f %12.0f",
    size < 1024 and (size .. " B") or (string.format("%.0f KB", size / 1024)),
    niter, "encode", dt_enc, mb_enc, ops_enc))

  -- decode
  local dt_dec = time_it("decode", niter, function()
    base64.decode(encoded)
  end)
  local mb_dec = (size * niter) / (1024 * 1024) / dt_dec
  local ops_dec = niter / dt_dec

  print(string.format("%-10s %8d %10s %12.4f %12.1f %12.0f",
    size < 1024 and (size .. " B") or (string.format("%.0f KB", size / 1024)),
    niter, "decode", dt_dec, mb_dec, ops_dec))

  -- correctness check once
  local round = base64.decode(encoded)
  assert(round == data, "round-trip failed at size " .. size)

  print(string.rep("-", 72))
end

-- latency: many tiny encodes
do
  local tiny = "hello world!!"  -- 13 bytes
  local n = 100000
  local dt = time_it("tiny", n, function()
    base64.encode(tiny)
  end)
  print(string.format("tiny 13 B  %8d %10s %12.4f %12s %12.0f  (%.2f us/op)",
    n, "encode", dt, "-", n / dt, (dt / n) * 1e6))
end

print(string.rep("=", 72))
print("done.")