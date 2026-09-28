#!/usr/bin/env python3
"""Amiga Moria 1.2 graphics for Umoria (read by port/mkamiga.py).

Amiga Moria 1.2 (Henrik Harmsen, 1992; UMoria 5.5 with bitmapped graphics,
GPL-3: https://github.com/suncore/Amiga-Moria) draws every map symbol from one
320x56 sheet of 40x7 cells of 8x8 (moria_gfx.iff; amiga.c copies one byte per
bitplane row). amiga_corrlist.c maps a symbol code (ASCII, or 128-255 for its
own object pictures) to a cell (column, row); its
treasure.c gives each object its code (daggers 196, swords 197-199, ...).
This downloads the release, decodes the sheet and writes:
  port/amiga/moria_gfx.png  the sheet, as drawn (16 colours)
  port/amiga/amiga.tsv      'gfx' code row col / 'obj' name code
Run from anywhere (needs bsdtar for the .lha): python3 port/amiga/extract.py
"""
import io
import os
import re
import struct
import subprocess
import tarfile
import tempfile
import urllib.request
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
URL = 'https://raw.githubusercontent.com/suncore/Amiga-Moria/main/'


def ilbm(data):
    """Decode an IFF ILBM (ByteRun1) to an RGB image."""
    assert data[:4] == b'FORM' and data[8:12] == b'ILBM'
    i, ch = 12, {}
    while i < len(data) - 8:
        t, n = data[i:i + 4], struct.unpack('>I', data[i + 4:i + 8])[0]
        ch[t] = data[i + 8:i + 8 + n]
        i += 8 + n + (n & 1)
    w, h, _, _, planes, mask, comp = struct.unpack('>HHhhBBB', ch[b'BMHD'][:11])
    pal = [tuple(ch[b'CMAP'][j:j + 3]) for j in range(0, len(ch[b'CMAP']), 3)]
    body, row = ch[b'BODY'], ((w + 15) // 16) * 2
    if comp == 1:
        out, k = bytearray(), 0
        while k < len(body):
            c = body[k]
            k += 1
            if c < 128:
                out += body[k:k + c + 1]
                k += c + 1
            elif c > 128:
                out += bytes([body[k]]) * (257 - c)
                k += 1
        body = bytes(out)
    n = planes + (mask == 1)
    im = Image.new('RGB', (w, h))
    for y in range(h):
        rows = [body[(y * n + p) * row:(y * n + p + 1) * row] for p in range(planes)]
        for x in range(w):
            v = sum(((rows[p][x >> 3] >> (7 - (x & 7))) & 1) << p for p in range(planes))
            im.putpixel((x, y), pal[v])
    return im


src = tarfile.open(fileobj=io.BytesIO(urllib.request.urlopen(URL + 'Moria_gfx_src_1.2.tar.gz').read()))
text = {os.path.basename(m.name): src.extractfile(m).read().decode('latin-1')
        for m in src.getmembers() if m.name.endswith(('amiga_corrlist.c', 'treasure.c'))}
with tempfile.TemporaryDirectory() as tmp:
    lha = os.path.join(tmp, 'moria.lha')
    with open(lha, 'wb') as f:
        f.write(urllib.request.urlopen(URL + 'Moria_gfx_1.2.lha').read())
    subprocess.run(['bsdtar', 'xf', lha, '-C', tmp, 'Moria/moria_gfx.iff'], check=True)
    ilbm(open(os.path.join(tmp, 'Moria/moria_gfx.iff'), 'rb').read()).save(os.path.join(HERE, 'moria_gfx.png'))


def code(k):
    k = k.strip()
    return ord(k[1:-1].encode().decode('unicode_escape')) if k.startswith("'") else int(k)


corr = text['amiga_corrlist.c']
ys = {code(k): int(v) for k, v in re.findall(r"GFX_CORR\[y\]\[([^\]]+)\]\s*=\s*(\d+)", corr)}
xs = {code(k): int(v) for k, v in re.findall(r"GFX_CORR\[x\]\[([^\]]+)\]\s*=\s*(\d+)", corr)}
objs = re.findall(r'^\{"([^"]*)"\s*,\s*0x[0-9A-Fa-f]+L,\s*TV_\w+,\s*(\'\\?.\'|\d+)',
                  text['treasure.c'], re.M)
with open(os.path.join(HERE, 'amiga.tsv'), 'w') as f:
    f.write('# Amiga Moria 1.2 (port/amiga/extract.py): gfx code row col | obj name code\n')
    for k in sorted(xs):
        f.write('gfx\t%d\t%d\t%d\n' % (k, ys[k], xs[k]))
    for n, c in objs:
        f.write('obj\t%s\t%d\n' % (n, code(c)))
