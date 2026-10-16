<!-- Suggested GitHub release title: NUchat 1.7.0 -->
<!-- Suggested tag: v1.7.0 (create the tag and the release only after this is merged) -->
<!-- Paste everything below this comment into the GitHub release body. -->

## NUchat 1.7.0

Release since [1.6.1](https://github.com/lord3nd3r/NUchat/releases/tag/v1.6.1). Full notes are in [CHANGELOG.md](https://github.com/lord3nd3r/NUchat/blob/main/CHANGELOG.md).

### Highlights

- **Plugins and scripts work.** C++ plugins load from the build, install, and user plugin directories. `/command` and `!command` in `PRIVMSG`/`NOTICE` are dispatched. Python and Lua server hooks match the real IRC command (including IRCv3 tags and an unprefixed `PING`), `EAT_*` is honored, and hook callbacks can no longer crash the client by editing the hook list or deleting their own timer. JavaScript `onMessage` receives the nick and the message text.
- **Network hardening.** Outbound lines have CR, LF, and NUL stripped. Debug logs, the raw log, and the chat echo redact `PASS`, `OPER`, SASL `AUTHENTICATE`, and service passwords. "Accept self-signed" covers only self-signed certificates, hostname mismatches, and untrusted issuer chains. A proxy that cannot be applied fails closed. Inline images are off by default, and fetches block private addresses, check redirects, validate image bytes, and cap the cache. DCC rejects non-unicast targets, times out, and deletes partial downloads.
- **Passwords move to the OS keychain** (QtKeychain) when it is available. Existing `NUchat.conf` secrets are copied out only after a successful write. If the keychain is missing or the write fails, the plaintext stays in the config.
- **Protocol.** ISUPPORT `PREFIX`, `CHANTYPES`, `CHANMODES`, and `CASEMAPPING` are applied. `server-time` stamps are used. SASL `AUTHENTICATE` is chunked at 400 bytes. Connects time out after 30 seconds, and reconnect delay doubles up to 15 minutes. `//text` sends `/text`. DCC can listen in a port range and advertise a manual IPv4. A PEM client certificate from Preferences is presented on the TLS handshake. Hunspell dictionaries are searched on Linux, macOS, and Windows.
- **Buffers, preferences, and text.** Numeric replies and away state stay on the server that produced them. Many preferences that were stored and ignored are now applied (logging, DCC download directory and size, URL handling, highlights, tray unread, and others). Controls that still cannot take effect are disabled in the dialog. IRC background colors render, message HTML is escaped, and an image embed stays on the buffer that requested it.
- **Build and CI.** The version header is generated from the CMake project version. `ctest` runs on Linux, Windows, macOS, and FreeBSD, plus a Linux AddressSanitizer / UndefinedBehaviorSanitizer job.

### Upgrade notes

- Saved server, SASL, NickServ, ZNC, and proxy passwords are migrated into the OS keychain on the next read when QtKeychain is linked and the write succeeds. A build without QtKeychain keeps them in `NUchat.conf`.
- Inline image previews stay off until you enable them under Preferences → URL Handlers. A saved `true` is unchanged.
- SOCKS4, an empty proxy host, and an invalid proxy port no longer connect directly. Active DCC send is refused while a proxy is configured.
- Log files rotate through `.log.1` … `.log.5`. A network or channel named `..` is stored as `_`.

### Downloads

Assets are published with the `v1.7.0` tag. The same names are linked from the README:

| Platform | Archive | Contents |
|----------|---------|----------|
| Linux | `nuchat-linux.zip` | `nuchat` |
| FreeBSD | `nuchat-freebsd.zip` | `nuchat` |
| macOS | `nuchat-macos.zip` | `nuchat-macos.dmg` |
| Windows | `nuchat-windows-installer.zip` | `NUchat_1.7.0_Windows_x64.exe` |

- https://github.com/lord3nd3r/NUchat/releases/download/v1.7.0/nuchat-linux.zip
- https://github.com/lord3nd3r/NUchat/releases/download/v1.7.0/nuchat-freebsd.zip
- https://github.com/lord3nd3r/NUchat/releases/download/v1.7.0/nuchat-macos.zip
- https://github.com/lord3nd3r/NUchat/releases/download/v1.7.0/nuchat-windows-installer.zip
