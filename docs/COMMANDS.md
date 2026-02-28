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

## Information

| Command | Aliases | Description |
|---------|---------|-------------|
| `/WHOIS` | `/W`, `/WI` | WHOIS lookup |
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
| `/IGNORE` | | Add to ignore list |
| `/UNIGNORE` | | Remove from ignore list |
