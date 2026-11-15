import unittest

from check_traceability import scenario_references


class ScenarioReferencesTest(unittest.TestCase):
    def test_expands_ranges_and_retains_individual_scenarios(self):
        self.assertEqual(scenario_references("E4–E6, G2 and P1-P2"),
                         {"E4", "E5", "E6", "G2", "P1", "P2"})

    def test_rejects_invalid_ranges(self):
        for text in ("E6–E4", "E4–G5", "E1–E5000"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                scenario_references(text)


if __name__ == "__main__":
    unittest.main()
