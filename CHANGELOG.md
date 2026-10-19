# Changelog

All notable changes to NUchat are documented in this file.

## [1.7.1] — 2026-10-19

### Fixed
- High CPU usage: removed the infinitely looping highlight pulse animation in the channel list, which forced the window to redraw every frame.
- A live message no longer restarts the 50 ms scroll-settle timer; it scrolls once. The timer only runs after a full buffer reload.
- Default scrollback lowered from 10000 to 2000 lines (adjustable in Preferences).

## [1.7.0] — 2026-10-16

### Security
- **OS keychain for passwords.** Server `PASS`, SASL, NickServ, ZNC, and proxy passwords are stored via QtKeychain when it is available. Existing `NUchat.conf` values are migrated on read and removed from the file only after the keychain accepts them. Non-password settings are unchanged. If QtKeychain is not linked, or a keychain write fails, the plaintext values stay in the config so nothing is lost.
- **Credentials are redacted** in the chat buffer, channel logs, the raw log, and debug traces. `/msg NickServ IDENTIFY`, and the same shape of command (`AUTH`, `REGISTER`, `GHOST`, `SET PASSWORD`, …) to NickServ and the other service nicks, still go out on the wire, but the password is shown as `***`. `PASS`, SASL `AUTHENTICATE` payloads, and `OPER` passwords are redacted the same way. Inbound lines delivered to plugins and scripts are not modified.
- Outbound IRC lines have CR, LF, and NUL stripped before they are sent, so one call cannot inject a second command.
- "Accept self-signed" ignores only self-signed certificates, hostname mismatches, and untrusted issuer chains. Expired certificates and other hard failures still abort the connection.
- A configured proxy with an empty host, an invalid port, or SOCKS4 no longer falls back to a direct connection or to SOCKS5. Image fetches and DCC receives use that proxy or fail closed. An active DCC send is refused while a proxy is configured, because it listens for a direct connection.
- Inline image previews are off by default. The downloader allows only http(s), blocks private and special-use addresses (including decimal and hex literals), checks each redirect before following it, validates image bytes before caching, and caps the on-disk cache at 100 MB.
- DCC offers to port 0, loopback, unspecified, link-local, and other non-unicast addresses are rejected. Private-LAN offers stay pending for a manual accept unless `dcc/allowPrivatePeers` is set. Receives time out on connect and on stall. Active sends bind to the IRC connection's IPv4 address, give up when nobody connects or progress stops, and partial downloads are deleted on failure. Filenames lose path segments, leading dots, and Windows reserved names. More than 32 active transfers are refused.

### Fixed
- **C++ plugins load.** The example plugin declares `Q_PLUGIN_METADATA`. The loader searches `plugins/` in the build tree, the install plugin directory, `~/.config/NUchat/plugins`, and a configured `plugin/directory`. `/command` is dispatched before Python and Lua, and a `PRIVMSG` or `NOTICE` body of `!command` is recognized after IRCv3 tags.
- **Script hooks.** Python and Lua `hook_server` match the IRC command after message tags and an optional prefix, including an unprefixed `PING`. `EAT_HEXCHAT` and `EAT_ALL` skip NUchat's own handling of that line, including the built-in command table and auto-`PONG`. `EAT_PLUGIN` stops later hooks and still lets NUchat handle the event. Hooks run from a priority snapshot, so a callback can add or remove hooks without crashing the walk, and a timer that removes itself is stopped with `deleteLater()`.
- **JavaScript.** The engine loads the in-tree `scripts/` directory, the install `share/nuchat/scripts` path, and `~/.config/NUchat/scripts/`. `onMessage` receives the nick and the `PRIVMSG`/`NOTICE` text.
- **Per-server routing.** Numeric replies (WHOIS, WHOWAS, away, and the rest) and away state stay on the server that produced them. An unmapped numeric is dropped instead of being filed under `unknown`. Unread keys, the server tree, and nick and channel identity use case-insensitive comparison.
- **Color and text.** `\x03` foreground/background and `\x04` hex colors render, and reverse video swaps them. Message text is HTML-escaped. An inline image is stored on the buffer that requested it and is not painted onto a different tab if the download finishes after a switch.
- **Log paths and lines.** A path component of `.` or `..` cannot leave the log directory. Trailing dots and spaces are removed so `..` is stored as `_` on every platform. New log directories and files are owner-only where the platform can express those bits. Lines strip C0 controls, ANSI escapes, and mIRC formatting, writes are flushed and `fsync`'d, rotation keeps `.log.1` through `.log.5`, and every new line ends in `\n`.

