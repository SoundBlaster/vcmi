# Experimental local diagnostic command channel

Status: planned. This facility supports interactive diagnostics for the
toroidal experiment and is not ASP, an agent authorization mechanism, or a
multiplayer security boundary.

An explicitly enabled local Unix-domain socket accepts a single bounded
command per connection. It is disabled by default and must not replace an
existing socket or file. The socket is owner-only; use a private directory.
Clients with access as the same OS user are trusted, including processes that
can manipulate that user's files. This is not isolation from other processes
running as that user.

Commands deliver a logical GUI click or an allowlisted game shortcut to the
existing GUI event dispatcher on the GUI thread. They must respect the current
window, normal client behavior, and server validation. Screen coordinates are
logical VCMI GUI coordinates, not screenshot pixels. The response acknowledges
event dispatch, not successful gameplay, and observers must establish outcomes
separately. A lost response is indeterminate: never automatically retry a
potentially state-changing command. Connections and requests are bounded and
must not block the render loop. Malformed or unsupported commands are rejected.

The initial slice targets macOS and other Unix systems; unsupported platforms
remain disabled. Semantic game actions, menu discovery, agent identity, grants,
fog-filtered authoritative observations, remote transport, and ASP integration
remain future work.
