#!/usr/bin/env python3
"""Derive disposable GUI route fixtures from the local Flight/Water Walk map."""

import argparse
import json
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


def create_fixture(source, output, seam):
    with ZipFile(source) as archive:
        files = {name: archive.read(name) for name in archive.namelist()}

    terrain = json.loads(files["surface_terrain.json"])
    if len(terrain) != 36 or any(len(row) != 36 for row in terrain):
        raise ValueError("expected the 36 by 36 route-safety source map")
    if seam == "horizontal":
        rows, columns = range(14, 19), (33, 34, 2, 3, 4)
    elif seam == "vertical":
        rows, columns = (33, 34, 2, 3, 4), range(13, 18)
    else:
        rows, columns = range(20, 23), range(13, 18)
    for y in rows:
        for x in columns:
            if not terrain[y][x].startswith("wt"):
                raise ValueError(f"expected water at {x},{y}")
            terrain[y][x] = "gr49_"
    files["surface_terrain.json"] = json.dumps(terrain).encode()

    if seam:
        objects = json.loads(files["objects.json"])
        heroes = [obj for obj in objects if obj.get("instanceName") == "hero_2"]
        if len(heroes) != 1 or (heroes[0]["x"], heroes[0]["y"]) != (16, 16):
            raise ValueError("expected the original route-safety hero")
        if seam == "horizontal":
            heroes[0]["x"] = 35  # Object anchor; visitable tile (34, 16).
        else:
            heroes[0]["y"] = 34  # Visitable tile (15, 34).
        files["objects.json"] = json.dumps(objects).encode()

    texts = json.loads(files["russian.json"])
    name = {"horizontal": "Toroidal Water Seam Crossing Lab",
            "vertical": "Toroidal Water Vertical Crossing Lab"}.get(seam, "Toroidal Water Crossing Lab")
    texts["header.name"] = name
    texts["header.description"] = "Experimental wraparound map. AI support is not yet verified. Test map: two shores separated by water."
    files["russian.json"] = json.dumps(texts).encode()
    with ZipFile(output, "x", compression=ZIP_DEFLATED) as archive:
        for name, contents in files.items():
            archive.writestr(name, contents)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="local toroidal-route-safety-flight-waterwalk.vmap")
    parser.add_argument("output_dir", type=Path, help="existing directory for new maps")
    args = parser.parse_args()
    for filename, seam in (("toroidal-water-crossing-lab.vmap", None),
                           ("toroidal-water-seam-crossing-lab.vmap", "horizontal"),
                           ("toroidal-water-vertical-crossing-lab.vmap", "vertical")):
        create_fixture(args.source, args.output_dir / filename, seam)
        print(args.output_dir / filename)


if __name__ == "__main__":
    main()
