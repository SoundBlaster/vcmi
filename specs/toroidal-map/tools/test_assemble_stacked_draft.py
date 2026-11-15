import copy
import unittest

from assemble_stacked_draft import assemble, blocking_pairs, place_object, rotate_cell_art


def obj(name, kind, x, y, level, mask):
    return {"instanceName": name, "type": kind, "subtype": "object", "x": x, "y": y, "l": level,
            "template": {"mask": mask}}


class StackedDraftTest(unittest.TestCase):
    def test_art_rotation_is_involutive_for_terrain_road_and_river(self):
        for tile in ("gr12_", "sn2-", "sw55_pd10_rw12|", "hl81_pd12_rw1+"):
            self.assertEqual(rotate_cell_art(rotate_cell_art(tile)), tile)
        self.assertEqual(rotate_cell_art("sw55_pd10_rw12|"), "sw55+pd10+rw12-")
        for invalid in ("", "gr_", "gr1?", "gr1_foo", "gr1_pd2_rw3_pc4_"):
            with self.assertRaises(ValueError):
                rotate_cell_art(invalid)

    def test_rotated_object_preserves_first_visitable_cell_and_canonical_anchor(self):
        town = obj("t", "town", 2, 1, 1, ["VV", "AB"])
        transformed = place_object(town, 4, 3)
        self.assertEqual((transformed["x"], transformed["y"], transformed["l"]), (3, 1, 0))
        # Original entrance is (1,1); rotation puts it at (2,1).
        self.assertEqual((transformed["x"] - 1, transformed["y"]), (2, 1))
        self.assertEqual(town["l"], 1)

    def test_outside_legacy_anchor_stays_canonical_in_the_new_map(self):
        source = obj("edge", "decoration", 4, 1, 1, ["BBB"])
        result = place_object(source, 4, 3)
        self.assertEqual((result["x"], result["y"], result["l"]), (1, 1, 0))

    def test_assembles_rows_and_rebinds_main_town(self):
        header = {"versionMajor": 3, "mapLevels": {"surface": {"width": 4, "height": 3, "index": 0},
                                                   "underground": {"width": 4, "height": 3, "index": 1}},
                  "players": {"red": {"mainTown": {"x": 2, "y": 1, "l": 1}}}}
        surface = [["gr0_"] * 4 for _ in range(3)]
        underground = [["wt0_"] * 4 for _ in range(3)]
        underground[0][0] = "sn1-"
        objects = [obj("t", "town", 2, 1, 1, ["VV", "AB"])]
        original = copy.deepcopy((header, surface, underground, objects))
        result_header, rows, converted, report = assemble(header, surface, underground, objects)
        self.assertEqual(result_header["versionMajor"], 0x54520004)
        self.assertEqual(result_header["topology"], "toroidal")
        self.assertEqual(result_header["players"]["red"]["mainTown"], {"x": 3, "y": 1, "l": 0})
        self.assertEqual(rows[2][3], "sn1|")
        self.assertEqual(rows[3:], surface)
        self.assertEqual(report["dimensions"], [4, 6])
        self.assertEqual(len(converted), 1)
        self.assertEqual((header, surface, underground, objects), original)

    def test_rejects_unsupported_nested_references(self):
        header = {"mapLevels": {"surface": {"width": 1, "height": 1, "index": 0},
                                "underground": {"width": 1, "height": 1, "index": 1}},
                  "events": [{"position": {"x": 0, "y": 0, "l": 0}}]}
        with self.assertRaisesRegex(ValueError, "Unsupported coordinate reference"):
            assemble(header, [["gr0_"]], [["wt0_"]], [])

    def test_rejoins_fragment_and_moves_whole_objects_including_outside_anchor(self):
        header = {"mapLevels": {"surface": {"width": 4, "height": 4, "index": 0},
                                "underground": {"width": 4, "height": 4, "index": 1}},
                  "players": {"red": {"mainTown": {"x": 3, "y": 1, "l": 1}}}}
        surface = [["wt0_"] * 4 for _ in range(4)]
        underground = [["wt0_"] * 4 for _ in range(4)]
        underground[1][2:] = ["sa1_", "sa2_"]
        objects = [obj("town", "town", 3, 1, 1, ["AB"]),
                   obj("outside", "decoration", 4, 1, 1, ["BB"]),
                   obj("other", "decoration", 0, 3, 1, ["B"])]
        new_header, rows, moved, report = assemble(header, surface, underground, objects, (3, 1))
        self.assertEqual(rows[5][2:], ["sa1_", "sa2_"])
        self.assertEqual((moved[0]["x"], moved[0]["y"]), (3, 5))
        self.assertEqual((moved[1]["x"], moved[1]["y"]), (0, 5))
        self.assertEqual((moved[2]["x"], moved[2]["y"]), (3, 0))
        self.assertEqual(new_header["players"]["red"]["mainTown"], {"x": 3, "y": 5, "l": 0})
        self.assertEqual(report["east_fragment_objects_moved"], 2)
        self.assertEqual(report["east_fragment"]["source_cells"], 2)
        self.assertEqual(underground[1][3], "sa2_")

    def test_collision_analysis_wraps_across_boundaries(self):
        objects = [obj("a", "x", 0, 1, 0, ["AB"]),
                   obj("b", "x", 3, 1, 0, ["B"])]
        pairs, interactions = blocking_pairs(objects, 4, 4)
        self.assertEqual(pairs, {("a", "b")})
        self.assertEqual(interactions, [("a", "b")])

    def test_stretches_polar_island_and_keeps_objects_upright(self):
        header = {"mapLevels": {"surface": {"width": 6, "height": 6, "index": 0},
                                "underground": {"width": 6, "height": 6, "index": 1}},
                  "players": {"red": {"mainTown": {"x": 2, "y": 3, "l": 1}}}}
        surface = [["wt0_"] * 6 for _ in range(6)]
        underground = [["wt0_"] * 6 for _ in range(6)]
        for y in (2, 3):
            for x in (2, 3):
                underground[y][x] = "sn0_"
        objects = [obj("town", "town", 2, 3, 1, ["A"]),
                   obj("elsewhere", "decoration", 1, 5, 0, ["B"])]
        result_header, rows, converted, report = assemble(header, surface, underground, objects, polar_seed=(2, 2))
        self.assertEqual(report["dimensions"], [6, 14])
        self.assertEqual(report["polar_component"], {"source_snow_cells": 4, "north_band_rows": 1,
                                                     "south_band_rows": 1, "objects_moved": 1,
                                                     "objects_with_active_cells_on_water": []})
        self.assertEqual((converted[0]["x"], converted[0]["y"], converted[0]["l"]), (4, 13, 0))
        self.assertEqual(result_header["players"]["red"]["mainTown"], {"x": 4, "y": 13, "l": 0})
        self.assertEqual((converted[1]["x"], converted[1]["y"]), (1, 12))
        self.assertTrue(all(cell.startswith("sn") for cell in rows[0] + rows[-1]))
        self.assertEqual(underground[2][2], "sn0_")

    def test_polar_island_rejects_unhandled_road_or_river(self):
        header = {"mapLevels": {"surface": {"width": 4, "height": 4, "index": 0},
                                "underground": {"width": 4, "height": 4, "index": 1}}}
        surface = [["wt0_"] * 4 for _ in range(4)]
        underground = [["wt0_"] * 4 for _ in range(4)]
        underground[2][2] = "sn0_pd1_"
        with self.assertRaisesRegex(ValueError, "road or river overlays"):
            assemble(header, surface, underground, [], polar_seed=(1, 1))

    def test_oceanizing_rejects_blocking_part_of_interactive_object(self):
        header = {"mapLevels": {"surface": {"width": 4, "height": 4, "index": 0},
                                "underground": {"width": 4, "height": 4, "index": 1}}}
        surface = [["wt0_"] * 4 for _ in range(4)]
        surface[0][0] = "rc0_"
        underground = [["wt0_"] * 4 for _ in range(4)]
        town = obj("town", "town", 0, 0, 0, ["AB"])
        with self.assertRaisesRegex(ValueError, "visitable objects"):
            assemble(header, surface, underground, [town], oceanize=True)

    def test_schematic_island_cannot_overwrite_object_footprints(self):
        header = {"mapLevels": {"surface": {"width": 50, "height": 50, "index": 0},
                                "underground": {"width": 50, "height": 50, "index": 1}}}
        water = [["wt0_"] * 50 for _ in range(50)]
        _, rows, _, report = assemble(header, water, water, [], snow_island=(25, 25))
        self.assertEqual(report["illustrative_snow_island"]["objects"], 0)
        self.assertEqual(rows[25][25], "sn0_")
        blocking = obj("b", "decoration", 24, 24, 1, ["B"])
        with self.assertRaisesRegex(ValueError, "overlaps objects"):
            assemble(header, water, water, [blocking], snow_island=(25, 25))


if __name__ == "__main__":
    unittest.main()
