# Upstream compatibility inventory

Status: source inspection only; integration and compatibility decisions remain
open. Feature source `5aafdd7f4` descends from `ced7f796e`; upstream
`580fe835778bd5a542d869045fe1f06aad555ed7` was rechecked on 2026-11-24.

| Input | Existing fork behavior | Integration question |
| --- | --- | --- |
| Tagged toroidal save | `versionForTopology(true)` selects `TOROIDAL_TRANSIENT_RETURN` (944 on the old branch); `encodeVersion` writes tag `0x5452` and the low-word version. | The tag identifies a fork layout but does not translate old version thresholds to upstream's new sequence. Preserve the original file and verify decoding against the released writer before promising support. |
| New bounded fork save | `versionForTopology(false)` selects `CAMPAIGN_ENEMY_HERO_CARRIED_OVER`, without the tag. | Determine how to write and read both upstream's new fields and previously released bounded layouts. |
| Untagged prototype save | The old reader interprets the numeric version directly. | Ordinals 940–943 overlap upstream; version alone cannot identify origin. Investigate other reliable evidence or reject ambiguity rather than guess. |
| Tagged VMAP | JSON map format uses separate `0x54520004` major marker. | Map format marker is independent of saved-game serialization. Verify real maps and object data with both builds. |
| Fork lobby greeting | `LobbyTopologyCapability` tags the version and negotiates numeric versions; framed packets separately identify layout. | Verify old fork/new fork and upstream combinations in both directions; tagged peers alone do not establish identical low-word semantics. |

`CLoadFile::readVersion` strips the tag before returning an enum; downstream
layout selection therefore receives only the ordinal. A future implementation
must carry enough format identity to choose the correct layout, or fail closed.
This observation does not prove which migration design is feasible. Existing
tests (`SaveFileTest`, `GameConnectionTopologyTest`) are candidates for the
compatibility matrix; none were run for an integrated build in this step.
