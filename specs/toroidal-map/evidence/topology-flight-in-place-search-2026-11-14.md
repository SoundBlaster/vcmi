# Flight landing on the current tile

The known-landing search previously required a path to a different LAND node.
If a flying hero entered an unobstructed, fully known land tile and there was
no other reachable land, it could reject the AIR step even though the hero
could legally land at that tile. It also failed to count a current in-place
landing as a safe starting state when checking a subsequent AIR step.

The search now counts a safe in-place AIR landing at either the actual or the
prospective position. It uses the same in-place safety checks as recovery,
including known guard neighborhoods, passability, object occupancy and guard
checks. The check does not apply to boats or Water Walk. A hidden neighbor is
not treated as evidence of safe landing.

`FlightCanLandInPlaceOnOnlyReachableShore` covers a flight from one island
across water to a second, with in-place landing on the only reachable shore.
`InPlaceFlightLandingPreventsUnsafeDeparture` confirms that leaving that
safe tile for water without enough points to return is rejected. These tests
invoke the game handler in a test harness, not through the graphical client.

On the TB5 macOS build, `vcmitest`, `vcmiclient` and `vcmiserver` built;
101 `HeroMovementTest` cases and the two-peer TCP lobby test passed. This
fix does not resolve the separate UNKNOWN frontier stranding case or verify
interactive Flight and Water Walk behavior.
