import copy
import unittest

from assemble_stacked_draft import (assemble, blocking_pairs, place_object, relocate_decorations, relocate_water_monoliths, relocate_water_whirlpools, replace_gate_pairs,
                                    repair_mine_guards, repair_mountain_interactions, repair_south_guards,
                                    rotate_cell_art, source_gate_pairs)


def obj(name, kind, x, y, level, mask):
    return {"instanceName": name, "type": kind, "subtype": "object", "x": x, "y": y, "l": level,
            "template": {"mask": mask}}


class StackedDraftTest(unittest.TestCase):
    def test_water_whirlpool_move_requires_clear_water_and_preserves_source(self):
        whirlpool = obj("whirlpool", "whirlpool", 3, 3, 0, ["AAA", "AAA"])
        source = [whirlpool]
        placed = copy.deepcopy(source)
        source_rows = [["wt0_"] * 8 for _ in range(8)]
        rows = copy.deepcopy(source_rows)
        rows[3][3] = "sn0_"
        report = relocate_water_whirlpools(source, placed, [source_rows, source_rows], rows,
                                          [("whirlpool", 4, 5)])
        self.assertEqual(report, [{"object": "whirlpool", "from": [3, 3], "to": [4, 5]}])
        self.assertEqual((source[0]["x"], source[0]["y"]), (3, 3))
        self.assertEqual(placed[0]["template"], whirlpool["template"])
        dry = copy.deepcopy(rows)
        dry[5][3] = "sn0_"
        with self.assertRaisesRegex(ValueError, "clear water"):
            relocate_water_whirlpools(source, copy.deepcopy(source), [source_rows, source_rows], dry,
                                      [("whirlpool", 4, 5)])
        blocker = obj("rock", "mountain", 3, 5, 0, ["B"])
        with self.assertRaisesRegex(ValueError, "occupied"):
            relocate_water_whirlpools(source, copy.deepcopy(source) + [blocker], [source_rows, source_rows], rows,
                                      [("whirlpool", 4, 5)])
        bad_source = copy.deepcopy(source_rows)
        bad_source[3][3] = "sn0_"
        with self.assertRaisesRegex(ValueError, "Source whirlpool entrances"):
            relocate_water_whirlpools(source, copy.deepcopy(source), [bad_source, source_rows], rows,
                                      [("whirlpool", 4, 5)])

    def test_assembler_applies_explicit_water_monolith_move(self):
        header = {"mapLevels": {"surface": {"width": 8, "height": 8, "index": 0},
                                "underground": {"width": 8, "height": 8, "index": 1}}}
        surface = [["wt0_"] * 8 for _ in range(8)]
        underground = [["wt0_"] * 8 for _ in range(8)]
        portal = obj("water", "monolithTwoWay", 2, 2, 0, ["BAB"])
        portal["subtype"] = "hotaPortalWaterBlue"
        portal["template"]["visitableFrom"] = ["---", "+-+", "+++"]
        _, _, converted, report = assemble(header, surface, underground, [portal],
                                            relocate_water_monolith=(("water", 5, 5),))
        self.assertEqual((converted[0]["x"], converted[0]["y"]), (5, 5))
        self.assertEqual(report["relocated_water_monoliths"],
                         [{"object": "water", "from": [2, 10], "to": [5, 5]}])
        self.assertEqual((portal["x"], portal["y"]), (2, 2))

    def test_water_monolith_move_preserves_source_and_requires_clear_approach(self):
        portal = obj("water", "monolithTwoWay", 2, 2, 0, ["BAB"])
        portal["subtype"] = "hotaPortalWaterBlue"
        portal["template"]["visitableFrom"] = ["---", "+-+", "+++"]
        source = [portal]
        placed = copy.deepcopy(source)
        source_rows = [["wt0_"] * 8 for _ in range(8)]
        rows = copy.deepcopy(source_rows)
        report = relocate_water_monoliths(source, placed, [source_rows, source_rows], rows, [("water", 5, 5)])
        self.assertEqual(report, [{"object": "water", "from": [2, 2], "to": [5, 5]}])
        self.assertEqual((source[0]["x"], source[0]["y"]), (2, 2))
        self.assertEqual((placed[0]["x"], placed[0]["y"]), (5, 5))

        blocker = obj("rock", "mountain", 4, 5, 0, ["B"])
        with self.assertRaisesRegex(ValueError, "occupied"):
            relocate_water_monoliths(source, copy.deepcopy(source) + [blocker], [source_rows, source_rows],
                                      rows, [("water", 5, 5)])
        dry = copy.deepcopy(rows)
        dry[5][4] = "sa0_"
        with self.assertRaisesRegex(ValueError, "clear water"):
            relocate_water_monoliths(source, copy.deepcopy(source), [source_rows, source_rows],
                                      dry, [("water", 5, 5)])
        no_approach = copy.deepcopy(rows)
        for x, y in ((3, 5), (5, 5), (3, 6), (4, 6), (5, 6)):
            if (x, y) not in ((3, 5), (4, 5), (5, 5)):
                no_approach[y][x] = "sa0_"
        with self.assertRaisesRegex(ValueError, "no clear water approach"):
            relocate_water_monoliths(source, copy.deepcopy(source), [source_rows, source_rows],
                                      no_approach, [("water", 5, 5)])
        wrong_source = copy.deepcopy(source_rows)
        wrong_source[2][1] = "sa0_"
        with self.assertRaisesRegex(ValueError, "Source monolith entrance"):
            relocate_water_monoliths(source, copy.deepcopy(source), [wrong_source, source_rows],
                                      rows, [("water", 5, 5)])
        with self.assertRaisesRegex(ValueError, "outside destination"):
            relocate_water_monoliths(source, copy.deepcopy(source), [source_rows, source_rows],
                                      rows, [("water", 8, 5)])

    def test_guard_can_return_to_same_road_type_but_not_different_overlay(self):
        source = [obj("building", "arena", 5, 4, 1, ["BBB", "BAB"]),
                  obj("guard", "monster", 5, 5, 1, ["AV"])]
        source_rows = [["gr0_"] * 8 for _ in range(8)]
        source_rows[5][4] = "gr0_pc1_"
        for terrain, expected in (("gr0_pc2+", 1), ("gr0_pd2+", 0), ("gr0_rw2+", 0)):
            converted = copy.deepcopy(source)
            converted[0]["l"] = converted[1]["l"] = 0
            converted[1]["y"] = 3
            rows = [["gr0_"] * 8 for _ in range(8)]
            rows[5][4] = terrain
            result = repair_south_guards(source, converted, rows, 8, 8, {"arena"}, source_rows)
            self.assertEqual(len(result["moves"]), expected, terrain)

    def test_guard_repair_is_limited_to_explicit_object_types(self):
        source = [obj("building", "arena", 5, 4, 1, ["BBB", "BAB"]),
                  obj("guard", "monster", 5, 5, 1, ["AV"])]
        converted = copy.deepcopy(source)
        converted[0]["l"] = converted[1]["l"] = 0
        converted[1]["y"] = 3
        rows = [["gr0_"] * 8 for _ in range(8)]
        self.assertEqual(repair_mine_guards(source, converted, rows, 8, 8)["candidates"], 0)
        report = repair_south_guards(source, converted, rows, 8, 8, {"arena"})
        self.assertEqual(report["candidates"], 1)
        self.assertEqual(report["moves"][0]["object"], "building")
        self.assertEqual(converted[1]["y"], 5)

    def test_repair_mine_guard_restores_relative_position_and_keeps_options(self):
        source = [obj("mine", "mine", 5, 4, 1, ["BBB", "BAB"]),
                  obj("guard", "randomMonsterLevel2", 5, 5, 1, ["AV"])]
        source[1]["options"] = {"count": 17}
        converted = copy.deepcopy(source)
        converted[0]["l"] = converted[1]["l"] = 0
        converted[1]["y"] = 3
        rows = [["gr0_"] * 8 for _ in range(8)]
        self.assertIn(("guard", "mine"), blocking_pairs(converted, 8, 8)[0])
        report = repair_mine_guards(source, converted, rows, 8, 8)
        self.assertEqual(report["candidates"], 1)
        self.assertEqual(len(report["moves"]), 1)
        self.assertEqual(report["skipped"], [])
        self.assertEqual((converted[1]["x"], converted[1]["y"]), (5, 5))
        self.assertEqual(converted[1]["options"], {"count": 17})
        self.assertNotIn(("guard", "mine"), blocking_pairs(converted, 8, 8)[0])
        self.assertEqual(source[1]["y"], 5)

    def test_repair_mine_guard_leaves_occupied_target_untouched(self):
        source = [obj("mine", "mine", 5, 4, 1, ["BBB", "BAB"]),
                  obj("guard", "monster", 5, 5, 1, ["AV"]),
                  obj("obstacle", "mountain", 0, 0, 0, ["B"])]
        converted = copy.deepcopy(source)
        converted[0]["l"] = converted[1]["l"] = 0
        converted[1]["y"] = 3
        converted[2].update(x=4, y=5)
        report = repair_mine_guards(source, converted, [["gr0_"] * 8 for _ in range(8)], 8, 8)
        self.assertEqual(report["moves"], [])
        self.assertEqual(report["skipped"], ["guard"])
        self.assertEqual(converted[1]["y"], 3)

    def test_required_mods_preserve_existing_and_do_not_modify_input(self):
        header = {"mapLevels": {"surface": {"width": 1, "height": 1, "index": 0},
                                "underground": {"width": 1, "height": 1, "index": 1}},
                  "mods": {"allOf": ["hota.cove"]}}
        result, _, _, _ = assemble(header, [["gr0_"]], [["wt0_"]], [],
                                   required_mods=("hota.cove", "hota.factory"))
        self.assertEqual(result["mods"]["allOf"], ["hota.cove", "hota.factory"])
        self.assertEqual(header["mods"]["allOf"], ["hota.cove"])
        header["mods"] = None
        result, _, _, _ = assemble(header, [["gr0_"]], [["wt0_"]], [], required_mods=("hota.cove",))
        self.assertEqual(result["mods"], {"allOf": ["hota.cove"]})

    def test_required_mods_refuse_to_downgrade_versioned_metadata(self):
        header = {"mapLevels": {"surface": {"width": 1, "height": 1, "index": 0},
                                "underground": {"width": 1, "height": 1, "index": 1}},
                  "mods": [{"modId": "hota.cove", "version": "1.0"}]}
        with self.assertRaisesRegex(ValueError, "unversioned mod requirements"):
            assemble(header, [["gr0_"]], [["wt0_"]], [], required_mods=("hota.factory",))

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

    def test_replaces_only_original_pairs_with_unused_portal_subtypes(self):
        source = [obj("a", "subterraneanGate", 1, 1, 0, ["A"]),
                  obj("b", "subterraneanGate", 1, 3, 0, ["A"]),
                  obj("c", "subterraneanGate", 4, 1, 1, ["A"]),
                  obj("d", "subterraneanGate", 4, 3, 1, ["A"]),
                  obj("existing", "monolithTwoWay", 2, 2, 0, ["A"])]
        source[-1]["subtype"] = "monolith1"
        self.assertEqual(source_gate_pairs(source, 6, 6, False), {("a", "c"), ("b", "d")})
        converted = copy.deepcopy(source)
        report = replace_gate_pairs(source, converted, [("a", "c", "monolith2"), ("b", "d", "monolith4")], 6, 6, False)
        self.assertEqual(len(report), 2)
        self.assertEqual([(a["type"], a["subtype"]) for a in converted],
                         [("monolithTwoWay", "monolith2"), ("monolithTwoWay", "monolith4"),
                          ("monolithTwoWay", "monolith2"), ("monolithTwoWay", "monolith4"),
                          ("monolithTwoWay", "monolith1")])
        self.assertEqual(converted[0]["template"], source[0]["template"])
        self.assertEqual(source[0]["type"], "subterraneanGate")
        with self.assertRaisesRegex(ValueError, "does not match source-map pairing"):
            replace_gate_pairs(source, copy.deepcopy(source), [("a", "d", "monolith2")], 6, 6, False)
        with self.assertRaisesRegex(ValueError, "subtype is unavailable"):
            replace_gate_pairs(source, copy.deepcopy(source), [("a", "c", "monolith1")], 6, 6, False)
        with self.assertRaisesRegex(ValueError, "subtype is unavailable"):
            replace_gate_pairs(source, copy.deepcopy(source), [("a", "c", "monolith2"), ("b", "d", "monolith2")], 6, 6, False)

    def test_relocates_plain_mountain_without_new_overlap_or_road_blockage(self):
        rows = [["gr0_"] * 7 for _ in range(7)]
        rows[2][3] = "gr0_pd1_"
        mountain = obj("hill", "mountain", 3, 3, 0, ["B"])
        portal = obj("gate", "monolithTwoWay", 3, 3, 0, ["A"])
        report = relocate_decorations([mountain, portal], rows, ["hill"])
        self.assertEqual(report, [{"object": "hill", "from": [3, 3], "to": [2, 3]}])
        self.assertEqual((portal["x"], portal["y"]), (3, 3))
        self.assertFalse(blocking_pairs([mountain, portal], 7, 7)[0])

    def test_relocation_refuses_interactive_or_unplaceable_objects(self):
        rows = [["wt0_"] * 4 for _ in range(4)]
        interactive = obj("bad", "mountain", 1, 1, 0, ["A"])
        with self.assertRaisesRegex(ValueError, "Interactive objects"):
            relocate_decorations([interactive], rows, ["bad"])
        mountain = obj("isolated", "mountain", 1, 1, 0, ["B"])
        with self.assertRaisesRegex(ValueError, "No clear dry placement"):
            relocate_decorations([mountain], rows, ["isolated"], max_distance=2)
        self.assertEqual((mountain["x"], mountain["y"]), (1, 1))

    def test_selected_spruce_relocation_is_explicit_and_preserves_source_interactions(self):
        rows = [["gr0_"] * 4 for _ in range(4)]
        spruce = obj("tree", "spruces", 1, 1, 0, ["B"])
        with self.assertRaisesRegex(ValueError, "plain allowed decoration"):
            relocate_decorations([spruce], rows, ["tree"])
        report = relocate_decorations([spruce], rows, ["tree"], allowed_types={"spruces"})
        self.assertEqual(len(report), 1)
        header = {"mapLevels": {"surface": {"width": 4, "height": 4, "index": 0},
                                "underground": {"width": 4, "height": 4, "index": 1}}}
        overlap = [obj("tree", "spruces", 1, 1, 0, ["B"]),
                   obj("hero", "hero", 1, 1, 0, ["A"])]
        with self.assertRaisesRegex(ValueError, "source interactive overlap"):
            assemble(header, rows, rows, overlap, relocate_obstacles=("tree",))

    def test_selected_obstacle_relocation_avoids_reserved_cells(self):
        rows = [["gr0_"] * 4 for _ in range(4)]
        mountain = obj("hill", "mountain", 1, 1, 0, ["B"])
        report = relocate_decorations([mountain], rows, ["hill"], reserved={(1, 0)})
        self.assertEqual(len(report), 1)
        self.assertNotEqual((mountain["x"], mountain["y"]), (1, 0))


    def test_repair_moves_only_new_mountain_interactions(self):
        source = [obj("mountain", "mountain", 1, 1, 0, ["B"]),
                  obj("portal", "monolithTwoWay", 3, 3, 0, ["A"])]
        converted = copy.deepcopy(source)
        converted[0].update(x=3, y=3)
        rows = [["gr0_"] * 7 for _ in range(14)]
        report = repair_mountain_interactions(source, converted, rows, 7, 7)
        self.assertEqual(report["candidates"], 1)
        self.assertEqual(len(report["moves"]), 1)
        self.assertEqual(report["no_clear_placement"], [])
        self.assertFalse(blocking_pairs(converted, 7, 14)[0])
        self.assertEqual((converted[1]["x"], converted[1]["y"]), (3, 3))
        # Original gameplay overlaps are deliberately left alone.
        source[0].update(x=3, y=3)
        converted = copy.deepcopy(source)
        report = repair_mountain_interactions(source, converted, rows, 7, 7)
        self.assertEqual(report["candidates"], 0)
        self.assertEqual((converted[0]["x"], converted[0]["y"]), (3, 3))

    def test_repair_reports_when_no_safe_mountain_placement_exists(self):
        source = [obj("mountain", "mountain", 1, 1, 0, ["B"]),
                  obj("portal", "monolithTwoWay", 2, 2, 0, ["A"])]
        converted = copy.deepcopy(source)
        converted[0].update(x=2, y=2)
        report = repair_mountain_interactions(source, converted, [["wt0_"] * 4 for _ in range(8)], 4, 4)
        self.assertEqual(report["no_clear_placement"], ["mountain"])
        self.assertEqual((converted[0]["x"], converted[0]["y"]), (2, 2))

    def test_repair_detects_new_interactive_overlap_between_previously_touching_objects(self):
        source = [obj("mountain", "mountain", 2, 2, 0, ["B"]),
                  obj("portal", "monolithTwoWay", 2, 2, 0, ["AB"])]
        converted = copy.deepcopy(source)
        converted[0]["x"] = 1
        report = repair_mountain_interactions(source, converted, [["gr0_"] * 6 for _ in range(12)], 6, 6)
        self.assertEqual(report["candidates"], 1)
        self.assertEqual(len(report["moves"]), 1)
        self.assertFalse(blocking_pairs(converted, 6, 12)[0])


if __name__ == "__main__":
    unittest.main()
