import importlib.util
import json
import os
from pathlib import Path
import select
import signal
import subprocess
import sys
import tempfile
import unittest

MODULE = Path(__file__).with_name("screener_observation_bridge.py")
spec = importlib.util.spec_from_file_location("screener_bridge", MODULE)
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)


class ObservationBridgeTest(unittest.TestCase):
    def test_tail_ignores_old_and_partial_records_and_detects_truncation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "observations.jsonl"
            path.write_text('{"session":1,"sequence":0,"monotonic_ns":10}\n')
            tail = bridge.ObservationTail(path)
            try:
                self.assertEqual(tail.read(), [])
                with path.open("ab") as output:
                    output.write(b'{"session":1,"sequence":1,')
                self.assertEqual(tail.read(), [])
                with path.open("ab") as output:
                    output.write(b'"monotonic_ns":20}\n')
                self.assertEqual(tail.read(), ["vcmi:1:1:20"])
                with path.open("ab") as output:
                    output.write(b'{"session":1,"sequence":1,"monotonic_ns":20}\n')
                self.assertEqual(tail.read(), [])
                path.write_text("")
                with self.assertRaisesRegex(RuntimeError, "truncated"):
                    tail.read()
            finally:
                tail.close()

    def test_process_captures_new_events_and_stops_session(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "observations.jsonl"
            trace.write_text('{"session":1,"sequence":0,"monotonic_ns":10}\n')
            server = root / "fake_server.py"
            server.write_text('''#!/usr/bin/env python3
import json, pathlib, sys
log = pathlib.Path(sys.argv[2]) / "calls.jsonl"
for line in sys.stdin:
    request = json.loads(line)
    if "id" not in request:
        continue
    method = request["method"]
    if method == "initialize":
        result = {}
    else:
        name = request["params"]["name"]
        args = request["params"]["arguments"]
        with log.open("a") as output:
            output.write(json.dumps({"name": name, "args": args}) + "\\n")
        values = {
            "screener.windows": {"windows": [{"windowID": 21, "processID": 42}]},
            "screener.window_start": {"sessionID": "b1e32985-cf88-4992-8741-dac8d8a760ff"},
            "screener.window_capture": {"capturedFrames": 1},
            "screener.window_stop": {},
        }
        result = {"structuredContent": values[name]}
    print(json.dumps({"jsonrpc": "2.0", "id": request["id"], "result": result}), flush=True)
''')
            server.chmod(0o755)
            process = subprocess.Popen([sys.executable, str(MODULE), "--server", str(server),
                                        "--observation-file", str(trace), "--recordings-dir", directory,
                                        "--bundle-id", "dev.test.game"], stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True, bufsize=1)
            try:
                self.assertTrue(select.select([process.stdout], [], [], 10)[0])
                self.assertIn("started", process.stdout.readline())
                with trace.open("a") as output:
                    event = '{"session":1,"sequence":1,"monotonic_ns":20}\n'
                    output.write(event + event)
                self.assertTrue(select.select([process.stdout], [], [], 10)[0])
                self.assertIn("Captured vcmi:1:1:20", process.stdout.readline())
                process.send_signal(signal.SIGINT)
                _, stderr = process.communicate(timeout=10)
                self.assertEqual(process.returncode, 0, stderr)
                calls = [json.loads(line) for line in (root / "calls.jsonl").read_text().splitlines()]
                self.assertEqual([call["name"] for call in calls], [
                    "screener.windows", "screener.window_start", "screener.window_capture", "screener.window_stop"])
                self.assertEqual(calls[2]["args"]["reason"], "vcmi:1:1:20")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    unittest.main()
