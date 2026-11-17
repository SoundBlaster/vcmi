#!/bin/sh
set -eu
mkdir -p /home/lobby/.local/share/vcmi /home/lobby/.cache/vcmi
chown -R lobby:lobby /home/lobby/.local /home/lobby/.cache
db=/home/lobby/.local/share/vcmi/vcmiLobby.db
if [ ! -e "$db" ]; then
  sqlite3 "$db" < /usr/share/vcmi/config/lobby.sql
  chown lobby:lobby "$db"
fi
cd /home/lobby
exec su -s /bin/sh lobby -c 'exec /usr/games/vcmilobby'
