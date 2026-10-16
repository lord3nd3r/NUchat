# NUchat Menus & Dialogs

Complete reference for all menu bar items, context menus, and dialogs.

---

## Menu Bar

### NUchat (Application)

| Item | Description |
|------|-------------|
| Network List | Open the network/server manager |
| Quick Connect | Fast connect dialog (server, port, nick, channel) |
| New Server Tab | Open a new server connection tab |
| New Channel Tab | Open a new channel tab |
| New Private Tab | Open a private message tab |
| Disconnect | Disconnect from the current server |
| Reconnect | Reconnect to the current server |
| Close Tab | Close the current tab |
| Quit | Exit NUchat |

### View

| Item | Description |
|------|-------------|
| Channel List | Browse channels on the server |
| DCC Transfers | View file transfer status |
| URL Grabber | View collected URLs from chat |
| Raw Log | View raw IRC protocol traffic |
| Plugins and Scripts | Manage loaded plugins/scripts |
| Away Log | Messages received while away |
| Search Text | Search within the current buffer |
| Fullscreen | Toggle fullscreen mode |

### Server

| Item | Description |
|------|-------------|
| Connect / Disconnect / Reconnect | Manage server connection |
| Join Channel | Join a channel by name |
| Part Channel | Leave the current channel |
| Change Nickname | Change your nick |
| Set Away / Set Back | Toggle away status |
| SASL Authentication | Configure SASL for current server |
| Server Password | Set server connection password |
| Send Raw Command | Opens the Raw Log dialog. It does not open a separate command prompt |

### Channel

| Item | Description |
|------|-------------|
| Set Topic | Edit channel topic |
| Channel Modes | View/set channel modes |
| Ban List | View channel bans |
| Clear Buffer | Clear the chat buffer |
| Save Buffer | Placeholder. Prints "Buffer save not yet implemented" |
| Invite User | Invite a user to the channel |
| Op / DeOp / Voice / DeVoice | Change user modes |
| Kick / Ban / Kick+Ban | Remove users |
| CTCP Ping / Version / Time / Finger | Send CTCP queries |

### User

| Item | Description |
|------|-------------|
| WHOIS / DNS Lookup / Ping | WHOIS sends `WHOIS`. DNS Lookup sends `USERHOST` (it is not a DNS query). Ping sends a CTCP PING |
| Send Private Message | Open a PM tab |
| Direct Chat (DCC) / Send File (DCC) | Both open the DCC transfers dialog. There is no DCC CHAT. `/DCC` implements `SEND` only |
| Ignore / Unignore / Ignore List | Manage ignore list |
| Notify List | Placeholder. Prints "Notify list not yet implemented". `/NOTIFY` still manages the list |

### Settings

| Item | Description |
|------|-------------|
| Preferences | Open the full settings editor |
| Themes | Submenu with all 25 themes |
| Next Theme / Previous Theme | Cycle through themes |
| Auto-Replace | Text replacement rules |
| URL Handlers | URL handling configuration |
| Text Events | Customize event format strings |
| Keyboard Shortcuts | Configure shortcuts |
| Identd Server | Placeholder. Prints "Identd server not yet implemented" |
| Perform List | Auto-commands on connect |

### Scripts

| Item | Description |
|------|-------------|
| Loaded Scripts | Open the Scripts dialog (Python and Lua) |
| Load Script | Python only. The dialog is titled "Load Python Script", filters `*.py`, and calls `pyEngine.loadScript`. Lua files dropped here are still passed to Python |
| Reload All Scripts | Calls `pyEngine.reloadAll()` only. Lua scripts are not reloaded. The Scripts dialog's Reload All button reloads both engines |
| Open Scripts Folder | Opens the Python engine's scripts directory when Python is built in |

### Window

| Item | Description |
|------|-------------|
| Previous Tab / Next Tab | Navigate tabs |
| Detach Tab / Attach Tab | Placeholders. Each prints that detach or attach is not yet implemented |
| Close Tab / Close All Tabs | Close tabs |

