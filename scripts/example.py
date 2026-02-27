"""
NUchat Example Script — HexChat-compatible Python API

Drop .py files into ~/.config/NUchat/scripts/ and they load automatically.
Scripts are hot-reloaded when modified.

Available API (import hexchat):
  hexchat.command(cmd)          — Execute a /command
  hexchat.prnt(text)            — Print text locally
  hexchat.get_info(id)          — Get info: "nick", "server", "topic", "version"
  hexchat.hook_command(name, cb, userdata=None, priority=PRI_NORM, help=None)
  hexchat.hook_server(name, cb, userdata=None, priority=PRI_NORM)
  hexchat.hook_print(name, cb, userdata=None, priority=PRI_NORM)
  hexchat.hook_timer(timeout_ms, cb, userdata=None)
  hexchat.unhook(hook_id)
  hexchat.nickcmp(a, b)         — IRC nick comparison
  hexchat.emit_print(event, *args)

Constants: EAT_NONE, EAT_HEXCHAT, EAT_PLUGIN, EAT_ALL
           PRI_HIGHEST, PRI_HIGH, PRI_NORM, PRI_LOW, PRI_LOWEST
"""

import hexchat

__module_name__ = "example"
__module_version__ = "1.0"
__module_description__ = "Example NUchat script"


# ── Example: /HELLO command ──
def hello_cb(word, word_eol):
    hexchat.prnt("Hello from Python script!")
    hexchat.prnt("You are: " + str(hexchat.get_info("nick")))
    hexchat.prnt("Server:  " + str(hexchat.get_info("server")))
    return hexchat.EAT_ALL

hexchat.hook_command("HELLO", hello_cb, help="Usage: /HELLO — test command from Python")


# ── Example: /PYVERSION command ──
import sys
def pyver_cb(word, word_eol):
    hexchat.prnt("Python " + sys.version)
    return hexchat.EAT_ALL

hexchat.hook_command("PYVERSION", pyver_cb, help="Usage: /PYVERSION — show Python version")


hexchat.prnt("\\00303example.py loaded!\\003")
