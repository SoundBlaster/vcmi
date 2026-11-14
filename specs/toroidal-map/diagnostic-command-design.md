# Experimental local diagnostic command channel

Status: implemented, with standalone socket tests and bounded interactive
evidence on an earlier build. This facility supports interactive diagnostics for the
toroidal experiment and is not ASP, an agent authorization mechanism, or a
multiplayer security boundary.

See [verification and the outstanding GUI check](evidence/diagnostic-command-channel-2026-11-14.md).

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

## Local use

Create a directory accessible only to the current user, then set
`VCMI_DIAGNOSTIC_SOCKET` to an absolute path inside it **before** launching
the game. The path must fit the platform's Unix socket limit. If initialization
fails, the feature remains disabled and the path is left untouched.

Send one event using `specs/toroidal-map/tools/vcmi_command.py --socket PATH`:

```text
vcmi_command.py --socket PATH click X Y
vcmi_command.py --socket PATH shortcut cast
vcmi_command.py --socket PATH shortcut southeast
```

`accepted` means only that an event was delivered to the dispatcher. The
observation layer and the game screen are separate sources of evidence. The
channel processes at most one client at a time, accepts one line of up to 256
bytes per connection, and drops an incomplete request after two seconds. It
does not automatically remove pre-existing paths. A private directory also
prevents other users on the machine from connecting before socket permissions
are restricted.
