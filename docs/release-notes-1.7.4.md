# NUchat 1.7.4

### Fixed
- Lag that grew over time: the chat view no longer rebuilds its whole text on almost every message once the scrollback is full. It now counts messages instead of wrapped lines and rebuilds only occasionally.
- Auto-reconnect now also kicks in when a connection is refused or DNS or the proxy fails, not just after a disconnect.
- Reconnecting a server no longer switches your active network or channel.
- DCC no longer keeps a dangling pointer to a deleted connection.
- Authenticated proxies now get the real password from the keychain instead of an empty one.
- Logging no longer fsyncs every line on the UI thread. Files are synced every 2 seconds and on quit.
- The Lua script memory limiter no longer drifts until every allocation fails.

### Security
- Channel topics are escaped and linkified like chat text instead of being shown as raw rich text.
- Opening links only allows http, https, irc, ircs and mailto.
- Windows, macOS and FreeBSD release builds now include QtKeychain, and release builds fail instead of silently storing passwords in plain text.

