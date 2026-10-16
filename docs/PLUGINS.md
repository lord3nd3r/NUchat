# NUchat C++ Plugin API

NUchat supports C++ plugins loaded as shared libraries at runtime. Plugins are
compiled against the `PluginInterface` header. Qt loads a plugin only when the
class embeds `Q_PLUGIN_METADATA` for `com.nuchat.PluginInterface`. `Q_OBJECT`
and `Q_INTERFACES` are not a substitute: the loader reads the IID from the
plugin metadata and skips anything else.

On startup NUchat scans, in order, a non-empty `plugin/directory` (a leading
`~` expanded), `plugins/` next to the executable, `<build>/plugins` when
`<build>/CMakeCache.txt` shows this is a build tree,
`${CMAKE_INSTALL_LIBDIR}/nuchat/plugins` after install (also
`lib/<triplet>/nuchat/plugins` next to the executable), and
`~/.config/NUchat/plugins`. Directories that do not exist are skipped. An
installed `/usr/bin/nuchat` does not scan `/usr/plugins`. The Preferences
field for `plugin/directory` is disabled and its caption says the path is
not used. Saving Preferences still writes that field into the key (the
dialog's fallback text is `~/.config/NUchat/plugins`). The Load, Unload, and
Reload buttons on that page have no handlers. The sample CMake file writes
the library to `${CMAKE_BINARY_DIR}/plugins`.

---

> **Security Warning:** Plugins are native shared libraries that run with the
> same privileges as NUchat itself. A malicious plugin can read your files,
> access the network, and exfiltrate credentials. **Only install plugins from
> sources you fully trust.** NUchat verifies the plugin interface ID before
> loading, but does not perform code signing or sandboxing.

---

## Table of Contents

- [Quick Start](#quick-start)
- [PluginInterface](#plugininterface)
- [Building a Plugin](#building-a-plugin)
- [Plugin Lifecycle](#plugin-lifecycle)
- [IrcConnection API](#ircconnection-api)
- [Example Plugin](#example-plugin)

---

## Quick Start

1. Create a new directory under `plugins/` (e.g. `plugins/myplugin/`)
2. Implement `PluginInterface` in a QObject subclass and add
   `Q_PLUGIN_METADATA(IID PluginInterface_iid)` — without it the library is
   skipped
3. Add a `CMakeLists.txt` to build the shared library
4. Build — the library is written to `<build>/plugins` (not the source tree)
5. NUchat loads it automatically on startup

---

## PluginInterface

The plugin interface is defined in `src/PluginInterface.h`:

```cpp
#pragma once

#include <QObject>

class IrcConnection;

class PluginInterface
{
public:
    virtual ~PluginInterface() = default;

    // Called once when the plugin is loaded
    virtual void initialize(QObject *parent) = 0;

    // Called for a user "/command" and for a channel/query "!command".
    // Return true if the plugin handled the command (prevents further processing)
    virtual bool handleCommand(const QString &command,
                               const QStringList &args,
                               IrcConnection *connection)
    {
        Q_UNUSED(command); Q_UNUSED(args); Q_UNUSED(connection);
        return false;
    }
};

#define PluginInterface_iid "com.nuchat.PluginInterface"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
```

### Methods

| Method | Description |
|--------|-------------|
| `initialize(QObject *parent)` | Called once after the plugin is loaded. Use this for setup, signal connections, etc. |
| `handleCommand(command, args, connection)` | Called for a user-typed `/command` and for a `PRIVMSG`/`NOTICE` whose text starts with `!command`. `command` is lowercase (e.g. `"hello"`). `args` contains the remaining words. `connection` is the `IrcConnection*` for that server. Return `true` to consume the command so later plugins and scripts do not see it. |

---

## Building a Plugin

### Directory Structure

```
plugins/
└── myplugin/
    ├── CMakeLists.txt
    ├── MyPlugin.h
    └── MyPlugin.cpp
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyPlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

add_library(MyPlugin SHARED MyPlugin.cpp)

target_link_libraries(MyPlugin PRIVATE Qt6::Core Qt6::Network)

# Include NUchat headers (PluginInterface.h, IrcConnection.h)
target_include_directories(MyPlugin PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Output into <build>/plugins, which is where NUchat looks during development.
# On Windows the DLL is a RUNTIME output, so set both. Per-config generators
# otherwise write under plugins/Release.
set(_plugindir "${CMAKE_BINARY_DIR}/plugins")
set_target_properties(MyPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${_plugindir}"
    RUNTIME_OUTPUT_DIRECTORY "${_plugindir}"
)
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(MyPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${cfg} "${_plugindir}"
        RUNTIME_OUTPUT_DIRECTORY_${cfg} "${_plugindir}"
    )
endforeach()

# LIBRARY is the .so/.dylib; RUNTIME is the Windows DLL.
include(GNUInstallDirs)
install(TARGETS MyPlugin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/nuchat/plugins
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/nuchat/plugins)
```

### Header (MyPlugin.h)

```cpp
#pragma once

#include <QObject>
#include "PluginInterface.h"

class MyPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    void initialize(QObject *parent) override;
    bool handleCommand(const QString &command,
                       const QStringList &args,
                       IrcConnection *connection) override;
};
```

### Implementation (MyPlugin.cpp)

```cpp
#include "MyPlugin.h"
#include "IrcConnection.h"
#include <QDebug>

void MyPlugin::initialize(QObject *parent)
{
    Q_UNUSED(parent)
    qDebug() << "MyPlugin loaded!";
}

bool MyPlugin::handleCommand(const QString &command,
                              const QStringList &args,
                              IrcConnection *connection)
{
    if (command == "mycmd") {
        if (connection) {
            QString target = args.value(0, "#general");
            connection->sendMessage(target, "Hello from MyPlugin!");
        }
        return true;  // consumed
    }
    return false;  // pass to other handlers
}
```

---

## Plugin Lifecycle

1. **Discovery** — NUchat scans the directories listed at the top of this page for `.so` (Linux/BSD), `.dylib` (macOS), or `.dll` (Windows) files. A library is skipped unless its embedded Qt plugin IID is `com.nuchat.PluginInterface`.
2. **Loading** — Each shared library is loaded with `QPluginLoader`
3. **Initialization** — `initialize()` is called with the `PluginManager`
4. **Command dispatch** — `handleCommand()` runs when the user types `/command`, and when a `PRIVMSG` or `NOTICE` body starts with `!command` (IRCv3 `@tags` are ignored). The command name is lowercased. Plugins run before Python and Lua scripts. The first plugin to return `true` consumes the command.
5. **Unloading** — Plugins are unloaded when NUchat exits

---

## IrcConnection API

The `IrcConnection` pointer passed to `handleCommand()` provides these methods:

| Method | Description |
|--------|-------------|
| `sendRaw(const QString &line)` | Send a raw IRC protocol line |
| `sendMessage(const QString &target, const QString &message)` | Send a PRIVMSG |
| `sendNotice(const QString &target, const QString &notice)` | Send a NOTICE |
| `joinChannel(const QString &channel, const QString &key = "")` | Join a channel |
| `partChannel(const QString &channel, const QString &reason = "")` | Part a channel |
| `changeNick(const QString &newNick)` | Change nickname |
| `setAway(const QString &reason = "")` | Set away status |
| `setBack()` | Remove away status |
| `sendCtcp(const QString &target, const QString &cmd, const QString &args = "")` | Send a CTCP message |
| `who(const QString &mask)` | Send a WHO query |
| `whois(const QString &nick)` | Send a WHOIS query |
| `nickname()` | Get the current nickname (QString) |
| `serverHost()` | Get the server hostname (QString) |
| `isConnected()` | Check if registered to the server (bool) |

---

## Example Plugin

The bundled example plugin is in `plugins/exampleplugin/`. `ExamplePlugin.h` includes `Q_PLUGIN_METADATA(IID PluginInterface_iid)`, and its CMake file writes the library to `<build>/plugins`.

```cpp
#include "ExamplePlugin.h"
#include "IrcConnection.h"
#include <QDebug>

void ExamplePlugin::initialize(QObject *parent)
{
    Q_UNUSED(parent)
    qDebug() << "ExamplePlugin loaded";
}

bool ExamplePlugin::handleCommand(const QString &command,
                                   const QStringList &args,
                                   IrcConnection *connection)
{
    if (command == "hello") {
        if (connection) {
            connection->sendMessage("#general", "Hello from plugin!");
        }
        return true;
    }
    return false;
}
```

Type `/hello` in NUchat, or send `!hello` in a channel, to trigger the plugin. `/hello` is handled as a client command and is not sent to the server when the plugin returns `true`.

---

## Tips

- Plugins have full access to Qt APIs — you can create QTimers, network requests, file I/O, etc.
- Use `qDebug()` for logging — output appears in the terminal where NUchat was launched.
- Include `IrcConnection.h` to access the full IRC protocol interface.
- Plugins are loaded before scripts, so plugin commands take priority.
