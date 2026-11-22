#!/usr/bin/env python3
"""Classify a framed world picture into a 256x128 grid of terrain classes.

Usage: world_map_classify.py SOURCE_IMAGE OUTPUT_NPY PREVIEW_PNG
"""

import sys
from PIL import Image, ImageFilter
import numpy as np

SRC, OUT_GRID, OUT_PNG = sys.argv[1], sys.argv[2], sys.argv[3]
W, H = 256, 128
BORDER = 8

img = Image.open(SRC).convert('RGB')
img = img.crop((BORDER, BORDER, img.width - BORDER, img.height - BORDER))
# smooth away sprite detail before per-pixel classification
img = img.filter(ImageFilter.MedianFilter(5))
a = np.asarray(img).astype(float)
r, g, b = a[..., 0], a[..., 1], a[..., 2]
mx = a.max(axis=2)
mn = a.min(axis=2)
sat = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1), 0)

WATER, GRASS, SAND, DIRT, SNOW, ROUGH, SWAMP = range(7)
cls = np.full(r.shape, DIRT)
water = (b > r + 25) & (b > g + 5) & (mx < 200)
snow = (mn > 165) & (sat < 0.22)
green = (g > r + 4) & (g > b + 4)
sand = (r > 170) & (g > 130) & (b < 140) & (r > b + 60) & ~green
rough = (sat < 0.35) & (mx < 170) & ~water & ~green
cls[rough] = ROUGH
cls[sand] = SAND
cls[green] = GRASS
cls[snow] = SNOW
cls[water] = WATER

ph, pw = cls.shape
codes = np.zeros((H, W), dtype=int)
for y in range(H):
    y0, y1 = int(y * ph / H), int((y + 1) * ph / H)
    for x in range(W):
        x0, x1 = int(x * pw / W), int((x + 1) * pw / W)
        cell = cls[y0:y1, x0:x1].ravel()
        counts = np.bincount(cell, minlength=7)
        if counts[WATER] >= cell.size * 0.5:
            codes[y, x] = WATER
        else:
            counts[WATER] = 0
            codes[y, x] = counts.argmax()

np.save(OUT_GRID, codes)
palette = np.array([(15, 50, 110), (60, 140, 40), (220, 190, 110), (130, 90, 50), (240, 240, 250), (120, 110, 100), (60, 90, 70)], dtype=np.uint8)
Image.fromarray(palette[codes]).resize((W * 4, H * 4), Image.NEAREST).save(OUT_PNG)
print({n: int((codes == i).sum()) for i, n in enumerate(['water', 'grass', 'sand', 'dirt', 'snow', 'rough', 'swamp'])})
