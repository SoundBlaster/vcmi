#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    printf 'Usage: %s SOURCE_IMAGE VCMITEST_BINARY NEW_WORK_DIR NEW_OUTPUT_VMAP\n' "$0" >&2
    printf 'PYTHON selects an interpreter with Pillow and NumPy (default: python3).\n' >&2
    exit 2
fi

source_image=$1
vcmitest=$2
work=$3
output=$4
python=${PYTHON:-python3}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ -e "$work" ] || [ -e "$output" ]; then
    printf 'Refusing to overwrite %s or %s\n' "$work" "$output" >&2
    exit 1
fi
mkdir -p "$work/coreroot/config" "$work/coreroot/Mods"
# the test runs from its own directory
work=$(CDPATH= cd -- "$work" && pwd)
output=$(CDPATH= cd -- "$(dirname -- "$output")" && pwd)/$(basename -- "$output")
vcmitest=$(CDPATH= cd -- "$(dirname -- "$vcmitest")" && pwd)/$(basename -- "$vcmitest")

# Only core content, so that the map does not depend on the test fixtures or installed mods
cat > "$work/coreroot/config/modSettings.json" <<'EOF'
{
	"activePreset" : "default",
	"presets" : { "default" : { "mods" : [ "vcmi", "core" ] } }
}
EOF

"$python" -I "$script_dir/world_map_classify.py" "$source_image" "$work/classes.npy" "$work/classes.png"
"$python" -I "$script_dir/world_map_refine.py" "$work/classes.npy" "$work/grid.txt" "$work/grid.png"
"$python" -I "$script_dir/world_map_masks.py" "$source_image" "$work/grid.txt" "$work/obstacles.txt" "$work/obstacles.png"

cd "$(dirname -- "$vcmitest")"
VCMI_TEST_MODDED_ROOT="$work/coreroot" \
VCMI_WORLD_GRID="$work/grid.txt" \
VCMI_WORLD_OBSTACLES="$work/obstacles.txt" \
VCMI_WORLD_SITES="$script_dir/world_map_sites.txt" \
VCMI_WORLD_OUTPUT="$output" \
VCMI_WORLD_NAME="${VCMI_WORLD_NAME:-Toroidal World}" \
VCMI_WORLD_DESCRIPTION="${VCMI_WORLD_DESCRIPTION:-Eight nations share a world whose edges wrap around: sail west from the Americas to reach Asia, or north past the Arctic to reach Antarctica.}" \
    "$vcmitest" --gtest_filter='MapManager.PaintWorldMapProbe'
