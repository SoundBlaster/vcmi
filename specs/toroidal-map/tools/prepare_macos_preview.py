#!/usr/bin/env python3
"""Copy a local VCMI preview and make its client/server library paths local.

The source bundle must already contain the client, server and their non-system
libraries in Contents/Frameworks. This is a development helper, not a release
or notarization tool. It never modifies the source or overwrites an output.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path


def run(*args: str) -> str:
    return subprocess.run(args, check=True, text=True, capture_output=True).stdout


def dependencies(binary: Path) -> list[str]:
    output = run("otool", "-L", str(binary))
    return re.findall(r"^\s+(.*?) \(compatibility version ", output, re.MULTILINE)


def rpaths(binary: Path) -> list[str]:
    output = run("otool", "-l", str(binary))
    return re.findall(r"cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.*?) \(offset", output)


def require_bundled(binary: Path, dependency: str, path: Path, contents: Path) -> None:
    if not path.is_file() or not path.resolve().is_relative_to(contents.resolve()):
        raise RuntimeError(f"{binary.name}: missing or external bundled dependency {dependency}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="existing local app bundle")
    parser.add_argument("destination", type=Path, help="new app bundle; must not exist")
    parser.add_argument("--identity", required=True, help="codesign identity or - for ad hoc")
    args = parser.parse_args()
    source = args.source.resolve(strict=True)
    destination = args.destination.absolute()
    if destination.exists() or destination.is_symlink():
        parser.error("destination already exists")
    if not destination.parent.is_dir() or destination.parent.resolve().is_relative_to(source):
        parser.error("destination parent must exist outside the source bundle")

    contents = source / "Contents"
    frameworks = contents / "Frameworks"
    binaries = [contents / "MacOS" / name for name in ("vcmiclient", "vcmiserver")]
    libraries = sorted(frameworks.glob("*.dylib"))
    if not libraries or any(not binary.is_file() for binary in binaries):
        parser.error("source must contain client, server and bundled libraries")
    for binary in binaries + libraries:
        for dependency in dependencies(binary):
            if dependency.startswith("@rpath/"):
                require_bundled(binary, dependency, frameworks / dependency.removeprefix("@rpath/"), contents)
            elif dependency.startswith("@loader_path/"):
                require_bundled(binary, dependency, binary.parent / dependency.removeprefix("@loader_path/"), contents)
            elif dependency.startswith("@executable_path/"):
                require_bundled(binary, dependency, contents / "MacOS" / dependency.removeprefix("@executable_path/"), contents)
            elif dependency.startswith("@"):
                raise RuntimeError(f"{binary.name}: unsupported dependency path {dependency}")
            elif dependency.startswith("/") and not dependency.startswith(("/usr/lib/", "/System/Library/")):
                raise RuntimeError(f"{binary.name}: external absolute dependency {dependency}")

    run("ditto", str(source), str(destination))
    new_contents = destination / "Contents"
    new_frameworks = new_contents / "Frameworks"
    new_binaries = [new_contents / "MacOS" / binary.name for binary in binaries]
    new_libraries = [new_frameworks / library.name for library in libraries]
    for binary in new_binaries + new_libraries:
        paths = rpaths(binary)
        local = "@loader_path" if binary.parent == new_frameworks else "@executable_path/../Frameworks"
        command = ["install_name_tool"]
        for path in paths:
            if path.startswith("/") and not path.startswith(("/usr/lib/", "/System/Library/")):
                command.extend(["-delete_rpath", path])
        if local not in paths:
            command.extend(["-add_rpath", local])
        if len(command) > 1:
            run(*command, str(binary))

    for item in new_libraries + [new_contents / "MacOS" / "vcmiserver", destination]:
        run("codesign", "--force", "--sign", args.identity, str(item))
    run("codesign", "--verify", "--deep", "--strict", str(destination))
    print(f"Prepared and verified {destination}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as error:
        print(f"Preview preparation failed: {error}", file=sys.stderr)
        if error.stderr:
            print(error.stderr.strip(), file=sys.stderr)
        sys.exit(1)
    except (OSError, RuntimeError) as error:
        print(f"Preview preparation failed: {error}", file=sys.stderr)
        sys.exit(1)
