import unittest

from preview_stacked_terrain import (add_illustrative_snow_island, oceanize_edge_rock, polarize_snow_island,
                                     rejoin_east_land_fragment, render_svg,
                                     seam_summary, stack_levels)


class TerrainPreviewTest(unittest.TestCase):
    def test_illustrative_island_preserves_input_and_rejects_overwriting_land(self):
        rows = [["wt0_"] * 50 for _ in range(50)]
        preview, count = add_illustrative_snow_island(rows, (25, 25))
        self.assertGreater(count, 0)
        self.assertEqual(preview[25][25], "sn0_")
        self.assertEqual(rows[25][25], "wt0_")
        rows[25][25] = "gr0_"
        with self.assertRaises(ValueError):
            add_illustrative_snow_island(rows, (25, 25))
        with self.assertRaises(ValueError):
            add_illustrative_snow_island(rows, (1, 1))

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

    def test_only_boundary_connected_rock_is_shown_as_water(self):
        rows = [["rc0_", "wt0_", "wt0_", "wt0_"],
                ["wt0_", "wt0_", "wt0_", "wt0_"],
                ["wt0_", "wt0_", "rc0_", "wt0_"],
                ["wt0_", "wt0_", "wt0_", "wt0_"]]
        preview, changed = oceanize_edge_rock(rows)
        self.assertEqual(changed, 1)
        self.assertEqual(preview[0][0], "wt0_")
        self.assertEqual(preview[2][2], "rc0_")
        self.assertEqual(rows[0][0], "rc0_")

    def test_east_fragment_rejoins_surface_without_overwriting_existing_land(self):
        surface = [["wt0_"] * 4 for _ in range(4)]
        underground = [["wt0_"] * 4 for _ in range(4)]
        underground[1][2:] = ["sa1_", "sa2_"]
        underground[2][3] = "sa3_"
        surface[2][3] = "gr0_"
        s, u, info = rejoin_east_land_fragment(surface, underground, (3, 1))
        self.assertEqual(info, {"source_cells": 3, "copied": 2, "existing_land": 1})
        self.assertEqual((s[1][2], s[1][3], s[2][3]), ("sa1_", "sa2_", "gr0_"))
        self.assertEqual(u[1][2:], ["wt0_", "wt0_"])
        self.assertEqual(surface[1][3], "wt0_")
        self.assertEqual(underground[1][3], "sa2_")

    def test_east_fragment_rejects_rock_overlap_and_other_edges(self):
        surface = [["wt0_"] * 4 for _ in range(4)]
        underground = [["wt0_"] * 4 for _ in range(4)]
        underground[1][3] = "sa0_"
        surface[1][3] = "rc0_"
        with self.assertRaises(ValueError):
            rejoin_east_land_fragment(surface, underground, (3, 1))
        surface[1][3] = "wt0_"
        underground[0][3] = "sa0_"
        with self.assertRaises(ValueError):
            rejoin_east_land_fragment(surface, underground, (3, 1))


if __name__ == "__main__":
    unittest.main()
