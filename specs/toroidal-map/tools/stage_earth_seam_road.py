#!/usr/bin/env python3
"""Create a disposable local Earth road fixture with the red hero at the seam."""

import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

from analyze_stacked_map import load_json


def stage(source, output):
    expected = {
        "f87b9ec936d3056a0aeaade05ece6bddd9ca0a24840ad1963c37047e2ad5c52b",
        # Integrated Earth draft with a guarded route from the starting area.
        "20c6b4c4abb50a2f1d20e0f39053d90c47680e01654711b2fb24901453ef11b1",
        # Same draft with an accessible approach to monolithTwoWay_11148.
        "d26eea5dc3c80036502ba23401b5c29a7dc9b7b75bb205c080cb1d4d4305f89a",
        # Same draft with the misplaced whirlpool moved onto water.
        "ce72175e7a609c19df65ba3c766ae4208218a8fd8066f8b40c0f387ce6483c66",
        # Same draft with the reef removed from the gem mine approach.
        "0f7a01877b14cf0a1d78237c38b2df868138a8a3c632b47900b893e19e93515d",
    }
    with ZipFile(source) as archive:
        files = {name: archive.read(name) for name in archive.namelist()}
        digest = hashlib.sha256()
        for name in sorted(files):
            digest.update(name.encode("utf-8") + b"\0")
            digest.update(files[name])
        if digest.hexdigest() not in expected:
            raise ValueError("unexpected Earth seam-road input")
        objects = load_json(archive, "objects.json")
        texts = load_json(archive, "russian.json")
    heroes = [obj for obj in objects if obj.get("instanceName") == "randomHero_7305"]
    if len(heroes) != 1 or (heroes[0]["x"], heroes[0]["y"], heroes[0]["l"]) != (165, 353, 0):
        raise ValueError("unexpected red hero placement")
    heroes[0]["x"], heroes[0]["y"] = 1, 425
    texts["header.name"] = "Earth Seam Road QA"
    files["objects.json"] = json.dumps(objects, ensure_ascii=False).encode("utf-8")
    files["russian.json"] = json.dumps(texts, ensure_ascii=False).encode("utf-8")
    with ZipFile(output, "x", compression=ZIP_DEFLATED) as archive:
        for name, content in files.items():
            archive.writestr(name, content)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    stage(args.source, args.output)
