# Current macOS preview and corner crossing

On 2026-11-15, the macOS arm64 client and server were built from `496d90b`.
A separate development-signed bundle was prepared at
`runtime/VCMI Toroid Preview 496d90b.app` under the TB5 VCMI build root and
passed deep, strict signature verification. It uses the existing local VCMI
data and the user's Heroes III Complete resources; it is a local test build,
not a distributable package.

Launching it through macOS Launch Services with a local temporary log reached
the scan of the TB5-backed Saves directory and then waited. A process sample
showed a directory open call, while the system TCC log recorded a removable
volume access request and `AUTHREQ_PROMPTING` for this bundle identifier.
No response to that request was observed before the test process was
terminated. This observation does not establish that the prompt was visible
or predict the outcome of a later authorization.

After the user reported granting some macOS permissions, a second Launch
Services attempt of the same signed bundle also remained in filesystem
initialization. A process sample showed the main thread blocked in an `open`
call during recursive directory initialization, before the requested log was
created. The available TCC log showed an All Files preflight, but did not
establish a removable-volume prompt or the cause of this second wait. The
process was terminated; the sample is retained as
`runtime/gui-preview-496d90b/launchservices-permission-recheck-sample.txt`.
This does not show which permission the user granted or whether it applies to
this particular bundle.

Launching the same signed executable directly from its `Contents/MacOS`
directory, with a temporary local log and diagnostic socket, reached the game
menu. A new single-player `Toroid Seam Laboratory` game opened on the 36×36
map. A player-visible diagnostic snapshot reported toroidal topology and the
selected hero at `(0,0,0)` with 1,560 movement points and 20,000 gold. The
first northwest command collected 1,000 gold through the corner while the
hero remained at `(0,0,0)` with 1,419 movement points. The second northwest
command moved the hero to `(35,35,0)` with 1,278 movement points. The client
closed after the test and TCP port 3030 was not listening. The new game
created its own save directory; no existing save was intentionally changed.

This confirms startup and one observed corner interaction on this build,
not uninterrupted animation, remote play, or retained Launch Services disk
authorization. The temporary log, observation trace, and process sample are
copied under `runtime/gui-preview-496d90b/` on TB5:

| Artifact | SHA-256 |
| --- | --- |
| `launchservices.log` | `1ed6eec4d4518afc59e5a171143bf489c41cda66e01394a89818233f520c0eb9` |
| `direct-client.log` | `38a792f1c9c9a8d75c10fddd1c230899402c255ffead99b2835a925fe1eeb353` |
| `observations.jsonl` | `f725ce849cb3159b1624a241d4e08184aaa26d46e1045ff0e0a3d1da737bc2d4` |
| `launchservices-sample.txt` | `b175d57e724700b6e01fca9f04194706ed9291b0bee874275a090c80edaa4f8e` |

The signed bundle's client, server and `libvcmi.dylib` hashes are,
respectively, `376674ea89a68abc84a00e17450aee781e79d14bb59111324364507078490f37`,
`565c9da17fefde949bb8a8ec8d1141284e97ed697719261fc6860f822dccf322`, and
`e5453e7466150d5fb2d9ea1b6a2bdf6507d827359a56e93947b582c599582316`.
