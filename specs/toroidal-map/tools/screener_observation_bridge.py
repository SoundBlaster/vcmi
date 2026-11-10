#!/usr/bin/env python3
"""Capture Screener window checkpoints for new VCMI observation records."""
import argparse
import json
import os
import select
import signal
import subprocess
import sys
import time


class ObservationTail:
    def __init__(self, path):
        self.path = path
        self.file = open(path, "rb")
        self.file.seek(0, os.SEEK_END)
        self.identity = os.fstat(self.file.fileno())
        self.pending = b""
        self.last = None

    def close(self):
        self.file.close()

    def read(self):
        current = os.stat(self.path)
        if (current.st_dev, current.st_ino) != (self.identity.st_dev, self.identity.st_ino):
            raise RuntimeError("Observation file was replaced; restart the bridge")
        if current.st_size < self.file.tell():
            raise RuntimeError("Observation file was truncated; restart the bridge")
        self.pending += self.file.read(4 * 1024 * 1024 + 1)
        if len(self.pending) > 4 * 1024 * 1024:
            raise RuntimeError("Observation batch exceeds 4 MiB")
        lines = self.pending.split(b"\n")
        self.pending = lines.pop()
        events = []
        for line in lines:
            if not line:
                continue
            record = json.loads(line)
            session, sequence, stamp = record["session"], record["sequence"], record["monotonic_ns"]
            if not all(isinstance(value, int) and not isinstance(value, bool) and value >= 0 for value in (session, sequence, stamp)):
                raise ValueError("Invalid observation identity or timestamp")
            identity = (session, sequence)
            if identity == self.last:
                continue
            if self.last and session == self.last[0] and sequence < self.last[1]:
                raise ValueError("Observation sequence went backwards")
            self.last = identity
            events.append("vcmi:{}:{}:{}".format(session, sequence, stamp))
        return events


class MCPClient:
    def __init__(self, server, root):
        self.process = subprocess.Popen([server, "--recordings-dir", root], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        start_new_session=True)
        self.buffer = b""
        self.next_id = 1
        try:
            self.call("initialize", {"protocolVersion": "2025-06-06", "capabilities": {},
                                     "clientInfo": {"name": "vcmi-screener-bridge", "version": "1"}})
            self.send({"jsonrpc": "2.0", "method": "notifications/initialized"})
        except BaseException:
            self.close()
            raise

    def send(self, request):
        self.process.stdin.write((json.dumps(request) + "\n").encode())
        self.process.stdin.flush()

    def call(self, method, params):
        request_id = self.next_id
        self.next_id += 1
        self.send({"jsonrpc": "2.0", "id": request_id, "method": method, "params": params})
        deadline = time.monotonic() + 30
        while True:
            while b"\n" in self.buffer:
                line, self.buffer = self.buffer.split(b"\n", 1)
                response = json.loads(line)
                if response.get("id") != request_id:
                    continue
                if "error" in response:
                    raise RuntimeError("MCP error: {}".format(response["error"]))
                result = response["result"]
                if result.get("isError"):
                    raise RuntimeError("MCP tool error: {}".format(result.get("content")))
                return result.get("structuredContent", result)
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([self.process.stdout], [], [], remaining)[0]:
                raise TimeoutError("MCP server did not reply")
            chunk = os.read(self.process.stdout.fileno(), 65536)
            if not chunk:
                raise RuntimeError("MCP server closed its output")
            self.buffer += chunk
            if len(self.buffer) > 4 * 1024 * 1024:
                raise RuntimeError("MCP reply exceeds 4 MiB")

    def tool(self, name, arguments):
        return self.call("tools/call", {"name": name, "arguments": arguments})

    def close(self):
        self.process.stdin.close()
        try:
            self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.process.stdout.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True, help="Path to Screener MCP with window recording support")
    parser.add_argument("--observation-file", required=True, help="Existing VCMI JSONL observation file")
    parser.add_argument("--recordings-dir", required=True, help="Local Screener trace directory")
    parser.add_argument("--bundle-id", required=True, help="Exact VCMI application bundle identifier")
    args = parser.parse_args()
    tail = ObservationTail(args.observation_file)
    client = None
    session = None
    stopping = False
    previous_handler = signal.getsignal(signal.SIGINT)

    def request_stop(signum, frame):
        nonlocal stopping
        stopping = True

    signal.signal(signal.SIGINT, request_stop)
    try:
        client = MCPClient(args.server, args.recordings_dir)
        windows = client.tool("screener.windows", {"appBundleID": args.bundle_id})["windows"]
        if len(windows) != 1:
            raise RuntimeError("Expected exactly one visible window for {}; found {}".format(args.bundle_id, len(windows)))
        window = windows[0]
        session = client.tool("screener.window_start", {"appBundleID": args.bundle_id, "windowID": window["windowID"],
                                                       "processID": window["processID"], "name": "VCMI observations"})["sessionID"]
        print("Screener session {} started; waiting for new VCMI observations (Ctrl-C stops).".format(session), flush=True)
        while not stopping:
            for reason in tail.read():
                if stopping:
                    break
                result = client.tool("screener.window_capture", {"sessionID": session, "reason": reason})
                print("Captured {} ({} frames)".format(reason, result["capturedFrames"]), flush=True)
            if not stopping:
                time.sleep(0.1)
    finally:
        signal.signal(signal.SIGINT, previous_handler)
        tail.close()
        if client:
            try:
                if session:
                    client.tool("screener.window_stop", {"sessionID": session})
            finally:
                client.close()


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError, TimeoutError) as error:
        print("Screener bridge: {}".format(error), file=sys.stderr)
        sys.exit(1)
