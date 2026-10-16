# Changelog

All notable changes to NUchat are documented in this file.

## [Unreleased]

### Security
- Outbound IRC lines have CR, LF, and NUL stripped before they are sent, so one call cannot inject a second command.
- Debug logs redact `PASS`, `OPER`, SASL `AUTHENTICATE` payloads, and NickServ `IDENTIFY` / credential commands.
- "Accept self-signed" ignores only self-signed certificates, hostname mismatches, and untrusted issuer chains. Expired certificates and other hard failures still abort the connection.
- A configured proxy with an empty host, an invalid port, or SOCKS4 no longer falls back to a direct connection or to SOCKS5.
- Inline image previews are off by default. The downloader blocks private and special-use addresses, checks each redirect before following it, validates image bytes before caching, and caps the on-disk cache.
- DCC offers to loopback, unspecified, link-local, and other non-unicast addresses are rejected. Receives use the configured proxy or fail closed. Active sends bind to the IRC connection's IPv4 address, time out, and partial downloads are deleted on failure.

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
