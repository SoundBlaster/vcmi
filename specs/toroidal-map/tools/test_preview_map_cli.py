"""Opt-in checks for invalid combinations of the graphical preview CLI."""

import os
import subprocess
import unittest


@unittest.skipUnless(os.environ.get("VCMI_PREVIEW_CLIENT"), "Set VCMI_PREVIEW_CLIENT to the built client")
class PreviewMapCliTest(unittest.TestCase):
    def test_rejects_conflicting_modes_before_startup(self):
        client = os.environ["VCMI_PREVIEW_CLIENT"]
        cases = (["--onlyAI"], ["--headless"], ["--spectate"], ["--autoSkip"],
                 ["--donotstartserver"], ["--testmap", "Maps/example.vmap"],
                 ["--testsave", "Saves/example.vsgm1"])
        for args in cases:
            with self.subTest(args=args):
                result = subprocess.run([client, "--preview-map", "Maps/example.vmap", *args],
                                        capture_output=True, text=True, timeout=10)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("local human-controlled graphical session", result.stderr)

    def test_rejects_empty_map_and_incompatible_test_seed(self):
        client = os.environ["VCMI_PREVIEW_CLIENT"]
        cases = ((["--preview-map", ""], "local human-controlled graphical session"),
                 (["--preview-map", "Maps/example.vmap", "--testseed", "1"],
                  "--testseed requires --testmap"))
        for args, message in cases:
            with self.subTest(args=args):
                result = subprocess.run([client, *args], capture_output=True, text=True, timeout=10)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stderr)


if __name__ == "__main__":
    unittest.main()
