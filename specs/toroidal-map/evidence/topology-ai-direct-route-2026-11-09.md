# Nullkiller2 direct seam routes

On 2026-11-09 on macOS arm64 with copied Complete data, a fully revealed SoD
fixture checked Nullkiller2 planning for each of four straight seam directions
and four diagonal corner crossings. In each case the internal AI pathfinder
offered a same-turn path whose first destination was the wrapped neighbor;
`AIGateway` completed it with one movement request through a scripted test
client. The client applies a simulated success response, so this is not a real
server/AI game or proof of general AI strategy. Server movement is covered
separately by the [integration record](topology-demo-integration-2026-11-09.md).

During testing the scripted client was corrected to invalidate the AI path
cache on movement, as the real client does; this independent fixture change
was committed as `134a6c9`. `vcmitest` built (`ai-seam-build.log`), and
`./vcmitest --gtest_filter=Nullkiller2_AIGateway.*` passed six tests
(`ai-seam-tests.log`). A full `./vcmitest` run passed 1,333 tests with three
disabled (`ai-seam-full-tests.log`). Obstacles, boat use, strategic choices,
other AI profiles, and real client/server execution remain unverified.

Source baseline `134a6c9` with local test change:

```text
e10931c5b02bb82358be2235e3489e24cec3bae3298f747bdb8811fe218ad665  test/nullkiller2/AIGatewayMovementTest.cpp
```