### Services

#### NickServ
Identify, Register, Ghost, Release, Set Password, Info, Help

#### ChanServ
Info, Topic, Mode, Access List (Add/Del/List), Transfer, Suspend, Help

#### OperServ
Channel Status (List Bans/Modes/Users), Extended (AKILL/RAKILL/SET/GET), Help

#### HostServ
Request vHost, On, Off, Info, Help

#### MemoServ
Send, Read, List, Del, Help

#### BotServ
Add Bot, Del Bot, List Bots, Help

#### Channel Settings
Mode toggles: `+i`, `+m`, `+s`, `+t`, `+n`, `+r`, `+k`, `+l`. Removal items exist for invite-only, moderated, secret, key, and limit. There is no menu item that removes `+t`, `+n`, or `+r`

#### Network Info
Links, Lusers, MOTD, Admin, Stats, Trace, Version, Time

#### Channel Stats
Modes, Topic, Who, Names

#### Oper Commands
Oper, Rehash, Restart, SQuit, GLINE, Kill, Wallops

### Help

| Item | Description |
|------|-------------|
| Documentation | Open docs |
| Keyboard Shortcuts | Print the current `shortcut/*` bindings into the active buffer. It does not open a dialog. Defaults are listed in [COMMANDS.md](COMMANDS.md) |
| Report a Bug | Open issue tracker |
| Check for Updates | Check for new versions |
| About NUchat | Version and credits |

---

## Right-Click Context Menus

### Nick Context Menu

Appears when right-clicking a nickname in the nick list or chat area. Supports multi-select — actions apply to all selected nicks.

| Item | Description |
|------|-------------|
| Open Query | Open a private message tab |
| WHOIS | Look up user info |
| CTCP Version / Ping / Time / Finger | Send CTCP queries |
| Op / DeOp / HalfOp / DeHalfOp | Operator mode changes |
| Voice / DeVoice | Voice mode changes |
| Invite | Invite to a channel |
| Slap | Slap with a large trout |
| Kick / Ban / Kick + Ban | Remove from channel |
| Ignore | Add to ignore list |
| **ChanServ →** | Op, DeOp, HalfOp, Voice, DeVoice, Kick, Ban, Unban, Kick+Ban, Quiet |
| **OperServ →** | Kill, AKILL, GLINE, KLINE |

### Link Context Menu

Appears when right-clicking a URL in the chat area.

| Item | Description |
|------|-------------|
| Open URL | Open in system browser |
| Copy Link | Copy URL to clipboard |
| Download Image | If the preview is already cached, copy it to Qt's download location (or `~/Downloads`). If it is not cached, a fetch starts and the buffer says it will be saved when ready; nothing connects that fetch to a save |

---

## Dialogs

| Dialog | Description |
|--------|-------------|
| **Network List** | Manage servers — 10 pre-configured networks, editable address/port/SSL/SASL/channels, per-network identity, drag-and-drop reorder. Server, SASL, NickServ, and ZNC passwords are stored in the OS keychain when QtKeychain is available |
| **Quick Connect** | Fast connect with server, port, nick, password, channel, SSL toggle |
| **Join Channel** | Join a channel by name with optional key |
| **Nick Change** | Change current nickname |
| **Channel List** | Browse and search server channel list |
| **DCC Transfers** | View DCC file transfer status |
| **Raw Log** | View raw IRC protocol messages. `PASS`, SASL `AUTHENTICATE` payloads, `OPER` passwords, and service `IDENTIFY`/`AUTH`/`REGISTER` commands are redacted |
| **URL Grabber** | Collected URLs from chat |
| **Search** | Search text within the current buffer |
| **Ban List** | View and manage channel bans |
| **Channel Modes** | View and set channel modes |
| **Topic** | Edit channel topic |
| **About** | Application info and version |
| **Preferences** | Full settings editor (18 tabs — see below) |
| **Invite User** | Invite a user to the current channel |
| **Server Password** | Set server password for connection |

---

## Preferences Tabs

