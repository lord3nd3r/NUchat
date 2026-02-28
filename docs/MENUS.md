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
| Send Raw Command | Send a raw IRC command |

### Channel

| Item | Description |
|------|-------------|
| Set Topic | Edit channel topic |
| Channel Modes | View/set channel modes |
| Ban List | View channel bans |
| Clear Buffer | Clear the chat buffer |
| Save Buffer | Save chat to file |
| Invite User | Invite a user to the channel |
| Op / DeOp / Voice / DeVoice | Change user modes |
| Kick / Ban / Kick+Ban | Remove users |
| CTCP Ping / Version / Time / Finger | Send CTCP queries |

### User

| Item | Description |
|------|-------------|
| WHOIS / DNS Lookup / Ping | User information queries |
| Send Private Message | Open a PM tab |
| Direct Chat (DCC) / Send File (DCC) | DCC operations |
| Ignore / Unignore / Ignore List | Manage ignore list |
| Notify List | Manage nick notify list |

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
| Identd Server | Toggle identd |
| Perform List | Auto-commands on connect |

### Window

| Item | Description |
|------|-------------|
| Previous Tab / Next Tab | Navigate tabs |
| Detach Tab / Attach Tab | Tear off / reattach tabs |
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
Mode toggles: `+i`, `+m`, `+s`, `+t`, `+n`, `+r`, `+k`, `+l` (and removal)

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
| Keyboard Shortcuts | Show shortcut reference |
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
| Download Image | Save image to ~/Downloads |

---

## Dialogs

| Dialog | Description |
|--------|-------------|
| **Network List** | Manage servers — 10 pre-configured networks, editable address/port/SSL/SASL/channels, per-network identity, drag-and-drop reorder |
| **Quick Connect** | Fast connect with server, port, nick, password, channel, SSL toggle |
| **Join Channel** | Join a channel by name with optional key |
| **Nick Change** | Change current nickname |
| **Channel List** | Browse and search server channel list |
| **DCC Transfers** | View DCC file transfer status |
| **Raw Log** | View raw IRC protocol messages |
| **URL Grabber** | Collected URLs from chat |
| **Search** | Search text within the current buffer |
| **Ban List** | View and manage channel bans |
| **Channel Modes** | View and set channel modes |
| **Topic** | Edit channel topic |
| **About** | Application info and version |
| **Preferences** | Full settings editor (17 tabs — see below) |
| **Emoji Picker** | Browse and insert emoji |
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
| **Connection** | Auto-reconnect (delay, max attempts). Auto-join. Proxy (SOCKS4/5, HTTP). |
| **SASL / Auth** | Auth method (PLAIN, EXTERNAL, SCRAM, ECDSA, NickServ, CERTFP). Certificate paths. |
| **DCC** | Download folder. Auto-accept. Max file size. IP detection. Port range. |
| **Logging** | Enable/disable. Directory. Format (Plain/HTML/JSON). PM logging. Per-channel files. |
| **Notifications** | Desktop notifications. Flash taskbar. Tray unread. Highlight on nick mention. Timeout. |
| **Sounds** | Enable/disable. Per-event toggles (highlight, PM, connect). Custom beep command. |
| **Auto-Replace** | Pattern/replacement rules for outgoing text. |
| **URL Handlers** | Clickable URLs. Auto-grab. Custom browser command. |
| **Text Events** | Format strings for join, part, quit, kick, nick change, topic, action, notice, CTCP. |
| **Plugins** | Load/unload/reload C++ plugins. Plugin directory path. |
| **Scripts** | Load/unload/reload scripts. Script editor. Script directory. |
| **Advanced** | Encoding (UTF-8 + alternatives). IPv6. CAP. Perform commands. Command character. |
