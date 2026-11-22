#!/usr/bin/env python3
"""Smooth a classified terrain grid and write it as terrain letters.

Usage: world_map_refine.py GRID_NPY OUTPUT_GRID_TXT PREVIEW_PNG
Letters: w water, g grass, s sand, d dirt, n snow, r rough, m swamp.
The grid wraps on both axes, like the toroidal map built from it.
"""

import sys
import numpy as np
from PIL import Image
from collections import deque

codes = np.load(sys.argv[1])
H, W = codes.shape
WATER = 0

def majority(grid, classes, mask=None):
    counts = np.zeros((classes,) + grid.shape, dtype=int)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            shifted = np.roll(np.roll(grid, dy, 0), dx, 1)
            for c in range(classes):
                counts[c] += (shifted == c)
    return counts

# land/water: majority vote, wrapping on both axes
for _ in range(2):
    land = (codes != WATER).astype(int)
    votes = majority(land, 2)
    newLand = np.where(votes[1] >= 5, 1, np.where(votes[0] >= 6, 0, land))
    codes = np.where((newLand == 1) & (codes == WATER), 3, codes)  # new land becomes dirt for now
    codes = np.where(newLand == 0, WATER, codes)

# drop land components smaller than 4 tiles (ships, lighthouses and other sprites)
def components(mask):
    seen = np.zeros_like(mask, dtype=bool)
    result = []
    for y in range(H):
        for x in range(W):
            if mask[y, x] and not seen[y, x]:
                comp = []
                q = deque([(y, x)]); seen[y, x] = True
                while q:
                    cy, cx = q.popleft(); comp.append((cy, cx))
                    for dy, dx in ((1,0),(-1,0),(0,1),(0,-1)):
                        ny, nx = (cy+dy) % H, (cx+dx) % W
                        if mask[ny, nx] and not seen[ny, nx]:
                            seen[ny, nx] = True; q.append((ny, nx))
                result.append(comp)
    return result
for comp in components(codes != WATER):
    if len(comp) < 4:
        for y, x in comp: codes[y, x] = WATER
for comp in components(codes == WATER):
    if len(comp) < 3:
        for y, x in comp: codes[y, x] = 3

# land type: majority among land neighbours, twice
for _ in range(2):
    votes = majority(codes, 7)
    votes[WATER] = 0
    best = votes.argmax(axis=0)
    strong = votes.max(axis=0) >= 4
    codes = np.where((codes != WATER) & strong, best, codes)

# polar land is snow, apart from the mountains
SNOW = 4
for y in list(range(10)) + list(range(H - 12, H)):
    codes[y] = np.where(np.isin(codes[y], (1, 3)), SNOW, codes[y])

letters = 'wgsdnrm'
with open(sys.argv[2], 'w') as f:
    for row in codes:
        f.write(''.join(letters[c] for c in row) + '\n')
palette = np.array([(15, 50, 110), (60, 140, 40), (220, 190, 110), (130, 90, 50), (240, 240, 250), (120, 110, 100), (60, 90, 70)], dtype=np.uint8)
Image.fromarray(palette[codes]).resize((W * 4, H * 4), Image.NEAREST).save(sys.argv[3])
print({n: int((codes == i).sum()) for i, n in enumerate(['water', 'grass', 'sand', 'dirt', 'snow', 'rough', 'swamp'])})
