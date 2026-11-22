"""Copies a LOD archive keeping only the text tables and object masks; every other entry stays listed with zero length."""
import struct, sys, zlib
src, dst = sys.argv[1], sys.argv[2]
with open(src, 'rb') as f:
    data = f.read()
if data[:4] != b'LOD\0':
    raise SystemExit('not a LOD archive: ' + src)
version, count = struct.unpack_from('<II', data, 4)
entries = []
for i in range(count):
    off = 92 + i * 32
    name = data[off:off + 16]
    start, size, kind, csize = struct.unpack_from('<IIII', data, off + 16)
    text = name.split(b'\0')[0].decode('ascii').upper().endswith(('.TXT', '.MSK'))
    body = b''
    if text:
        raw = data[start:start + (csize or size)]
        body = zlib.decompress(raw) if csize else raw
    entries.append((name, kind, body))
header_size = 92 + 32 * count
out = bytearray(data[:92])
table = bytearray()
blob = bytearray()
for name, kind, body in entries:
    table += name + struct.pack('<IIII', header_size + len(blob), len(body), kind, 0)
    blob += body
with open(dst, 'wb') as f:
    f.write(out + table + blob)
print(dst, count, 'entries', len(blob), 'bytes kept')
