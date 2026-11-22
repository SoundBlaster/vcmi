#!/bin/sh
# Compiles vcmimapbuilder inside ubuntu:24.04 and installs it into /out.
# Run it in a container with limited CPU and memory: the build shares the host with other services.
set -eu
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
	cmake ninja-build g++ libboost-dev libboost-filesystem-dev libboost-program-options-dev \
	libtbb-dev libluajit-5.1-dev zlib1g-dev libminizip-dev
cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
	-DENABLE_CLIENT=OFF -DENABLE_SERVER=OFF -DENABLE_LAUNCHER=OFF -DENABLE_EDITOR=OFF -DENABLE_LOBBY=OFF \
	-DENABLE_TEST=OFF -DENABLE_TRANSLATIONS=OFF -DENABLE_DISCORD=OFF -DENABLE_GITVERSION=OFF -DENABLE_MAPBUILDER=ON \
	-DENABLE_NULLKILLER2_AI=OFF -DENABLE_STUPID_AI=OFF -DENABLE_BATTLE_AI=OFF -DENABLE_MMAI=OFF
cmake --build /build -j "${JOBS:-1}"
rm -rf /out/*
DESTDIR=/out cmake --install /build
