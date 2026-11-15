import unittest

from analyze_stacked_map import occupants


def obj(name, x, y, level, mask):
    return {"instanceName": name, "type": "test", "x": x, "y": y,
            "l": level, "template": {"mask": mask}}


class FootprintAnalysisTest(unittest.TestCase):
    def test_rotation_can_block_a_previously_clear_entrance(self):
        objects = [obj("entrance", 1, 1, 1, ["A"]),
                   obj("building", 0, 1, 1, ["BB"])]
        original, visits, clipped = occupants(objects, 5, 5, False)
        self.assertEqual(original, set())
        self.assertEqual(visits, [])
        self.assertEqual(clipped, 1)

        rotated, visits, clipped = occupants(objects, 5, 5, True)
        self.assertEqual(rotated, {("building", "entrance")})
        self.assertEqual(visits[0][0], (3, 3, 0))
        self.assertEqual(clipped, 0)

    def test_toroidal_footprint_wraps_and_visible_tiles_do_not_block(self):
        objects = [obj("entrance", 4, 4, 1, ["A"]),
                   obj("building", 4, 4, 1, ["BV"]),
                   obj("other", 0, 4, 1, ["A"])]
        pairs, visits, _ = occupants(objects, 5, 5, True)
        self.assertEqual(pairs, {("building", "other")})
        self.assertEqual(visits[0][0], (4, 0, 0))

    def test_visitable_placement_preserves_rotated_entrance_cell(self):
        objects = [obj("entrance", 2, 1, 1, ["AV"]),
                   obj("beacon", 1, 1, 1, ["A"])]
        anchor_pairs, _, _ = occupants(objects, 5, 5, True, "anchor")
        visit_pairs, visits, _ = occupants(objects, 5, 5, True, "visitable")
        self.assertEqual(anchor_pairs, set())
        self.assertEqual(visit_pairs, {("beacon", "entrance")})
        self.assertEqual(visits[0][0], (3, 3, 0))


if __name__ == "__main__":
    unittest.main()
