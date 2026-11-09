# Random templates and toroidal maps: design candidate

This note records an open design direction, not implemented generator support.
The user asked about zone templates such as Jebus Outcast and observed that a
map edge could be blocked by forest. A random-map template describes zones,
connections and other rules; its diagram is not necessarily a spatial map.
Joining map edges changes tile adjacency and can introduce shortcuts between
zones, including diagonally at corners. It can thereby bypass guards or other
rules even when the generated map is otherwise valid.

## Existing tools

The checked VCMI source includes a graphical Template Editor inside the map
editor. Its open/save dialogs select VCMI `.json` templates. HotA documents a
separate editor and `.h3t` files. Conversion and availability of a particular
Jebus Outcast version, including its rules and mod dependencies, have not
been verified. Importing a diagram or a file is not proof that another engine
will generate the same maps or apply identical game rules.

- [VCMI template format](https://github.com/vcmi/vcmi/blob/develop/docs/modders/Random_Map_Template.md)
- [HotA template editor](https://h3hota.com/en/templates)

## Options to evaluate

1. **Sealed border:** generate a conventional map, retain impassable terrain
   or obstacle barriers at seams, then use toroidal topology. This can avoid
   unintended seam passages if checked, but completely sealed axes provide no
   playable wrap across those axes and do not by themselves satisfy the goal
   of a map with meaningful crossings.
2. **Controlled openings:** keep barriers except for seam passages explicitly
   permitted by the template, with appropriate guard and terrain rules. This
   needs validation on both sides of a seam and for diagonals at corners.
3. **Toroidal-native generation:** use wrapped geometry during zone placement,
   adjacency, obstacle placement, routing, terrain and roads. This can allow
   zones themselves to span a seam, but is a broader change to the generator.

A compatible implementation could combine these approaches per template.
The map topology flag alone does not select or verify them. Objects spanning a
seam remain restricted in the current pilot, so a forest border also needs to
be made with supported terrain and footprints until that restriction changes.

## Verification needed before advertising support

For a named template and a range of reproducible seeds, compare the actual
traversable map graph to the intended zone connections. Check passages and
guards across horizontal and vertical seams and all corners, including blocked
cases; do not assume that a blocked cardinal step necessarily settles every
diagonal rule. Check that player starting areas and reachable resources remain
appropriate, and that seam terrain, roads and rivers render coherently. Test
the generator's topology flag, saved map and editor round trip, then exercise
the result in game. Verify special template rules separately, such as hero
limits, enabled objects, mods and conversion fidelity.

No specific random generator, HotA import or Jebus Outcast profile is claimed
as supported until those behaviors have been implemented and demonstrated.
