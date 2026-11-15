import unittest

from analyze_polar_objects import analyze_objects


class PolarObjectsTest(unittest.TestCase):
    def test_counts_anchors_footprints_and_visitable_cells_before_rotation(self):
        def obj(name, level, x, y, mask):
            return {"instanceName": name, "type": name, "l": level,
                    "x": x, "y": y, "template": {"mask": mask}}

        objects = [obj("mine", 1, 3, 3, ["A"]),
                   obj("tree", 1, 4, 3, ["BV"]),
                   obj("surface", 0, 3, 3, ["A"])]
        result = analyze_objects(objects, {(1, 1)}, 5, 5)
        self.assertEqual(result["object_anchors"], 1)
        self.assertEqual(result["object_footprints"], 2)
        self.assertEqual(result["objects_with_visitable_cells"], 1)
        self.assertEqual(result["visitable_types"], {"mine": 1})

    def test_rejects_component_outside_rotated_level(self):
        with self.assertRaises(ValueError):
            analyze_objects([], {(1, 5)}, 5, 5)


if __name__ == "__main__":
    unittest.main()
