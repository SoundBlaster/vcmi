#!/usr/bin/env python3
"""Mark the tiles of a terrain grid that the map generator fills with obstacles.

Usage: world_map_masks.py SOURCE_IMAGE GRID_TXT OUTPUT_MASK_TXT PREVIEW_PNG
Rough tiles stand for mountains and dark green picture areas for forests;
a smooth random field leaves gaps between the clumps. 'x' marks a tile.
"""

import sys
from PIL import Image, ImageFilter
import numpy as np
SRC, GRID, OUT, PNG = sys.argv[1:5]
W, H, BORDER = 256, 128, 8
img = Image.open(SRC).convert('RGB')
img = img.crop((BORDER, BORDER, img.width - BORDER, img.height - BORDER)).filter(ImageFilter.MedianFilter(3))
a = np.asarray(img).astype(float); r, g, b = a[..., 0], a[..., 1], a[..., 2]
mx = a.max(axis=2)
forest = (g > r + 8) & (g > b + 4) & (mx < 115)
ph, pw = forest.shape
rows = [l.strip() for l in open(GRID)]
frac = np.zeros((H, W))
for y in range(H):
    for x in range(W):
        frac[y, x] = forest[int(y*ph/H):int((y+1)*ph/H), int(x*pw/W):int((x+1)*pw/W)].mean()
# smooth random field: gaps between clumps keep the land passable
rng = np.random.default_rng(7)
noise = rng.random((H // 4, W // 4))
field = np.asarray(Image.fromarray((noise * 255).astype(np.uint8)).resize((W, H), Image.BICUBIC)).astype(float) / 255
mask = np.zeros((H, W), dtype=bool)
for y in range(H):
    for x in range(W):
        t = rows[y][x]
        if t == 'w':
            continue
        if t == 'r' and field[y, x] > 0.30:
            mask[y, x] = True
        elif t == 'g' and frac[y, x] > 0.35 and field[y, x] > 0.40:
            mask[y, x] = True
        elif t == 'n' and field[y, x] > 0.62:
            mask[y, x] = True
        elif t in 'ds' and field[y, x] > 0.80:
            mask[y, x] = True
with open(OUT, 'w') as f:
    for row in mask:
        f.write(''.join('x' if v else '.' for v in row) + '\n')
pal = {'w': (15, 50, 110), 'g': (60, 140, 40), 's': (220, 190, 110), 'd': (130, 90, 50), 'n': (240, 240, 250), 'r': (120, 110, 100), 'm': (60, 90, 70)}
pic = np.array([[pal[c] for c in row] for row in rows], dtype=np.uint8)
pic[mask] = (pic[mask] * 0.45).astype(np.uint8)
Image.fromarray(pic).resize((W * 4, H * 4), Image.NEAREST).save(PNG)
print('obstacle tiles', int(mask.sum()), 'land', sum(c != 'w' for row in rows for c in row))
