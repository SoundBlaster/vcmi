#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    printf 'Usage: %s SOURCE_TWO_LEVEL_VMAP NEW_OUTPUT_VMAP\n' "$0" >&2
    exit 2
fi

# This recipe addresses specific objects in one converted map revision.
expected_sha256=69e1f1aff0398a7042e7b158b1ac638ae3237b23d0a5a8927f0afcb7ef0c4023
actual_sha256=$(python3 -B -c 'import hashlib, sys; print(hashlib.sha256(open(sys.argv[1], "rb").read()).hexdigest())' "$1")
if [ "$actual_sha256" != "$expected_sha256" ]; then
    printf 'Unexpected source VMAP SHA-256: %s\nExpected: %s\n' "$actual_sha256" "$expected_sha256" >&2
    exit 1
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec python3 "$script_dir/assemble_stacked_draft.py" "$1" "$2" \
    --rejoin-east-fragment 251 130 \
    --polar-seed 136 21 \
    --oceanize-edge-rock \
    --illustrative-snow-island 220 275 \
    --gate-pair subterraneanGate_5430 subterraneanGate_10528 monolith2 \
    --gate-pair subterraneanGate_5431 subterraneanGate_10533 monolith4 \
    --relocate-mountain mountain_3335 \
    --relocate-mountain mountain_7529 \
    --repair-mountain-interactions \
    --repair-mine-guards \
    --repair-other-guards \
    --relocate-obstacle mountain_8016 \
    --relocate-obstacle mountain_8064 \
    --relocate-obstacle spruces_9932 \
    --relocate-obstacle spruces_10139 \
    --reserve-tile 88 209 \
    --reserve-tile 111 186 \
    --reserve-tile 41 74 \
    --reserve-tile 39 110 \
    --require-mod hota \
    --require-mod hota.artifacts \
    --require-mod hota.bulwark \
    --require-mod hota.cannon \
    --require-mod hota.cove \
    --require-mod hota.factory \
    --require-mod hota.heroes \
    --require-mod hota.highlandsterrain \
    --require-mod hota.interference \
    --require-mod hota.mapdecorations \
    --require-mod hota.mapobjects \
    --require-mod hota.neutralcreatures \
    --require-mod hota.neutralcreatures.ancientaltar \
    --require-mod hota.terrainoverlays \
    --require-mod hota.wastelandterrain \
    --relocate-water-monolith monolithTwoWay_5267 240 429
