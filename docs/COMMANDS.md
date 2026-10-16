# NUchat IRC Commands

All commands are case-insensitive and prefixed with `/`. Unknown commands are forwarded to the server as raw IRC.

---

## General

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
| `/CLEAR` | | Clear message buffer |
| `/CLOSE` | | Part channel and close tab |
| `/CYCLE` | `/REJOIN` | Part and rejoin channel |
| `/RAW` | `/QUOTE` | Send raw IRC command |
| `/SAY` | | Send the rest of the line as a normal message, including text that starts with `/` |
| `/ECHO` | | Print text in the current buffer. Nothing is sent to the server |

## Information

| Command | Aliases | Description |
|---------|---------|-------------|
| `/WHOIS` | `/W`, `/WI` | WHOIS lookup |
| `/WHOWAS` | `/WW` | WHOWAS lookup |
| `/WHO` | | WHO query |
| `/NAMES` | | List channel names |
| `/USERHOST` | | Userhost lookup |
| `/LIST` | | Channel list |
| `/SYSINFO` | | Display system info (OS, CPU, RAM, GPU, uptime) |

## Channel Management

| Command | Aliases | Description |
|---------|---------|-------------|
| `/TOPIC` | `/T` | Get or set channel topic |
| `/MODE` | | Set or query channel/user modes |
| `/KICK` | | Kick user from channel |
| `/BAN` | `/B` | Ban user (auto-builds hostmask) |
| `/UNBAN` | | Remove ban |
| `/KB` | `/KICKBAN` | Kick and ban user |
| `/INVITE` | `/INV` | Invite user to channel |
| `/NOTICE` | | Send a notice |

## User Modes

| Command | Aliases | Description |
|---------|---------|-------------|
| `/OP` | | Give operator status (+o) |
| `/DEOP` | | Remove operator status (-o) |
| `/HALFOP` | `/HOP` | Give half-operator (+h) |
| `/DEHALFOP` | `/DEHOP` | Remove half-operator (-h) |
| `/VOICE` | `/V` | Give voice (+v) |
| `/DEVOICE` | | Remove voice (-v) |

## Services

| Command | Aliases | Description |
|---------|---------|-------------|
| `/NS` | `/NICKSERV` | Message NickServ |
| `/CS` | `/CHANSERV` | Message ChanServ |
| `/OS` | `/OPERSERV` | Message OperServ |
| `/HS` | `/HOSTSERV` | Message HostServ |
| `/MS` | `/MEMOSERV` | Message MemoServ |
| `/BS` | `/BOTSERV` | Message BotServ |
| `/HELPSERV` | | Message HelpServ |

## CTCP

| Command | Aliases | Description |
|---------|---------|-------------|
| `/CTCP` | | Send arbitrary CTCP message |
| `/SLAP` | | Slap someone with a large trout |

## Server Information

| Command | Aliases | Description |
|---------|---------|-------------|
| `/LINKS` | | List server links |
| `/LUSERS` | | Network statistics |
| `/MOTD` | | Display Message of the Day |
| `/ADMIN` | | Show admin info |
| `/STATS` | | Server statistics |
| `/TRACE` | | Trace route |
| `/VERSION` | | Server version |
| `/TIME` | | Server time |
| `/MAP` | | Server map |

## Operator Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `/OPER` | | Authenticate as IRC operator |
| `/KILL` | | Kill user (oper only) |
| `/GLINE` | | Global ban (oper only) |
| `/KLINE` | | Local ban (oper only) |
| `/ZLINE` | | IP ban (oper only) |
| `/DLINE` | | IP range ban (oper only) |
| `/WALLOPS` | | Send wallops message (oper only) |
| `/SQUIT` | | Split server link (oper only) |
| `/REHASH` | | Rehash server config (oper only) |

## Other

| Command | Aliases | Description |
|---------|---------|-------------|
| `/IGNORE` | | Add to ignore list. With no arguments, print the current list |
| `/UNIGNORE` | | Remove from ignore list |

## Client commands

These are handled inside NUchat. They are not IRC protocol commands.

| Command | Aliases | Description |
|---------|---------|-------------|
| `/SERVER` | `/NEWSERVER` | Connect to `<host> [port] [password]`. A port that starts with `+` turns TLS on (default port 6697). Otherwise the default port is 6667 and TLS is off. Nick, username, and real name come from saved settings. |
| `/TIMER` | | `/TIMER <seconds> <command or text>` runs once after the delay, on the server and channel that were active when the timer was set. A leading `/` runs a command; other text is sent as a message. |
| `/LASTLOG` | | `/LASTLOG <text>` prints lines from the current buffer whose text contains that string, case-insensitive. At most the latest 100 matches are shown. |
| `/EXEC` | | `/EXEC [-o] <command>` runs the command in a shell (`/bin/sh -c` on Unix, `cmd /c` on Windows). Output is shown locally. `-o` sends up to 20 lines to the current channel instead. The process is killed after 15 seconds. |
| `/ALIAS` | | `/ALIAS` lists aliases. `/ALIAS <name> <expansion>` saves one. `$*` in the expansion is replaced with the arguments; if `$*` is absent, the arguments are appended. Names are stored in uppercase. Alias expansion stops after 5 nested aliases. |
| `/UNALIAS` | | `/UNALIAS <name>` removes a saved alias. |
| `/NOTIFY` | | `/NOTIFY` prints the notify list. `/NOTIFY <nick>` adds a nick and sends `MONITOR +`. `/NOTIFY -<nick>` removes it and sends `MONITOR -`. |
| `/DCC` | | `/DCC SEND <nick> <filepath>` offers a file to that nick. Other `/DCC` subcommands are not implemented. |

## Formatting Shortcuts (mIRC)

These keyboard shortcuts insert mIRC control characters into the message input:

| Shortcut | Code | Effect |
|----------|------|--------|
| `Ctrl+K` | `\x03` | Color — follow with number (e.g. `Ctrl+K 4` for red) |
| `Ctrl+B` | `\x02` | Bold |
| `Ctrl+U` | `\x1F` | Underline |
| `Ctrl+I` | `\x1D` | Italic |
| `Ctrl+O` | `\x0F` | Reset all formatting |
