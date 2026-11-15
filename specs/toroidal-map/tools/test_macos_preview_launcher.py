"""Exercise the local preview launcher with a recording client."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(sys.platform == "darwin", "macOS launcher")
class PreviewLauncherTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="vcmi-launcher-test-")
        cls.directory = Path(cls.temporary.name)
        cls.launcher = cls.directory / "launcher"
        source = Path(__file__).with_name("macos_preview_launcher.c")
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(cls.launcher)], check=True)
        client = cls.directory / "vcmiclient"
        client.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
        client.chmod(0o755)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_launcher(self, *arguments):
        result = subprocess.run([str(self.launcher), *arguments], check=True, capture_output=True, text=True)
        return result.stdout.splitlines()

    def test_distinct_private_logs_and_forwarded_arguments(self):
        first = self.run_launcher("--version")
        second = self.run_launcher("--version")
        for result in (first, second):
            self.assertEqual(result[0], "--logLocation")
            directory = Path(result[1])
            self.addCleanup(shutil.rmtree, directory, True)
            self.assertEqual(directory.parent, Path("/tmp"))
            self.assertTrue(directory.name.startswith("vcmi-toroid-preview."))
            self.assertEqual(os.stat(directory).st_mode & 0o777, 0o700)
            self.assertEqual(result[2:], ["--version"])
        self.assertNotEqual(first[1], second[1])

    def test_explicit_log_location_is_preserved(self):
        self.assertEqual(self.run_launcher("--logLocation", "/tmp/custom log", "--version"),
                         ["--logLocation", "/tmp/custom log", "--version"])
        self.assertEqual(self.run_launcher("--logLocation=/tmp/custom", "--version"),
                         ["--logLocation=/tmp/custom", "--version"])


if __name__ == "__main__":
    unittest.main()
