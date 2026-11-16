import unittest

from compare_walk_seams import compare


class CompareWalkSeamsTest(unittest.TestCase):
    def test_opposite_edges_join_distinct_components(self):
        report = compare([".###.", "#####", "#####"])
        self.assertEqual(report["components"], 2)
        self.assertEqual(len(report["seam_links"]), 1)
        self.assertEqual(report["seam_links"][0]["example"], [[0, 0], [4, 0]])

    def test_corner_diagonal_is_a_seam_link(self):
        report = compare([".###", "####", "####", "###."])
        self.assertEqual(report["components"], 2)
        self.assertEqual(len(report["seam_links"]), 1)

    def test_already_connected_is_not_reported(self):
        self.assertEqual(compare(["...."])["seam_links"], [])

    def test_zone_links_include_different_zones_even_if_component_already_connected(self):
        report = compare(["...."], [[1, 1, 2, 2]])
        self.assertEqual(report["seam_links"], [])
        self.assertEqual(report["zone_seam_links"][0]["zones"], (1, 2))
        self.assertGreater(report["zone_seam_links"][0]["directed_edges"], 0)

    def test_rejects_mismatched_zone_grid(self):
        with self.assertRaises(ValueError):
            compare([".."], [[1]])

    def test_bad_grids_rejected(self):
        for rows in ([], ["..", "."], [".?" ]):
            with self.assertRaises(ValueError):
                compare(rows)


if __name__ == "__main__":
    unittest.main()
