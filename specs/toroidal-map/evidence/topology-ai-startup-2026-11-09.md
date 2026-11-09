# Nullkiller2 startup hero selection

Startup hero selection previously rejected a hero across a toroidal seam using
the planar squared distance to a town, even when its computed path could reach
the town this turn. The distance threshold now uses the shared map topology.
The original checks on path shape, turn and garrison remain in effect.

`Nullkiller2_AIGateway.startupRecognizesHeroNearTownAcrossSeam` constructs an
actual game state and town, moves a hero to the opposite edge and runs the AI
pathfinder and startup selection. On the synthetic 36-wide fixture, the same
positions produce no nearby hero with bounded topology and select the hero
with toroidal topology. The toroidal assertion failed before the fix and passed
after it. The town is intentionally placed at an edge for this synthetic case;
this does not establish that this placement is valid in an authored toroidal
map or that every town entrance and gameplay case works.

The focused run passed 19 tests across topology, AI movement and cluster
suites. Logs on TB5: `ai-startup-before-test.log`,
`ai-startup-compare-build.log` and `ai-startup-compare-tests.log`.
