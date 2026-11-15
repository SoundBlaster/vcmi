#!/usr/bin/env python3
"""Create a disposable local Earth road fixture with the red hero at the seam."""

import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

from analyze_stacked_map import load_json


def stage(source, output):
    expected = "f87b9ec936d3056a0aeaade05ece6bddd9ca0a24840ad1963c37047e2ad5c52b"
    with ZipFile(source) as archive:
        files = {name: archive.read(name) for name in archive.namelist()}
        digest = hashlib.sha256()
        for name in sorted(files):
            digest.update(name.encode("utf-8") + b"\0")
            digest.update(files[name])
        if digest.hexdigest() != expected:
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
