# NUchat Scripting API

NUchat provides a **HexChat-compatible scripting API** for both Python and Lua.
Scripts are loaded automatically from `~/.config/NUchat/scripts/` on startup and
hot-reloaded when modified on disk.

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

def greet_cb(word, word_eol):
    hexchat.prnt("Hello, " + hexchat.get_info("nick") + "!")
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

hexchat.prnt("myscript.lua loaded!")
```

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
| `hexchat.prnt(text)` | Print text to the current channel tab locally. Supports mIRC color codes (e.g. `\00303green\003`). |
| `hexchat.emit_print(event, ...)` | Emit a print event with the given arguments. |
| `hexchat.get_info(id)` | Retrieve information about the current context. See [get_info Keys](#get_info-keys). |
| `hexchat.hook_command(name, cb, ...)` | Register a hook for a `/COMMAND`. See [hook_command](#hook_command). |
| `hexchat.hook_server(name, cb, ...)` | Register a hook for raw server numerics/commands. See [hook_server](#hook_server). |
| `hexchat.hook_print(name, cb, ...)` | Register a hook for print events. See [hook_print](#hook_print). |
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
| `help` | string | Help text shown for `/HELP name` (Python only) |

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
| `priority` | int | Hook priority |

#### hook_print

Register a handler for print events (local display events).

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
| `callback` | function | Called when the timer fires. Return `1` to repeat, `0` to cancel. |
| `userdata` | any | Optional data passed to the callback |

### Constants

#### Return Values (EAT)

| Constant | Value | Meaning |
|----------|-------|---------|
| `EAT_NONE` | 0 | Pass the event to other hooks and NUchat |
| `EAT_HEXCHAT` | 1 | Don't let NUchat process this event, but pass to other hooks |
| `EAT_PLUGIN` | 2 | Don't pass to other hooks, but let NUchat process it |
| `EAT_ALL` | 3 | Consume completely — no other hooks or NUchat processing |

Lua also defines `EAT_XCHAT` as an alias for `EAT_HEXCHAT`.

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
| `"nick"` | Current nickname on the active server |
| `"server"` | Hostname of the active server |
| `"topic"` | Topic of the active channel |
| `"version"` | NUchat version string |
| `"channel"` | Name of the active channel |

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

```python
import hexchat

__module_name__ = "autogreet"
__module_version__ = "1.0"
__module_description__ = "Greets users when they join"

def on_join(word, word_eol, userdata):
    nick = word[0]
    channel = word[1] if len(word) > 1 else ""
    my_nick = hexchat.get_info("nick")
    if nick != my_nick:
        hexchat.command("PRIVMSG " + channel + " :Welcome, " + nick + "!")
    return hexchat.EAT_NONE

hexchat.hook_print("Join", on_join)
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
    hexchat.prnt("MOTD was " + str(len(motd_lines)) + " lines long")
    motd_lines.clear()
    return hexchat.EAT_NONE

hexchat.hook_server("372", on_motd)      # RPL_MOTD
hexchat.hook_server("376", on_endmotd)   # RPL_ENDOFMOTD
```

---

## Script Management

Scripts can be managed via the **Scripts** menu or through commands:

| Command | Description |
|---------|-------------|
| `/LOAD <path>` | Load a script file |
| `/UNLOAD <filename>` | Unload a loaded script |
| `/RELOAD` | Reload all scripts |

The Scripts dialog (**Scripts > Plugins and Scripts...**) shows all loaded Python and Lua
scripts with their metadata and hook counts.

---

## Hot Reloading

NUchat watches the scripts directory for file changes. When a `.py` or `.lua` file is
modified, it is automatically reloaded. All existing hooks from that script are removed
before the script is re-executed.

---

## Notes

- The `hexchat` module is injected by NUchat at runtime — there is no separate package
  to install.
- Scripts run in the main thread. Long-running operations will block the UI.
- All script output (`prnt`) appears in the currently active channel tab.
- mIRC color codes are supported in `prnt()` output (e.g. `\00304red text\003`).
