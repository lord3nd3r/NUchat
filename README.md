# NUchat

NUchat is a full-featured IRC client built with Qt 6 and QML. It aims to provide the
functionality of established clients like HexChat in a modern, themeable interface with
SSL/TLS, SASL authentication, inline image previews, clickable URLs, a plugin system,
and JavaScript scripting.

---

## Table of Contents

- [Screenshots](#screenshots)
- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
- [Running](#running)
- [Configuration](#configuration)
- [Themes](#themes)
- [Menu Reference](#menu-reference)
- [IRC Commands](#irc-commands)
- [Right-Click Menus](#right-click-menus)
- [Dialogs](#dialogs)
- [Preferences](#preferences)
- [Plugins and Scripts](#plugins-and-scripts)
- [Project Structure](#project-structure)
- [TODO](#todo)
- [License](#license)

---

## Screenshots

*(Screenshots to be added)*

---

## Features

- Qt 6 / QML interface with C++17 backend
- SSL/TLS on all connections (port 6697 by default)
- SASL authentication: PLAIN, EXTERNAL, SCRAM-SHA-256, ECDSA-NIST256P-CHALLENGE
- CAP LS 302 capability negotiation
- 25 built-in color themes (HexChat Dark, Monokai, Dracula, Nord, Gruvbox, Catppuccin, and more)
- mIRC color code rendering (foreground, background, bold, italic, underline, strikethrough, reverse, hex colors)
- Clickable URLs in chat with inline image previews for image links
- Right-click context menu on links: Open URL, Copy Link, Download Image
- Right-click context menu on nicks: WHOIS, CTCP, Op/DeOp, Kick/Ban, ChanServ and OperServ submenus
- Tab nick-completion with cycling
- Server/channel tree sidebar with collapsible servers
- Per-channel message history preserved when switching tabs
- Topic bar with rich text rendering
- Nick list with color-coded prefixes (owner, admin, op, halfop, voice)
- 10 pre-configured IRC networks (Libera.Chat, OFTC, EFnet, DALnet, Undernet, Rizon, IRCnet, QuakeNet, Snoonet, freenode)
- Per-network identity override (nickname, username, real name)
- Quick Connect dialog with remembered settings
- Network List with full server editing (address, port, SSL, SASL, auto-join channels)
- 60+ IRC command aliases
- Services menus: NickServ, ChanServ, OperServ, HostServ, MemoServ, BotServ
- Channel mode management (invite-only, moderated, secret, topic lock, key, limit)
- C++ plugin system with runtime loading
- JavaScript scripting engine
- Chat logging (plain text, HTML, JSON)
- Desktop notifications on highlights and private messages
- Window geometry and theme persistence via QSettings
- Desktop integration (.desktop file and SVG/PNG icons at multiple sizes)
- Fullscreen mode

---

## Requirements

- Linux (tested on Ubuntu)
- CMake 3.16 or later
- A C++17 compiler (GCC 9+ or Clang 10+)
- Qt 6.2 or later with the following modules:
  - Core, Gui, Widgets, Network, Concurrent
  - Quick, QuickControls2, Declarative (for the QML UI)

### Installing dependencies on Ubuntu/Debian

```
sudo apt install build-essential cmake \
  qt6-base-dev qt6-declarative-dev qt6-quickcontrols2-dev \
  libgl1-mesa-dev
```

### Installing dependencies on Fedora

```
sudo dnf install cmake gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquickcontrols2-devel \
  mesa-libGL-devel
```

### Installing dependencies on Arch Linux

```
sudo pacman -S cmake qt6-base qt6-declarative qt6-quickcontrols2
```

---

## Building

```
git clone https://github.com/lord3nd3r/NUchat.git
cd NUchat
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The executable will be at `build/src/NUchat`.

### Build options

CMake will warn if Qt6 Quick is not found. The core library can compile without it
(headless mode), but the QML UI requires Quick and QuickControls2.

### Running tests

```
cd build
ctest --output-on-failure
```

---

## Running

From the build directory:

```
./src/NUchat
```

Or install the desktop entry for launcher integration:

```
cp resources/nuchat.desktop ~/.local/share/applications/
cp resources/icons/nuchat.svg ~/.local/share/icons/hicolor/scalable/apps/
```

---

## Configuration

All settings are stored via QSettings at:

```
~/.config/NUchat/NUchat.conf
```

Cached images from inline previews are stored at:

```
~/.cache/NUchat/images/
```

Settings persist automatically. The config file stores theme selection, window geometry,
user identity, network list, quick connect history, and all preferences.

---

## Themes

25 built-in themes, each defining 50+ color properties. Change themes via
Settings > Themes in the menu bar, or cycle with Next Theme / Previous Theme.

| # | Theme |
|---|-------|
| 0 | HexChat Dark (default) |
| 1 | Monokai |
| 2 | Solarized Dark |
| 3 | Solarized Light |
| 4 | Dracula |
| 5 | Nord |
| 6 | Gruvbox Dark |
| 7 | Gruvbox Light |
| 8 | One Dark |
| 9 | One Light |
| 10 | Catppuccin Mocha |
| 11 | Catppuccin Latte |
| 12 | Tokyo Night |
| 13 | Material Dark |
| 14 | Cyberpunk |
| 15 | Retro Green |
| 16 | Retro Amber |
| 17 | Zenburn |
| 18 | Ayu Dark |
| 19 | GitHub Dark |
| 20 | Midnight Blue |
| 21 | Rose Pine |
| 22 | Everforest Dark |
| 23 | Ice |
| 24 | High Contrast |

---

## Menu Reference

### NUchat (application menu)

- Network List -- Open the network/server manager
- Quick Connect -- Fast connect dialog (server, port, nick, channel)
- New Server Tab / New Channel Tab / New Private Tab
- Disconnect / Reconnect
- Close Tab
- Quit

### View

- Channel List, DCC Transfers, URL Grabber, Raw Log
- Plugins and Scripts, Away Log
- Search Text, Fullscreen

### Server

- Connect / Disconnect / Reconnect
- Join Channel / Part Channel
- Change Nickname / Set Away / Set Back
- SASL Authentication / Server Password / Send Raw Command

### Channel

- Set Topic / Channel Modes / Ban List
- Clear Buffer / Save Buffer
- Invite User
- Op / DeOp / Voice / DeVoice / Kick / Ban / Kick+Ban
- CTCP Ping / Version / Time / Finger

### User

- WHOIS / DNS Lookup / Ping
- Send Private Message / Direct Chat (DCC) / Send File (DCC)
- Ignore / Unignore / Ignore List
- Notify List

### Settings

- Preferences
- Themes (submenu with all 25 themes)
- Next Theme / Previous Theme
- Auto-Replace / URL Handlers / Text Events / Keyboard Shortcuts
- Identd Server / Perform List

### Window

- Previous Tab / Next Tab
- Detach Tab / Attach Tab
- Close Tab / Close All Tabs

### Services

- **NickServ**: Identify, Register, Ghost, Release, Set Password, Info, Help
- **ChanServ**: Info, Topic, Mode, Access List (Add/Del/List), Transfer/Suspend, Help
- **OperServ**: Channel Status (List Bans/Modes/Users), Extended (AKILL/RAKILL/SET/GET), Help
- **HostServ**: Request vHost, On, Off, Info, Help
- **MemoServ**: Send, Read, List, Del, Help
- **BotServ**: Add Bot, Del Bot, List Bots, Help
- **Channel Settings**: Mode toggles (+i, +m, +s, +t, +n, +r, +k, +l and their removal)
- **Network Info**: Links, Lusers, MOTD, Admin, Stats, Trace, Version, Time
- **Channel Stats**: Modes, Topic, Who, Names
- **Oper Commands**: Oper, Rehash, Restart, SQuit, GLINE, Kill, Wallops

### Help

- Documentation, Keyboard Shortcuts, Report a Bug
- Check for Updates, About NUchat

---

## IRC Commands

All commands are case-insensitive. Unknown commands are sent to the server as raw IRC.

| Command | Aliases | Description |
|---------|---------|-------------|
| `/JOIN` | `/J` | Join a channel (with optional key) |
| `/PART` | `/LEAVE`, `/P` | Leave a channel |
| `/NICK` | | Change nickname |
| `/MSG` | `/PRIVMSG`, `/M`, `/QUERY`, `/Q` | Send private message or open query tab |
| `/ME` | | Send a CTCP ACTION |
| `/QUIT` | `/DISCONNECT`, `/BYE` | Disconnect from server |
| `/AWAY` | | Set away message (no args to unset) |
| `/BACK` | | Remove away status |
| `/WHOIS` | `/W`, `/WI` | WHOIS lookup |
| `/RAW` | `/QUOTE` | Send raw IRC command |
| `/TOPIC` | `/T` | Get or set channel topic |
| `/MODE` | | Set or query channel/user modes |
| `/KICK` | | Kick user from channel |
| `/NOTICE` | | Send a notice |
| `/NS` | `/NICKSERV` | Message NickServ |
| `/CS` | `/CHANSERV` | Message ChanServ |
| `/OS` | `/OPERSERV` | Message OperServ |
| `/HS` | `/HOSTSERV` | Message HostServ |
| `/MS` | `/MEMOSERV` | Message MemoServ |
| `/BS` | `/BOTSERV` | Message BotServ |
| `/HELPSERV` | | Message HelpServ |
| `/OP` | | Give operator status (+o) |
| `/DEOP` | | Remove operator status (-o) |
| `/HALFOP` | `/HOP` | Give half-operator (+h) |
| `/DEHALFOP` | `/DEHOP` | Remove half-operator (-h) |
| `/VOICE` | `/V` | Give voice (+v) |
| `/DEVOICE` | | Remove voice (-v) |
| `/BAN` | `/B` | Ban user (auto-builds hostmask) |
| `/UNBAN` | | Remove ban |
| `/KB` | `/KICKBAN` | Kick and ban user |
| `/INVITE` | `/INV` | Invite user to channel |
| `/CYCLE` | `/REJOIN` | Part and rejoin channel |
| `/CLEAR` | | Clear message buffer |
| `/CLOSE` | | Part channel and close tab |
| `/CTCP` | | Send arbitrary CTCP message |
| `/SLAP` | | Slap someone with a large trout |
| `/OPER` | | Authenticate as IRC operator |
| `/KILL` | | Kill user (oper only) |
| `/GLINE` | | Global ban (oper only) |
| `/KLINE` | | Local ban (oper only) |
| `/ZLINE` | | IP ban (oper only) |
| `/DLINE` | | IP range ban (oper only) |
| `/WALLOPS` | | Send wallops message (oper only) |
| `/SQUIT` | | Split server link (oper only) |
| `/REHASH` | | Rehash server config (oper only) |
| `/LINKS` | | List server links |
| `/LUSERS` | | Network statistics |
| `/MOTD` | | Display Message of the Day |
| `/ADMIN` | | Show admin info |
| `/STATS` | | Server statistics |
| `/TRACE` | | Trace route |
| `/WHO` | | WHO query |
| `/NAMES` | | List channel names |
| `/USERHOST` | | Userhost lookup |
| `/VERSION` | | Server version |
| `/TIME` | | Server time |
| `/MAP` | | Server map |
| `/LIST` | | Channel list |
| `/IGNORE` | | Add to ignore list |
| `/UNIGNORE` | | Remove from ignore list |

---

## Right-Click Menus

### Nick context menu (nick list and chat area)

Appears when right-clicking a nickname in the nick list or a `<nick>` in the chat area.

- Open Query
- WHOIS
- CTCP Version / Ping / Time / Finger
- Op / DeOp / HalfOp / DeHalfOp / Voice / DeVoice
- Invite
- Slap
- Kick / Ban / Kick + Ban
- Ignore
- **ChanServ submenu**: Op, DeOp, HalfOp, Voice, DeVoice, Kick, Ban, Unban, Kick+Ban, Quiet
- **OperServ submenu**: Kill, AKILL, GLINE, KLINE

### Link context menu (chat area)

Appears when right-clicking a URL in the chat area.

- Open URL -- Opens in system browser
- Copy Link -- Copies URL to clipboard
- Download Image -- Saves image to ~/Downloads (for image URLs)

---

## Dialogs

| Dialog | Description |
|--------|-------------|
| Network List | Manage servers with 10 pre-configured networks, editable address/port/SSL/SASL/channels, per-network identity |
| Quick Connect | Fast connect with server, port, nick, password, channel, SSL toggle |
| Join Channel | Join a channel by name with optional key |
| Nick Change | Change current nickname |
| Channel List | Browse and search server channel list |
| DCC Transfers | View DCC file transfer status |
| Raw Log | View raw IRC protocol messages |
| URL Grabber | Collected URLs from chat |
| Search | Search text within the current buffer |
| Ban List | View and manage channel bans |
| Channel Modes | View and set channel modes |
| Topic | Edit channel topic |
| About | Application info and version |
| Preferences | Full settings editor (see below) |
| Emoji Picker | Browse and insert emoji |
| Invite User | Invite a user to the current channel |
| Server Password | Set server password for connection |

---

## Preferences

The Preferences dialog has 17 tabs covering all configurable options:

**Interface** -- Show/hide server tree, user list, topic bar, status bar, mode buttons.
Open tabs in background. Confirm on close. Minimize to tray.

**Colors** -- Theme selector. mIRC 16-color palette preview. Toggle strip mIRC colors
and nick coloring.

**Text and Fonts** -- Font family and size. Timestamps and format. Max scrollback lines.
Indent wrapped text. Text highlighting with custom extra words.

**Input** -- Spell check. Tab nick-completion settings and suffix. Per-channel input
history with configurable max size.

**User Info** -- Primary nickname plus 3 alternates. Username, real name. Quit message,
part message, away message. These are used as defaults for all connections.

**Connection** -- Auto-reconnect with configurable delay and max attempts. Auto-join on
connect. Global user info toggle. Proxy support (None, SOCKS4, SOCKS5, HTTP CONNECT)
with host, port, and authentication.

**SASL / Auth** -- Default authentication method (SASL PLAIN, EXTERNAL, SCRAM-SHA-256,
ECDSA-NIST256P-CHALLENGE, NickServ, Server Password, CERTFP). NickServ command template.
Client certificate and key PEM paths. Accept invalid SSL certificates toggle.

**DCC** -- Download folder. Auto-accept transfers. Max file size. IP detection method
(automatic, server reply, manual). Port range for DCC.

**Logging** -- Enable/disable logging. Log directory. Format (Plain Text, HTML, JSON).
Timestamps in logs. Log private messages. Separate log file per channel.

**Notifications** -- Desktop notifications on highlight and private message. Flash taskbar.
Unread count in tray. Highlight on nick mention. Notification timeout.

**Sounds** -- Enable/disable sounds. Separate toggles for highlight, PM, and connection
events. Custom beep command.

**Auto-Replace** -- Pattern/replacement rules applied to outgoing text. Built-in examples
(smiley to emoji, common typos). Add custom rules.

**URL Handlers** -- Make URLs clickable. Auto-grab URLs to URL Grabber. Custom browser
command.

**Text Events** -- Customizable format strings for: channel message, join, part, quit,
kick, nick change, topic change, action, notice, CTCP.

**Plugins** -- View loaded C++ plugins. Load, unload, and reload at runtime. Configure
plugin directory path.

**Scripts** -- JavaScript scripting engine. Load, unload, and reload scripts. Open script
editor. Configure script directory.

**Advanced** -- Text encoding (UTF-8 plus 8 alternatives). Identify before join. IPv6
toggle. CAP negotiation. Perform-on-connect command list. Show raw IRC in server tab.
Command character.

---

## Plugins and Scripts

### C++ Plugins

NUchat supports runtime-loaded C++ plugins. Plugins implement the `PluginInterface`
(defined in `src/PluginInterface.h`) and are compiled as shared libraries.

An example plugin is included in `plugins/exampleplugin/`.

To create a plugin:

1. Create a class inheriting from `PluginInterface`
2. Implement `name()`, `version()`, `init()`, `shutdown()`, and `onMessage()`
3. Build as a shared library and place in the `plugins/` directory
4. Load via Settings > Preferences > Plugins, or automatically on startup

### JavaScript Scripts

Place `.js` files in the `scripts/` directory. They are loaded automatically on startup
and can hook into IRC events.

An example script is at `scripts/hello.js`.

---

## Project Structure

```
NUchat/
  CMakeLists.txt              Top-level build file
  README.md                   This file
  src/
    CMakeLists.txt            Source build file
    main.cpp                  Application entry point
    IrcConnection.h/cpp       IRC protocol: QSslSocket, CAP LS 302
    IRCConnectionManager.h/cpp  Multi-server management, command dispatch, WHOIS
    MessageModel.h/cpp        Chat message storage, IRC-to-HTML conversion, image embeds
    MessageParser.h/cpp       Raw IRC message parsing
    ServerChannelModel.h/cpp  Server/channel tree model
    ImageDownloader.h/cpp     Async image download, caching, clipboard
    ThemeManager.h/cpp        Theme property management (C++ side)
    Settings.h/cpp            QSettings wrapper exposed to QML
    Logger.h/cpp              Chat logging system
    NotificationManager.h/cpp Desktop notification support
    ScriptManager.h/cpp       JavaScript scripting engine
    PluginManager.h/cpp       C++ plugin loader
    PluginInterface.h         Plugin API interface
    resources.qrc             Qt resource file
  qml/
    main.qml                  Main application window and UI
    ThemeManager.qml          25 built-in themes with 50+ color properties each
    NetworkListDialog.qml     Network/server manager
    QuickConnectDialog.qml    Quick connect dialog
    PreferencesDialog.qml     Full preferences editor
    JoinChannelDialog.qml     Join channel dialog
    NickChangeDialog.qml      Nickname change dialog
    ChannelListDialog.qml     Server channel list browser
    BanListDialog.qml         Channel ban list manager
    ChannelModeDialog.qml     Channel mode editor
    TopicDialog.qml           Topic editor
    DccDialog.qml             DCC transfers
    RawLogDialog.qml          Raw IRC log viewer
    UrlGrabberDialog.qml      URL collector
    SearchDialog.qml          Buffer search
    AboutDialog.qml           About dialog
    SettingsDialog.qml        Settings sub-dialog
    EmojiPicker.qml           Emoji picker component
    ChannelView.qml           Channel view component
    NickList.qml              User list component
    ServerTree.qml            Server tree component
  plugins/
    exampleplugin/            Example C++ plugin
  scripts/
    hello.js                  Example JavaScript script
  tests/
    test_messageparser.cpp    Unit tests for message parser
  resources/
    icons/
      nuchat.svg              Application icon (SVG)
      nuchat-{16..512}.png    Application icons (PNG, multiple sizes)
    nuchat.desktop            Linux desktop entry file
    themes/
      default.json            Default theme data
```

---

## TODO

### High Priority

- [ ] DCC file transfer implementation (send and receive)
- [ ] DCC direct chat implementation
- [ ] Ignore list (currently a stub -- needs actual message filtering)
- [ ] Notify list (online/offline tracking for specified nicks)
- [ ] Auto-reconnect on disconnect (backend logic)
- [ ] SASL authentication handshake in IrcConnection
- [ ] Perform-on-connect command execution after joining a server

### Features

- [ ] Spell checking in the input field
- [ ] Away log (collect messages received while away)
- [ ] Auto-replace pattern processing on outgoing messages
- [ ] URL Grabber -- collect URLs from chat in UrlGrabberDialog
- [ ] Keyboard shortcuts (configurable)
- [ ] Detach/attach tabs (tear-off tab support)
- [ ] Channel list search and filtering in ChannelListDialog
- [ ] Ban list management in BanListDialog (add/remove bans from dialog)
- [ ] Identd server
- [ ] IPv6 connection support
- [ ] Proxy support (SOCKS4, SOCKS5, HTTP CONNECT)
- [ ] Notification timeout and sound event configuration
- [ ] Logging to file (plain text, HTML, JSON)
- [ ] Text event customization (format strings for join/part/quit/etc.)

### UI / Polish

- [ ] Video embed previews (currently shows a label, no player)
- [ ] Animated GIF playback in inline previews
- [ ] Image preview size configuration
- [ ] Drag-and-drop file sending via DCC
- [ ] Status bar with connection info
- [ ] Mode buttons row in channel view
- [ ] Minimize to system tray
- [ ] Multi-line input support
- [ ] Paste image from clipboard
- [ ] Tab reordering via drag-and-drop

### Code Quality

- [ ] Expand unit test coverage beyond MessageParser
- [ ] Add integration tests for IrcConnection
- [ ] CI/CD pipeline (GitHub Actions)
- [ ] Packaging (AppImage, Flatpak, .deb)
- [ ] Documentation for plugin and scripting APIs
- [ ] Code documentation and comments cleanup

---

## License

This project is currently unlicensed. A license will be added in a future release.
