#pragma once

// HexChat-compatible hook return bits. Shared by the Python and Lua engines
// and by the slash-command / server-line dispatchers.
//
//   EAT_NONE    (0) — later hooks run, NUchat still handles the event
//   EAT_HEXCHAT (1) — later hooks run, NUchat skips its own handling
//   EAT_PLUGIN  (2) — later hooks do not run, NUchat still handles the event
//   EAT_ALL     (3) — later hooks do not run, NUchat skips its own handling

inline constexpr int NUCHAT_EAT_NONE = 0;
inline constexpr int NUCHAT_EAT_HEXCHAT = 1;
inline constexpr int NUCHAT_EAT_PLUGIN = 2;
inline constexpr int NUCHAT_EAT_ALL = NUCHAT_EAT_HEXCHAT | NUCHAT_EAT_PLUGIN;

inline bool eatSuppressesCore(int eat) { return (eat & NUCHAT_EAT_HEXCHAT) != 0; }
inline bool eatStopsHooks(int eat) { return (eat & NUCHAT_EAT_PLUGIN) != 0; }

// Combine a hook's return value into the mask accumulated so far.
// Once EAT_PLUGIN is set the caller must not invoke later hooks; this still
// ORs the bits for a hook that already ran.
inline int foldEat(int accumulated, int hookReturn) {
    const int bits = hookReturn & (NUCHAT_EAT_HEXCHAT | NUCHAT_EAT_PLUGIN);
    return accumulated | bits;
}
