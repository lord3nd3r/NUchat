--[[
NUchat Example Script — HexChat-compatible Lua API

Drop .lua files into ~/.config/NUchat/scripts/ and they load automatically.
Scripts are hot-reloaded when modified.

Available API (both "hexchat" and "nuchat" globals work):
  hexchat.command(cmd)          — Execute a /command
  hexchat.prnt(text)            — Print text locally
  hexchat.print(text)           — Alias for prnt (HexChat compat)
  hexchat.emit_print(event, …)  — Emit a print event
  hexchat.get_info(id)          — Get info: "nick", "server", "topic", "version"
  hexchat.hook_command(name, cb [, userdata [, priority]])
  hexchat.hook_server(name, cb [, userdata [, priority]])
  hexchat.hook_print(name, cb [, userdata [, priority]])
  hexchat.hook_timer(timeout_ms, cb [, userdata])
  hexchat.unhook(hook_id)
  hexchat.nickcmp(a, b)         — IRC nick comparison

Constants: EAT_NONE, EAT_HEXCHAT, EAT_XCHAT, EAT_PLUGIN, EAT_ALL
           PRI_HIGHEST, PRI_HIGH, PRI_NORM, PRI_LOW, PRI_LOWEST
--]]


-- ── Example: /LUAHELLO command ──
hexchat.hook_command("LUAHELLO", function(word, word_eol, userdata)
    hexchat.prnt("Hello from Lua script!")
    hexchat.prnt("You are: " .. tostring(hexchat.get_info("nick")))
    hexchat.prnt("Server:  " .. tostring(hexchat.get_info("server")))
    return hexchat.EAT_ALL
end)


-- ── Example: /LUAVERSION command ──
hexchat.hook_command("LUAVERSION", function(word, word_eol, userdata)
    hexchat.prnt("Lua " .. _VERSION)
    return hexchat.EAT_ALL
end)


-- ── Example: /LUATIME command (demonstrates hook_timer) ──
hexchat.hook_command("LUATIME", function(word, word_eol, userdata)
    hexchat.prnt("Starting a 5-second one-shot timer...")
    hexchat.hook_timer(5000, function(userdata)
        hexchat.prnt("Timer fired! 5 seconds have passed.")
        return 0  -- return 0 to remove the timer (one-shot)
    end)
    return hexchat.EAT_ALL
end)


hexchat.prnt("\00303example.lua loaded!\003")
