#!/usr/bin/env python3
"""Send one diagnostic GUI event; a missing reply leaves its outcome unknown."""
import argparse
import socket
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--socket", required=True, help="Private Unix socket selected at VCMI launch")
    sub = parser.add_subparsers(dest="kind", required=True)
    click = sub.add_parser("click", help="Click in logical VCMI GUI coordinates")
    click.add_argument("x", type=int)
    click.add_argument("y", type=int)
    shortcut = sub.add_parser("shortcut", help="Dispatch one allowlisted VCMI shortcut")
    shortcut.add_argument("name", choices=("cast", "return", "accept", "north", "south", "east", "west",
                                           "northeast", "northwest", "southeast", "southwest"))
    args = parser.parse_args()
    command = f"click {args.x} {args.y}\n" if args.kind == "click" else f"shortcut {args.name}\n"
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
            connection.settimeout(5)
            connection.connect(args.socket)
            connection.sendall(command.encode("ascii"))
            data = bytearray()
            while b"\n" not in data and len(data) < 256:
                chunk = connection.recv(256 - len(data))
                if not chunk:
                    break
                data.extend(chunk)
            response = data.decode("ascii").strip() if data.endswith(b"\n") else ""
    except (OSError, UnicodeError) as error:
        print(f"Command result unknown: {error}; do not retry without checking game state", file=sys.stderr)
        return 2
    print(response or "Command result unknown: connection closed without a response")
    return 0 if response == "accepted" else 1


if __name__ == "__main__":
    sys.exit(main())
