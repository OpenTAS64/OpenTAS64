#!/usr/bin/env python3
"""Build a Windows .ico resource from the BMP embedded in AppIcon.h."""
import base64
import re
import struct
import sys
from pathlib import Path

header_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
source = header_path.read_text(encoding="utf-8")
match = re.search(r'encoded\[\]\s*=\s*"([A-Za-z0-9+/=]+)"', source)
if not match:
    raise SystemExit("Could not find the embedded OpenTAS64 icon bitmap.")

bmp = base64.b64decode(match.group(1))
if bmp[:2] != b"BM":
    raise SystemExit("Embedded icon data is not a BMP file.")

pixel_offset = struct.unpack_from("<I", bmp, 10)[0]
dib_size = struct.unpack_from("<I", bmp, 14)[0]
width, height, planes, bits = struct.unpack_from("<iiHH", bmp, 18)
if dib_size < 40 or width <= 0 or height <= 0 or bits not in (24, 32):
    raise SystemExit("Unsupported embedded BMP format.")

# ICO DIBs store the combined XOR + AND mask height in the header.
dib = bytearray(bmp[14:14 + dib_size])
struct.pack_into("<i", dib, 8, height * 2)
xor_bitmap = bmp[pixel_offset:]
mask_stride = ((width + 31) // 32) * 4
and_mask = bytes(mask_stride * height)
image = bytes(dib) + xor_bitmap + and_mask

directory = struct.pack("<HHH", 0, 1, 1)
entry = struct.pack(
    "<BBBBHHII",
    0 if width >= 256 else width,
    0 if height >= 256 else height,
    0, 0, 1, bits, len(image), 22
)
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_bytes(directory + entry + image)
