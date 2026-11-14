# Transit safety and scouting at fog boundaries

The requested behavior is to prevent Flight or Water Walk routes from
stranding a hero on water, while retaining useful scouting behavior. A player
recalled using the mouse cursor over fog in a streamed game to identify a
possible landing site. The game edition, precise context and effect have not
been established; that recollection is a design input, not evidence of VCMI
or original Heroes III behavior.

In the baseline VCMI source, ordinary clicks on unrevealed tiles are ignored.
Ordinary hover after applying Flight or Water Walk returns the standard pointer
on an unrevealed tile. Adventure-spell targeting is processed before that
ordinary-hover check and is a separate case. None of these source observations
replaces the pending graphical test, or establishes another game's behavior.

The original toroidal server guard had a three-way landing search: FOUND, NONE and
UNKNOWN. It rejects a transition from a known safe landing to NONE, but permits
UNKNOWN to avoid treating unseen terrain as a known obstruction. The existing
single-island reproduction shows this may lead to stranding as fog is revealed.
The return-to-origin recovery reduces the damage when its preconditions hold;
it does not make the route safe in the first place.

Candidate safeguards need to satisfy all of the following:

- Do not let a hero exhaust its route beyond a reachable legal landing on
  an unchanged, single-island map; cover both spells and seam crossings.
- Permit a route to a visible, reachable shore, including one that spends the
  last movement points landing; cover partial fog around a valid route.
- Make acceptance depend on what the player knows, rather than hidden terrain
  or guards; compare paired maps with identical visibility but different
  concealed cells.
- Preserve existing bounded-map behavior and normal visual route hints; do not
  claim or silently add a new cursor-based hint about unseen terrain. Whether
  to add such an intentional hint remains a separate design decision.
- Retain recovery for interrupted moves, changed terrain, occupied cells and
  saves without a reliable recorded origin. A route guard alone cannot prove
  those cases safe.

Requiring a *known* landing is one possible implementation, but the search
also requires every route tile's guard neighborhood to be revealed. Treating
UNKNOWN as failure can therefore reject a route even if a landing exists
beyond fog. The subsequent [guard change](topology-unknown-frontier-guard-2026-11-14.md)
passes one visible-shore route near fog; broader route and GUI coverage remains open.
