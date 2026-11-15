import copy
import unittest

from assemble_stacked_draft import assemble, place_object, rotate_cell_art


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


if __name__ == "__main__":
    unittest.main()