### Protocol
- **ISUPPORT** — `PREFIX`, `CHANTYPES`, `CHANMODES`, and `CASEMAPPING` (`ascii`, `rfc1459`, `strict-rfc1459`) are parsed and used for channel detection, nick prefixes, mode parameters, and nick/channel identity. Until 005 arrives the client assumes rfc1459 and the usual `#&` / `qaohv` defaults.
- **server-time** timestamps are applied to messages that carry a `time` tag.
- **SASL** `AUTHENTICATE` payloads are split into 400-byte chunks. A PEM client certificate and unencrypted key from Preferences are presented on the TLS handshake.
- Connections that do not complete within 30 seconds are aborted. Auto-reconnect waits the configured delay on the first attempt and doubles it after each later attempt, up to 15 minutes.
- `//text` sends the literal message `/text`. A token such as `/etc/passwd` is sent as text rather than as a command. Multi-line pastes of more than three lines still ask for confirmation when they start with `/`.
- `/TIMER` rejects non-finite, non-positive, and overflowing delays before they are converted to milliseconds.
- Spell checking looks for Hunspell dictionaries in `DICPATH`, next to the application, Homebrew locations, system paths, and the user data directory on Windows, macOS, and Linux. `en_US` and `en-US` names both match.
- DCC sends listen in `dcc/portLow`–`dcc/portHigh` (default 1024–5000) and can advertise a manual IPv4 while still binding to the IRC connection's local address. DCC filenames treat `\` the same on every platform: it is escaped, not used as a directory separator.

### Preferences
- Logging enable, directory (with `~` expansion), timestamps, per-channel files, and private-message filtering are read by the logger. Download directory, auto-accept, and max size (`dcc/maxSize` in megabytes, `0` for unlimited; legacy `dcc/maxFileSize` in bytes is still accepted) are applied to DCC.
- URL grabber, clickable links, browser command, tab completion, history length, confirm-close, tabs-in-background, nick highlight, highlight words, nick colors, raw IRC in the server tab, and tray unread are wired.
- Controls that still cannot take effect stay disabled and say so: text encoding, IPv6, identify-before-join, turning CAP off, the command character, HTML/JSON log format, the nick-completion popup, hanging indent, and taskbar flash.

### Build
- `src/Version.h` is generated from `project(NUchat VERSION ...)` in the top-level `CMakeLists.txt`. Single-config generators default to Release. Warnings are `-Wall -Wextra` (MSVC `/W4`). `HAVE_PYTHON` and `HAVE_LUA` are public on the core library, same as `HAVE_HUNSPELL`. CPack still takes its version from that project version, so the Windows installer name follows it.
- CI limits the workflow token, pins actions, caches ccache and Qt, and runs `ctest` on Linux, Windows, macOS, and FreeBSD. A separate Linux job runs the tests under AddressSanitizer and UndefinedBehaviorSanitizer. Widget tests use the offscreen Qt platform so a headless runner does not need a display. Windows test binaries stay console programs.
- README, command, menu, plugin, and scripting docs match the TLS, proxy, image, DCC, plugin, and scripting behavior. `THIRD-PARTY.md` records dependency licenses. Unused `IRCClient` sources and the placeholder QML files are removed.

## [1.6.1] — 2026-08-27

