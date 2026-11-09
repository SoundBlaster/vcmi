#!/usr/bin/env python3
"""List tracked C++ lines that may depend on adventure-map topology.

Run from any directory. Output is JSON Lines by default; --summary prints
counts. Matches are candidates, not proof of a defect or complete coverage.
"""

import argparse
import collections
import json
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[3]
SCOPES = ('lib', 'client', 'server', 'AI', 'mapeditor')
SUFFIXES = ('.cpp', '.h', '.hpp', '.cc', '.cxx')
PATTERNS = {
    'distance_or_neighbor': re.compile(
        r'\b(?:\w*(?:dist(?:ance|2d|2dsq)|neighbou?r|adjacen)\w*|'
        r'dist|getClosestTile)\s*\(', re.I),
    'boundary_check': re.compile(
        r'\b(?:isInTheMap|isInMap|isOnMap|isValidPosition|'
        r'isValidTile|withinBounds|outOfBounds)\s*\(', re.I),
    'coordinate_arithmetic': re.compile(
        r'(?:\.(?:x|y)\s*[-+*/%]|[-+*/%]\s*\w+\.(?:x|y)\b|'
        r'\b\w*(?:pos|position|coord|tile|neighbou?r|origin|dest)\w*\s*[-+]=?\s*\w+)', re.I),
    'coordinate_math': re.compile(
        r'\b(?:abs|sqrt|hypot|pow)\s*\([^;]*(?:\.(?:x|y)\b|'
        r'\b\w*(?:pos|coord|tile|dist)\w*)', re.I),
    'extent_or_border': re.compile(
        r'\b(?:width|height|mapWidth|mapHeight|mapSize|mapSizes|'
        r'BORDER_WIDTH)\b\s*(?:[-+*/%<>=]|\))|'
        r'(?:[-+*/%<>=])\s*\b(?:width|height|mapWidth|mapHeight|'
        r'mapSize|mapSizes|BORDER_WIDTH)\b', re.I),
}


def tracked_sources():
    output = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '-z', '--', *SCOPES])
    for raw in output.split(b'\0'):
        if raw:
            name = raw.decode('utf-8')
            if name.endswith(SUFFIXES):
                yield name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--summary', action='store_true')
    parser.add_argument('--category', choices=sorted(PATTERNS))
    parser.add_argument('--path', help='restrict results to a path prefix')
    parser.add_argument('--markdown-index', action='store_true')
    args = parser.parse_args()
    totals = collections.Counter()
    modules = collections.Counter()
    files = set()
    file_matches = collections.defaultdict(list)
    count = 0
    for name in tracked_sources():
        if args.path and not name.startswith(args.path):
            continue
        for line_number, line in enumerate((ROOT / name).read_text(encoding='utf-8', errors='replace').splitlines(), 1):
            matches = [category for category, pattern in PATTERNS.items() if pattern.search(line)]
            if args.category and args.category not in matches:
                continue
            if not matches:
                continue
            count += 1
            files.add(name)
            totals.update(matches)
            modules.update([name.split('/', 1)[0]])
            file_matches[name].append(line_number)
            if not args.summary and not args.markdown_index:
                print(json.dumps({'path': name, 'line': line_number, 'categories': matches,
                                  'text': line.strip()}, ensure_ascii=False))
    if args.summary:
        print(json.dumps({'matched_lines': count, 'matched_files': len(files),
                          'categories': dict(sorted(totals.items())),
                          'modules': dict(sorted(modules.items()))}, indent=2))
    elif args.markdown_index:
        baseline = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', '--short=12', 'HEAD'], text=True).strip()
        print('# Coordinate audit candidate index\n')
        print(f'Generated from tracked C++ at `{baseline}`. Each row is a search candidate,')
        print('not a confirmed defect. Line numbers can shift after this baseline.\n')
        print('Use `python3 specs/toroidal-map/tools/audit_coordinates.py --path <path>`')
        print('to regenerate full matching lines. Review status covers this snapshot only;')
        print('regenerating the index resets manually recorded statuses.\n')
        print('| File | Matching lines | Review |')
        print('| --- | --- | --- |')
        for name, lines in sorted(file_matches.items()):
            print(f'| `{name}` | {", ".join(map(str, lines))} | unreviewed |')


if __name__ == '__main__':
    main()
