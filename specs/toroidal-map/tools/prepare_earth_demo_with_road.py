#!/usr/bin/env python3
"""Assemble the local Earth demo and extend its road through the seam."""

import argparse
import os
import subprocess
import tempfile
from pathlib import Path


def prepare(source, output, test_binary, profile):
    source = source.resolve(strict=True)
    test_binary = test_binary.resolve(strict=True)
    profile = profile.resolve(strict=True)
    output = output.absolute()
    if os.path.lexists(output):
        raise FileExistsError(f"refusing to overwrite {output}")
    if not test_binary.is_file() or not os.access(test_binary, os.X_OK):
        raise ValueError("test binary must be an executable file")
    if not (profile / "config/modSettings.json").is_file() or not (profile / "Mods").is_dir():
        raise ValueError("profile must have config/modSettings.json and Mods/")
    recipe = Path(__file__).resolve().with_name("prepare_earth_demo.sh")
    env = os.environ.copy()
    env.update(VCMI_TEST_MODDED_ROOT=str(profile), VCMI_USER_DATA_PATH=str(profile), PYTHONDONTWRITEBYTECODE="1")
    with tempfile.TemporaryDirectory(prefix=".earth-demo-", dir=output.parent) as temporary:
        base = Path(temporary) / "assembled.vmap"
        road = Path(temporary) / "road.vmap"
        subprocess.run(["sh", str(recipe), str(source), str(base)], check=True, env=env)
        env.update(VCMI_INSPECT_START_VMAP=str(base), VCMI_EARTH_SEAM_ROAD_OUTPUT=str(road))
        subprocess.run([str(test_binary), "--gtest_filter=MapFormat.PreparesEarthConnectedSeamRoad"],
                       cwd=test_binary.parent, check=True, env=env)
        if not road.is_file():
            raise RuntimeError("road generator did not produce a map")
        # Link on the same filesystem for atomic no-overwrite publication.
        os.link(road, output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="converted two-level source VMAP")
    parser.add_argument("output", type=Path, help="new output VMAP")
    parser.add_argument("test_binary", type=Path, help="vcmitest built from this branch")
    parser.add_argument("profile", type=Path, help="isolated VCMI profile with the required mods")
    args = parser.parse_args()
    prepare(args.source, args.output, args.test_binary, args.profile)
