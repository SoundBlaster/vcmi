"""HTTP front of vcmimapbuilder: takes a terrain grid from the terrain painter page and returns the built .vmap."""

import argparse
import base64
import collections
import ipaddress
import json
import logging
import os
import subprocess
import tempfile
import threading
import time
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

MAX_BODY = 2 * 1024 * 1024
BUILD_TIMEOUT = 60
QUEUE_TIMEOUT = 20
RATE_WINDOW = 600
RATE_LIMIT = 30
MAX_CLIENTS = 4096

builds = threading.Semaphore(1)
history = collections.OrderedDict()
history_lock = threading.Lock()


def allowed(client):
    now = time.monotonic()
    with history_lock:
        # Entries are ordered by last accepted request. Expire old client keys
        # globally and refuse new ones at capacity rather than growing forever.
        while history:
            oldest, requests = next(iter(history.items()))
            if now - requests[-1] <= RATE_WINDOW:
                break
            del history[oldest]
        recent = history.get(client)
        if recent is None:
            if len(history) >= MAX_CLIENTS:
                return False
            recent = history[client] = collections.deque()
        while recent and now - recent[0] > RATE_WINDOW:
            recent.popleft()
        if len(recent) >= RATE_LIMIT:
            return False
        recent.append(now)
        history.move_to_end(client)
        return True


class Handler(SimpleHTTPRequestHandler):
    builder = None
    static_root = None
    server_version = "vcmimapbuilder"

    def client(self):
        # Only nginx can reach this service; nginx is reachable via trusted
        # Caddy, which replaces untrusted incoming forwarding headers.
        forwarded = self.headers.get("X-Real-IP", "").split(",")[0].strip()
        try:
            return str(ipaddress.ip_address(forwarded))
        except ValueError:
            return self.client_address[0]

    def reply(self, status, payload):
        body = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.rstrip("/").endswith("/api/health"):
            self.reply(HTTPStatus.OK, {"status": "ok"})
        elif self.static_root:
            super().do_GET()
        else:
            self.reply(HTTPStatus.NOT_FOUND, {"error": "Not found"})

    def do_POST(self):
        if not self.path.rstrip("/").endswith("/api/map"):
            self.reply(HTTPStatus.NOT_FOUND, {"error": "Not found"})
            return
        try:
            length = int(self.headers.get("Content-Length", ""))
        except ValueError:
            self.reply(HTTPStatus.LENGTH_REQUIRED, {"error": "Content-Length is required"})
            return
        if length <= 0 or length > MAX_BODY:
            self.reply(HTTPStatus.REQUEST_ENTITY_TOO_LARGE, {"error": "The request is too large"})
            return
        if not allowed(self.client()):
            self.reply(HTTPStatus.TOO_MANY_REQUESTS, {"error": "Too many maps in a short time, try again later"})
            return
        body = self.rfile.read(length)
        if not builds.acquire(timeout=QUEUE_TIMEOUT):
            self.reply(HTTPStatus.SERVICE_UNAVAILABLE, {"error": "The server is busy, try again in a minute"})
            return
        try:
            self.build(body)
        finally:
            builds.release()

    def build(self, body):
        with tempfile.TemporaryDirectory(prefix="mapbuild-") as work:
            request = os.path.join(work, "request.json")
            output = os.path.join(work, "map.vmap")
            report = os.path.join(work, "report.json")
            with open(request, "wb") as f:
                f.write(body)
            started = time.monotonic()
            try:
                result = subprocess.run([self.builder, request, output, report], cwd=work, capture_output=True, timeout=BUILD_TIMEOUT)
            except subprocess.TimeoutExpired:
                logging.warning("build timed out for %s", self.client())
                self.reply(HTTPStatus.GATEWAY_TIMEOUT, {"error": "Building took too long"})
                return
            if result.returncode == 2:
                lines = [line for line in result.stderr.decode(errors="replace").splitlines() if line.startswith("Error: ")]
                message = lines[-1][len("Error: "):] if lines else "Invalid request"
                self.reply(HTTPStatus.BAD_REQUEST, {"error": message})
                return
            if result.returncode != 0:
                logging.error("builder exited with %s: %s", result.returncode, result.stderr.decode(errors="replace")[-2000:])
                self.reply(HTTPStatus.INTERNAL_SERVER_ERROR, {"error": "The map builder failed"})
                return
            with open(report, encoding="utf-8") as f:
                report_data = json.load(f)
            with open(output, "rb") as f:
                map_data = base64.b64encode(f.read()).decode()
            logging.info("built %sx%s for %s in %.2fs", report_data["width"], report_data["height"], self.client(), time.monotonic() - started)
            self.reply(HTTPStatus.OK, {"report": report_data, "map": map_data})

    def log_message(self, format, *args):
        logging.debug("%s %s", self.address_string(), format % args)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--builder", required=True, help="path to vcmimapbuilder")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--static", help="also serve this directory, for local testing without the website")
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    Handler.builder = os.path.abspath(args.builder)
    Handler.static_root = os.path.abspath(args.static) if args.static else None

    # SimpleHTTPRequestHandler falls back to the working directory, so files are served only with --static
    def handler(*handler_args, **kwargs):
        return Handler(*handler_args, directory=Handler.static_root or tempfile.gettempdir(), **kwargs)

    server = ThreadingHTTPServer(("0.0.0.0", args.port), handler)
    logging.info("listening on %s", args.port)
    server.serve_forever()


if __name__ == "__main__":
    main()
