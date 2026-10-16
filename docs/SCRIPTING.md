# NUchat Scripting API

NUchat provides a **HexChat-style scripting API** for both Python and Lua.
It covers the calls most scripts use. It is not a full HexChat runtime; see [Current limitations](#current-limitations).
Scripts are loaded automatically from `~/.config/NUchat/scripts/` on startup.
A change on disk reloads the file. Lua drops that script's hooks first; Python hot reload does not. See [Hot Reloading](#hot-reloading).

---

> **Security Warning:** Scripts execute with the same privileges as NUchat and
> have **full access** to the Python/Lua standard libraries, including file I/O,
> network sockets, and process execution (`os.system`, `subprocess`, etc.).
> **Only load scripts from sources you fully trust.** NUchat does not sandbox
> or restrict script capabilities.

> **Lua memory limit:** The Lua VM is capped at **32 MB** of heap memory per
> script engine instance. Scripts that attempt to allocate beyond this limit will
> receive a Lua out-of-memory error. Python scripts have no equivalent hard cap
> (they use the system allocator).

---

## Table of Contents

- [Quick Start](#quick-start)
- [Python API](#python-api)
- [Lua API](#lua-api)
- [API Reference](#api-reference)
  - [Functions](#functions)
  - [Hook Types](#hook-types)
  - [Constants](#constants)
  - [get_info Keys](#get_info-keys)
- [Hook Callbacks](#hook-callbacks)
- [Examples](#examples)
- [JavaScript](#javascript)

---

## Quick Start

### Python

1. Create a `.py` file in `~/.config/NUchat/scripts/`
2. `import hexchat` at the top
3. Use the API to register hooks and interact with IRC

```python
import hexchat

__module_name__ = "myscript"
__module_version__ = "1.0"
__module_description__ = "My first NUchat script"

def greet_cb(word, word_eol, userdata=None):
    hexchat.prnt(f"Hello, {hexchat.get_info('nick')}!")
    return hexchat.EAT_ALL

hexchat.hook_command("GREET", greet_cb, help="Usage: /GREET — say hello")
hexchat.prnt("myscript.py loaded!")
```

### Lua

1. Create a `.lua` file in `~/.config/NUchat/scripts/`
2. Use the `hexchat` global (or `nuchat` alias)
3. Use the API to register hooks and interact with IRC

```lua
hexchat.hook_command("GREET", function(word, word_eol, userdata)
    hexchat.prnt("Hello, " .. hexchat.get_info("nick") .. "!")
    return hexchat.EAT_ALL
end)

hexchat.prnt("myscript.lua loaded!")  -- does not display; see below
```

Lua `prnt` looks up a `MessageModel` child of the connection manager. The model is not parented there, so that line does not appear. Use a Python script, or `hexchat.command("ECHO ...")`, to print locally.

---

## Python API

### Module: `hexchat`

Import with `import hexchat`. All functions and constants are accessed through this module.

#### Script Metadata

Define these module-level variables for identification in the Scripts dialog:

| Variable | Type | Description |
|----------|------|-------------|
| `__module_name__` | `str` | Display name of the script |
| `__module_version__` | `str` | Version string |
| `__module_description__` | `str` | Short description |

---

## Lua API

### Global: `hexchat` / `nuchat`

Both `hexchat` and `nuchat` globals are available. They are identical and provided for
compatibility with both HexChat and NUchat naming conventions.

---

## API Reference

### Functions

| Function | Description |
|----------|-------------|
| `hexchat.command(cmd)` | Execute an IRC command (without leading `/`). Example: `hexchat.command("JOIN #channel")` |
| `hexchat.prnt(text)` | Python sends `/ECHO`, which prints in the active buffer and supports mIRC color codes (e.g. `\00303green\003`). Lua looks up a `MessageModel` child of the connection manager. The model is not parented there, so Lua `prnt` does not display. |
| `hexchat.emit_print(event, ...)` | Print the arguments, joined by spaces, in the active tab. The event name is ignored. This does not dispatch HexChat text events. |
| `hexchat.get_info(id)` | Retrieve information about the current context. See [get_info Keys](#get_info-keys). |
| `hexchat.hook_command(name, cb, ...)` | Register a hook for a `/COMMAND`. See [hook_command](#hook_command). |
| `hexchat.hook_server(name, cb, ...)` | Register a hook for raw server numerics/commands. See [hook_server](#hook_server). |
| `hexchat.hook_print(name, cb, ...)` | Store a print-event hook. The callback is not called. See [hook_print](#hook_print). |
| `hexchat.hook_timer(timeout, cb, ...)` | Register a timer hook. See [hook_timer](#hook_timer). |
| `hexchat.unhook(hook_id)` | Remove a previously registered hook by its ID. |
| `hexchat.nickcmp(a, b)` | Case-insensitive IRC nick comparison. Returns 0 if equal. |

### Hook Types

#### hook_command

Register a handler for a user-typed `/COMMAND`.

**Python:**
```python
hook_id = hexchat.hook_command(name, callback, userdata=None, priority=hexchat.PRI_NORM, help=None)
```

**Lua:**
```lua
hook_id = hexchat.hook_command(name, callback [, userdata [, priority]])
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Command name (without `/`), e.g. `"HELLO"` |
| `callback` | function | Handler function — see [Hook Callbacks](#hook-callbacks) |
| `userdata` | any | Optional data passed to the callback |
| `priority` | int | Hook priority (default: `PRI_NORM`) |
| `help` | string | Accepted by the Python binding and then discarded. There is no `/HELP` command, and the string is not stored |

**Returns:** An integer hook ID that can be passed to `unhook()`.

#### hook_server

Register a handler for raw IRC server messages (numerics or named commands).

**Python:**
```python
hook_id = hexchat.hook_server(name, callback, userdata=None, priority=hexchat.PRI_NORM)
```

**Lua:**
```lua
hook_id = hexchat.hook_server(name, callback [, userdata [, priority]])
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Server numeric (e.g. `"376"` for end of MOTD) or command (e.g. `"PRIVMSG"`) |
| `callback` | function | Handler receiving the raw line |
| `userdata` | any | Optional data passed to the callback |
| `priority` | int | Hook priority. Higher values run first (`PRI_HIGHEST` … `PRI_LOWEST`). Equal priorities keep registration order. |

The command name is taken from the IRC line after IRCv3 message-tags and the
optional `:prefix` are skipped:

- `:nick!user@host PRIVMSG #chan :hello` matches `PRIVMSG`
- `@time=2026-10-16T00:00:00.000Z :nick!user@host PRIVMSG #chan :hello` matches `PRIVMSG`
- `PING :irc.example.net` matches `PING` (there is no prefix)

`word` / `word_eol` are the space-split line **with tags removed**. A hook
named `RAW LINE` receives every server line. Names are matched
case-insensitively (`"privmsg"` and `"PRIVMSG"` are the same hook).

#### hook_print

Store a handler for a print-event name. Neither engine calls these callbacks. Python's `handlePrintEvent` exists and has no caller. Use `hook_server` for IRC lines such as `JOIN`.

**Python:**
```python
hook_id = hexchat.hook_print(name, callback, userdata=None, priority=hexchat.PRI_NORM)
```

**Lua:**
```lua
hook_id = hexchat.hook_print(name, callback [, userdata [, priority]])
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Event name (e.g. `"Channel Message"`, `"Join"`) |
| `callback` | function | Handler for the event |
| `userdata` | any | Optional data passed to the callback |
| `priority` | int | Hook priority |

#### hook_timer

Register a timer that fires after a specified delay.

**Python:**
```python
hook_id = hexchat.hook_timer(timeout_ms, callback, userdata=None)
```

**Lua:**
```lua
hook_id = hexchat.hook_timer(timeout_ms, callback [, userdata])
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `timeout_ms` | int | Delay in milliseconds |
| `callback` | function | Called when the timer fires. Return `1` to repeat, `0` (or `false`) to cancel. Cancelling from inside the callback is safe. |
| `userdata` | any | Optional data passed to the callback |

### Constants

#### Return Values (EAT)

| Constant | Value | Meaning |
|----------|-------|---------|
| `EAT_NONE` | 0 | Pass the event to other hooks and NUchat |
| `EAT_HEXCHAT` | 1 | Don't let NUchat process this event, but pass to other hooks |
| `EAT_PLUGIN` | 2 | Don't pass to other hooks, but let NUchat process it |
| `EAT_ALL` | 3 | Consume completely — no other hooks or NUchat processing |

Python and Lua both define `EAT_XCHAT` as an alias for `EAT_HEXCHAT`.

These values are honored:

- Hooks run highest priority first. `EAT_PLUGIN` and `EAT_ALL` stop later hooks in that engine and skip the other language engine (Python runs before Lua).
- For a **server line**, `EAT_HEXCHAT` and `EAT_ALL` skip NUchat's own handling of that line, including the automatic `PONG` for `PING`. A script that eats `PING` has to answer it itself.
- For a **user command**, `EAT_HEXCHAT` and `EAT_ALL` skip the built-in command table. `EAT_PLUGIN` stops later script hooks but still lets NUchat run the command.
- C++ plugins run before either script engine on `/command`. A plugin that returns true consumes the command.

#### Hook Priorities

| Constant | Value | Description |
|----------|-------|-------------|
| `PRI_HIGHEST` | 127 | Highest priority — runs first |
| `PRI_HIGH` | 64 | High priority |
| `PRI_NORM` | 0 | Normal (default) |
| `PRI_LOW` | -64 | Low priority |
| `PRI_LOWEST` | -128 | Lowest priority — runs last |

### get_info Keys

| Key | Returns |
|-----|---------|
| `"nick"`, `"nickname"` | Current nickname on the active server |
| `"server"`, `"network"`, `"host"` | Hostname of the active server |
| `"topic"` | Topic of the active channel |
| `"version"` | NUchat version string |
| `"channel"` | Empty string (see below) |
| `"away"` | Empty string. `IRCConnectionManager` has an `isAway` property, and `get_info` does not read it |
| `"event_text"` | Empty string |

### Current limitations

- `get_info("channel")` returns an empty string. The Python engine returns that directly. The Lua engine reads a Qt property named `activeChannel`, and `IRCConnectionManager` does not publish that property.
- `get_info("away")` returns an empty string in both engines.
- `nickcmp` always compares with rfc1459 rules. It does not use the server's ISUPPORT `CASEMAPPING`.
- `emit_print` prints its arguments locally. It does not run print hooks or format a HexChat text event.
- Python `get_list` returns an empty list. Python `get_prefs` returns `None`. `get_context`, `find_context`, and `set_context` are stubs (a constant context id, and `set_context` always succeeds). Lua does not define those functions.
- Python `hook_command`'s `help` keyword is parsed and ignored.
- Python command, server, and print callbacks are always called with three arguments: `word`, `word_eol`, and `userdata`. A function that only accepts the first two raises `TypeError`. Lua command and server callbacks are also called with those three values; Lua ignores extra arguments, so a shorter function still runs. Lua timer callbacks receive only userdata.
- `hook_print` stores the callback in both engines and never calls it. Python has `handlePrintEvent`, and nothing in the client calls it. A `hook_print("Join", ...)` callback does not run when someone joins. Use `hook_server("JOIN", ...)` for server lines.
- Lua `prnt` does not show text. It searches for a `MessageModel` that is a child of `IRCConnectionManager`, and `setMessageModel` does not reparent the model. Python `prnt` goes through `/ECHO` and does show.
- Python `hook_unload(callback, userdata=None)` is implemented. The callback is called with userdata when that script is unloaded. Lua has no `hook_unload`.

---

## Hook Callbacks

### Command / Server / Print Callbacks

**Python:**
```python
def my_callback(word, word_eol, userdata=None):
    # word     — list of space-separated tokens
    # word_eol — list where word_eol[i] = everything from word[i] onward
    return hexchat.EAT_NONE
```

**Lua:**
```lua
function my_callback(word, word_eol, userdata)
    -- word     — table of space-separated tokens
    -- word_eol — table where word_eol[i] = everything from word[i] onward
    return hexchat.EAT_NONE
end
```

### Timer Callbacks

**Python:**
```python
def my_timer(userdata=None):
    # Return 1 to keep the timer repeating, 0 to cancel
    return 0
```

**Lua:**
```lua
function my_timer(userdata)
    -- Return 1 to keep repeating, 0 to cancel (one-shot)
    return 0
end
```

---

## Examples

### Auto-greet on join (Python)

`hook_print` is not dispatched. Hook the server `JOIN` line instead. `word` is the tag-stripped line, so the prefix is `word[0]` and the channel is `word[2]`.

```python
import hexchat

__module_name__ = "autogreet"
__module_version__ = "1.0"
__module_description__ = "Greets users when they join"

def on_join(word, word_eol, userdata):
    prefix = word[0][1:] if word and word[0].startswith(":") else ""
    nick = prefix.split("!")[0]
    channel = word[2].lstrip(":") if len(word) > 2 else ""
    my_nick = hexchat.get_info("nick")
    if nick and nick != my_nick and channel:
        hexchat.command(f"PRIVMSG {channel} :Welcome, {nick}!")
    return hexchat.EAT_NONE

hexchat.hook_server("JOIN", on_join)
hexchat.prnt("autogreet.py loaded!")
```

### Countdown timer (Lua)

```lua
hexchat.hook_command("COUNTDOWN", function(word, word_eol, userdata)
    local count = tonumber(word[2]) or 5
    local remaining = count

    local function tick(ud)
        if remaining > 0 then
            hexchat.prnt(tostring(remaining) .. "...")
            remaining = remaining - 1
            return 1  -- repeat
        else
            hexchat.prnt("Go!")
            return 0  -- stop
        end
    end

    hexchat.hook_timer(1000, tick)
    return hexchat.EAT_ALL
end)
```

### Server hook — track MOTD (Python)

```python
import hexchat

__module_name__ = "motdtracker"
__module_version__ = "1.0"
__module_description__ = "Tracks MOTD lines"

motd_lines = []

def on_motd(word, word_eol, userdata):
    motd_lines.append(word_eol[0])
    return hexchat.EAT_NONE

def on_endmotd(word, word_eol, userdata):
    hexchat.prnt(f"MOTD was {len(motd_lines)} lines long")
    motd_lines.clear()
    return hexchat.EAT_NONE

hexchat.hook_server("372", on_motd)      # RPL_MOTD
hexchat.hook_server("376", on_endmotd)   # RPL_ENDOFMOTD
```

---

## JavaScript

NUchat also loads `*.js` files with Qt's JavaScript engine. This is separate
from the HexChat Python/Lua API: there is no `hexchat` module.

Scripts are loaded from the same places as the examples shipped with NUchat:
the in-tree `scripts/` directory when running a dev build,
`<prefix>/share/nuchat/scripts` after install, `scripts/` next to the
executable, and `~/.config/NUchat/scripts/`.

Define a global `onMessage(conn, sender, message)`. It is called for each
`PRIVMSG` and `NOTICE` after IRCv3 tags are stripped. `sender` is the nick,
`message` is the message text (so a body of `!ping` is exactly `"!ping"`, not
the raw IRC line). `conn` is the live connection; `conn.sendRaw(line)` sends
a protocol line. Other server lines (joins, numerics, `PING`) are not passed
to `onMessage`. If several scripts each define `onMessage`, the last one
loaded wins.

```javascript
function onMessage(conn, sender, message) {
    if (message.indexOf("!ping") === 0) {
        conn.sendRaw("PRIVMSG #general :PONG from script");
    }
}
```

See `scripts/hello.js`.

---

## Script Management

There are no `/LOAD`, `/UNLOAD`, or `/RELOAD` commands. Those names are forwarded to the server as raw IRC.

| Surface | What it does |
|---------|----------------|
| **Scripts → Loaded Scripts...** and **View → Plugins and Scripts...** | Open the Scripts dialog |
| **Scripts → Load Script...** and the dialog's **Load...** | Python only. Title "Load Python Script", filter `*.py`, `pyEngine.loadScript`. There is no Lua file picker |
| **Scripts → Reload All Scripts** | `pyEngine.reloadAll()` only |
| Dialog **Reload** | Python: `reloadScript`. Lua: `unloadScript` then `loadScript` of `scriptsDirectory/filename` |
| Dialog **Reload All** | `pyEngine.reloadAll()` and `luaEngine.reloadAll()` |
| Dialog **Unload** | Unloads the selected Python or Lua script |
| **Scripts → Open Scripts Folder** | Python scripts directory when Python is built in. The dialog's Open Folder falls back to the Lua directory |
| Preferences → Scripts buttons | Load, Unload, and Open Script Editor have no handlers. The list is a static label |

The Scripts dialog lists loaded Python and Lua files. Python rows show `__module_name__`, version, and description. Lua rows show the filename: the Lua engine never fills its info map. The hook total under the list is Python's `hookCount()` only. Lua hooks are not included, and the count is not per script.

---

## Hot Reloading

NUchat watches `~/.config/NUchat/scripts/` for file changes.

Lua reloads a changed `.lua` file by unloading that script first, which drops its hooks, then running it again. A new `.lua` file that appears in the directory is loaded.

Python reloads a changed `.py` file by running it again. It does not remove the previous hooks first, so a hot reload can leave the old hooks in place alongside the new ones. Unload or Reload from the Scripts dialog does remove that script's hooks before loading it again.

---

## Notes

- The `hexchat` module is injected by NUchat at runtime — there is no separate package
  to install.
- Scripts run in the main thread. Long-running operations will block the UI.
- Python `prnt` appears in the currently active channel tab. Lua `prnt` does not (see [Current limitations](#current-limitations)).
- mIRC color codes in Python `prnt()` are rendered (e.g. `\00304red text\003`). Lua `prnt` does not display, so those codes are not shown from Lua.

---

## Migrating from HexChat

If you have existing HexChat scripts, NUchat can import them automatically.

### Automatic (first launch)

On first launch, if NUchat finds no loaded scripts and detects `~/.config/hexchat/`,
it opens the **Migrate from HexChat** wizard. You can choose to import:

- **Scripts** — copies `.py` and `.lua` files from `~/.config/hexchat/addons/` into
  `~/.config/NUchat/scripts/`
- **Networks** — imports your server list from `servlist.conf` into the NUchat network list
- **Identity** — imports your nickname, username, and realname from `hexchat.conf`

### Manual

Open **Settings → Preferences → Scripts → Open Migration Wizard** at any time.

### Compatibility notes

- NUchat uses `import hexchat` for the calls listed above. Scripts that depend on print events, `get_list`, contexts, or `get_info("channel")` need changes. See [Current limitations](#current-limitations).
- **Command hooks** receive arguments with the command name excluded from the word list, and Python callbacks always receive 3 parameters (word, word_eol, userdata).
- **SAY command** is implemented — `hexchat.command("SAY text")` sends the text as a normal message on the active channel.
- `hexchat.find_context()`, `hexchat.get_context()`, and `hexchat.get_list()` are stubs.
- Lua scripts using the `hexchat` global work for the implemented calls. Scripts using `xchat` need the global renamed. Lua has no `hook_unload`.
- Python `hexchat.hook_unload(callback, userdata=None)` runs when that script is unloaded. The callback receives userdata.
