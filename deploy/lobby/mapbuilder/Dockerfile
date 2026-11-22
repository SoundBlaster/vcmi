FROM ubuntu:24.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
	python3 libboost-filesystem1.83.0 libboost-program-options1.83.0 libtbb12 libluajit-5.1-2 libminizip1t64 zlib1g \
	&& rm -rf /var/lib/apt/lists/*
COPY out/ /
COPY server.py /opt/mapbuilder/server.py
RUN useradd --create-home --uid 10002 builder \
	&& mkdir -p /home/builder/.config/vcmi /home/builder/.local/share/vcmi \
	&& printf '{ "activePreset" : "default", "presets" : { "default" : { "mods" : [ "vcmi", "core" ] } } }\n' > /home/builder/.config/vcmi/modSettings.json \
	&& chown -R builder /home/builder
USER builder
ENV XDG_DATA_HOME=/home/builder/.local/share XDG_CONFIG_HOME=/home/builder/.config XDG_CACHE_HOME=/tmp
EXPOSE 8080
CMD ["python3", "-I", "/opt/mapbuilder/server.py", "--builder", "/usr/bin/vcmimapbuilder", "--port", "8080"]
