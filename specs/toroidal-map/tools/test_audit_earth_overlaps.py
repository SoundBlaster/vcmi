import unittest

from audit_earth_overlaps import audit


def obj(name, x, y, level, mask):
    return {"instanceName": name, "type": name, "x": x, "y": y, "l": level,
            "template": {"mask": mask}}


class OverlapAuditTest(unittest.TestCase):
    def test_wrap_finds_new_interactive_pair_and_ignores_old_pair(self):
        source = [obj("visit", 0, 0, 0, ["A"]), obj("old", 0, 0, 0, ["B"]),
                  obj("new", 3, 3, 1, ["B"])]
        draft = [obj("visit", 0, 0, 0, ["A"]), obj("old", 0, 0, 0, ["B"]),
                 obj("new", 1, 0, 0, ["BV"])]
        report = audit(source, draft, (4, 4), (4, 4))
        self.assertEqual(report["new_interactive_pairs"], 1)
        self.assertEqual(report["pairs"][0]["names"], ["new", "visit"])
        self.assertEqual(report["pairs"][0]["overlaps"][0]["cell"], [0, 0])

    def test_no_interaction_and_invalid_dimensions(self):
        source = [obj("a", 0, 0, 0, ["B"])]
        draft = [obj("a", 0, 0, 0, ["B"]), obj("b", 0, 0, 0, ["B"])]
        self.assertEqual(audit(source, draft, (4, 4), (4, 4))["new_interactive_pairs"], 0)
        with self.assertRaises(ValueError):
            audit(source, draft, (0, 4), (4, 4))

    def test_actual_wrapping_at_edge(self):
        source = [obj("visit", 3, 0, 0, ["A"]), obj("new", 1, 0, 1, ["B"])]
        draft = [obj("visit", 3, 0, 0, ["A"]), obj("new", 0, 0, 0, ["BV"])]
        report = audit(source, draft, (4, 4), (4, 4))
        self.assertEqual(report["pairs"][0]["overlaps"][0]["cell"], [3, 0])


if __name__ == "__main__":
    unittest.main()
