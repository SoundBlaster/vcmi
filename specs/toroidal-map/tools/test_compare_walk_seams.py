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

    def test_bad_grids_rejected(self):
        for rows in ([], ["..", "."], [".?" ]):
            with self.assertRaises(ValueError):
                compare(rows)


if __name__ == "__main__":
    unittest.main()
