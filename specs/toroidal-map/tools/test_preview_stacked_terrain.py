import unittest

from preview_stacked_terrain import polarize_snow_island, render_svg, seam_summary, stack_levels


class TerrainPreviewTest(unittest.TestCase):
    def test_rotates_second_level_and_preserves_surface(self):
        surface = [["gr1_", "sa2_"], ["wt3_", "sn4_"]]
        underground = [["dt1_", "lv2_"], ["hl3_", "ws4_"]]
        self.assertEqual(stack_levels(surface, underground),
                         [["ws4_", "hl3_"], ["lv2_", "dt1_"]] + surface)

    def test_rejects_unknown_terrain_and_mismatched_dimensions(self):
        with self.assertRaises(ValueError):
            render_svg([["unknown"]])
        with self.assertRaises(ValueError):
            stack_levels([["gr0_"]], [["wt0_", "wt1_"]])

    def test_summarizes_opposing_edges_without_assuming_coast_is_invalid(self):
        rows = stack_levels([["gr0_", "wt0_"], ["rc0_", "wt0_"]],
                            [["wt0_", "wt0_"], ["gr0_", "wt0_"]])
        summary = seam_summary(rows)
        self.assertEqual(summary["north_south"], {"land/water": 1, "water/rock": 1})
        self.assertEqual(sum(summary["west_east"].values()), 4)
        self.assertEqual(sum(summary["level_join"].values()), 2)

    def test_polar_preview_moves_snow_without_overwriting_original_land(self):
        rows = [["wt0_", "wt0_", "wt0_", "wt0_"],
                ["gr0_", "sn0_", "sn1_", "wt0_"],
                ["wt0_", "sn2_", "sn3_", "wt0_"],
                ["wt0_", "wt0_", "wt0_", "wt0_"]]
        preview, north_depth, cells = polarize_snow_island(rows, (1, 1))
        self.assertEqual((north_depth, cells), (1, 4))
        self.assertEqual(len(preview), 6)
        self.assertEqual(preview[2][0], "gr0_")
        self.assertEqual(preview[2][1], "wt0_")
        self.assertTrue(all(tile.startswith("sn") for tile in preview[0]))
        self.assertTrue(all(tile.startswith("sn") for tile in preview[-1]))
        self.assertEqual(rows[1][1], "sn0_")

    def test_polar_preview_rejects_snow_touching_the_boundary(self):
        with self.assertRaises(ValueError):
            polarize_snow_island([["sn0_", "wt0_"], ["wt0_", "wt0_"]], (0, 0))


if __name__ == "__main__":
    unittest.main()
