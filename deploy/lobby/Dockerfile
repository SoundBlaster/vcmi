FROM ubuntu:24.04
COPY vcmi-lobby.deb /tmp/vcmi-lobby.deb
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends /tmp/vcmi-lobby.deb sqlite3 ca-certificates && rm -rf /var/lib/apt/lists/* /tmp/vcmi-lobby.deb
COPY entrypoint.sh /usr/local/bin/lobby-entrypoint
RUN chmod 755 /usr/local/bin/lobby-entrypoint && useradd --create-home --uid 10001 lobby
EXPOSE 3031
ENTRYPOINT ["/usr/local/bin/lobby-entrypoint"]
