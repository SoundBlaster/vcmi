import unittest

from preview_stacked_terrain import render_svg, stack_levels


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


if __name__ == "__main__":
    unittest.main()