| Tab | Key Settings |
|-----|-------------|
| **Interface** | Show/hide server tree, user list, topic bar, status bar, mode buttons. Open tabs in background. Confirm on close. Minimize to tray. |
| **Colors** | Theme selector. mIRC 16-color palette preview. Strip colors / nick coloring toggles. |
| **Text and Fonts** | Font family/size. Timestamps + format. Max scrollback. Indent. Highlight words. |
| **Input** | Spell check. Tab completion settings + suffix. Per-channel history + max size. |
| **User Info** | Primary nick + 3 alternates. Username, real name. Quit/part/away messages. |
| **Connection** | Auto-reconnect (initial delay, then exponential backoff capped at 15 minutes, max attempts). A connect that does not finish within 30 seconds is aborted. Auto-join. Proxy (SOCKS5, HTTP CONNECT; SOCKS4 is refused). Accept self-signed SSL certificates (`conn/allowSelfSignedCerts`) — hostname mismatches, self-signed certs, and untrusted issuers only. |
| **SASL / Auth** | The method combo lists None, SASL PLAIN, SASL EXTERNAL, SASL SCRAM-SHA-256, SASL ECDSA-NIST256P, two NickServ spellings, Server Password, and CERTFP. It saves `auth/methodIndex` and nothing reads that key. A connection uses the network's `saslMethod`, and only `PLAIN` and `EXTERNAL` are implemented. PEM client certificate and unencrypted key are sent on the TLS handshake. SASL PLAIN responses are split into 400-byte `AUTHENTICATE` chunks. |
| **DCC** | Download folder. Auto-accept (public peers only, unless private-LAN peers are allowed). Max file size. Listen port range (`dcc/portLow`–`dcc/portHigh`, default 1024–5000). Advertised address: the IRC connection's local IPv4, or a manual IPv4 when that method is selected. The listen socket still binds to the local address. Loopback and non-unicast offers are rejected. Receives follow the proxy setting or fail closed. Active sends are refused while a proxy is on. |
| **Logging** | Enable, directory, timestamps, private messages, and per-channel files are read by the logger. The format combo is disabled and only offers plain text; there is no `log/formatIndex` key. An empty `log/directory` uses Qt's application config location plus `/logs`. The field's unset text is `~/.config/NUchat/logs`, and saving Preferences writes that string. Rotation is 10 MB (`.log.1` through `.log.5`). |
| **Notifications** | Desktop notifications. Flash taskbar. Tray unread. Highlight on nick mention. Timeout. |
| **Sounds** | Enable/disable. Per-event toggles (highlight, PM, connect). Custom beep command. |
| **Auto-Replace** | Pattern/replacement rules for outgoing text. |
| **URL Handlers** | Clickable URLs. Auto-grab. Custom browser command. Inline image previews are off by default (http/https only, 15 MB limit). Private and special-use addresses are blocked, including after a redirect. A configured proxy that cannot be applied skips the fetch. |
| **Text Events** | Format strings for join, part, quit, kick, nick change, topic, action, notice, CTCP. |
| **Plugins** | The list is a static placeholder (`ExamplePlugin`). Load, Unload, and Reload have no handlers. The directory field is disabled. Saving Preferences still writes `plugin/directory`. The loader scans that key when it is non-empty. |
| **Scripts** | The list is a static "No scripts loaded" label. Load, Unload, and Open Script Editor have no handlers. The directory field is disabled. `script/directory` is written on OK and is not read. Python and Lua load `~/.config/NUchat/scripts/`. The working controls are the Scripts menu and the Scripts dialog. The migration wizard button on this tab does open the wizard. |
| **Advanced** | Encoding, IPv6, identify-before-join, turning CAP off, and the command character are disabled. CAP stays on. Perform commands are read. Raw IRC in the server tab is `adv/showRawIRC`. |
| **Shortcuts** | Edits the `shortcut/*` sequences listed in [COMMANDS.md](COMMANDS.md). Clear a field to disable that shortcut. |