### Added
- **Channel list streaming & filtering** — wired `RPL_LIST` (322) and `RPL_LISTEND` (323) numeric signals to populate the channel list dialog dynamically with search filtering and size protection.
- **RPL_AWAY (301) response handling** — surfaces target away messages directly in the active buffer or WHOIS output.

### Improved
- **Text memory lifecycle & scrollback bounding** — dynamic enforcement of user-configured `ui/maxScrollback` limits in the active message model and per-channel history buffers to prevent unbounded heap growth in long sessions.
- **QTextDocument memory recycling** — chat area safely re-indexes its document text when exceeding scrollback limit headroom while maintaining exact cursor and scroll position.
- **Raw socket buffer protection** — added a 64 KB safety threshold in the IRC read buffer to discard corrupted newline-less stream floods.
- **Raw log buffering** — capped the raw traffic dialog model at 1,000 entries.
- **WHOIS buffer cleanup** — automatic eviction and cleanup on `ERR_NOSUCHNICK` (401) or aborted queries.

## [1.6.0] — 2026-07-08

### Added
- **Network display names** — networks can carry a custom label shown in the
  sidebar instead of the raw hostname.
- **Collapsible sidebar** for more chat room on small windows.
- **Notification rate limiting** — desktop balloons and sounds are throttled
  so a busy channel can't spam the system tray.
- **Auto-focus message input** on channel switch and window activation —
  typing anywhere goes straight to the input bar.
- `hexchat.hook_unload()` support in the Python scripting API.
- Expanded HexChat import (more of `servlist.conf` / identity settings carried
  over).
- Regression tests for IRCv3 message-tag parsing.

### Fixed
- **Scrollback no longer appears twice** when joining a channel that has
  existing log history (the log was loaded once by the join handler and again
  by the scrollback loader).
- **Crash-prone plugin/script wiring** — the raw-line hook for plugins and
  scripts captured a dangling reference to the connection object (undefined
  behavior on every received line).
- **DCC receive hardening** — offers with an invalid (zero/negative) file size
  are rejected, and a transfer is aborted if the peer sends more data than it
  offered, so a malicious sender can no longer fill the disk.
- **Private messages and CTCPs addressed with different nick casing** (e.g.
  `MyNick` vs `mynick`) now route to the correct query tab instead of opening
  a bogus channel tab.
- **IRCv3 message-tag values** are now unescaped in a single left-to-right
  pass per spec (`\\s` no longer double-decodes; invalid escapes and trailing
  backslashes handled per spec).
- **Server passwords containing spaces** now work (`PASS` is colon-framed).
- `/NOTIFY -nick` removal is now case-insensitive, matching the add check.
- **Channel-mode prefix tracking** — parametered channel modes (`+b/+e/+I/+k`
  always, `+l/+f/+j` when setting) now keep the mode-parameter index aligned,
  so op/voice prefix updates no longer mis-align after mixed mode strings.
- **Lag meter** — only a genuine `PONG …LAG<id>` reply is accepted (chat text
  mentioning PONG/LAG no longer skews the measurement), and the meter stops
  based on live connections rather than open tabs.
- `/SYSINFO` no longer leaves a stray child process behind if a probe command
  hangs.
- Reconnecting the same connection object with a different SSL mode no longer
  risks sending registration before the TLS handshake completes.
- Focus-stealing fixed: the chat area no longer grabs keyboard focus on
  scroll.
- PM/query right-click context menus target the correct user; QML load error
  in the nick context menu fixed.
- DCC send, multi-server reconnect, and highlight fixes from the 1.5.x code
  review follow-up.
- Null-safety fixes on application exit.

## [1.5.0] — 2026-06-10

First tagged release with pre-built binaries for Linux, Windows, macOS, and
FreeBSD. See the
[v1.5.0 release page](https://github.com/lord3nd3r/NUchat/releases/tag/v1.5.0)
for details and downloads.
