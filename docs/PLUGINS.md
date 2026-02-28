# NUchat C++ Plugin API

NUchat supports C++ plugins loaded as shared libraries at runtime. Plugins are
compiled against the `PluginInterface` header and placed in the `plugins/` directory.

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
2. Implement `PluginInterface` in a QObject subclass
3. Add a `CMakeLists.txt` to build the shared library
4. Build — the `.so` is placed in the `plugins/` directory
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

    // Called when a user command is dispatched
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
| `handleCommand(command, args, connection)` | Called when the user enters a command. `command` is lowercase (e.g. `"hello"`). `args` contains space-separated arguments. `connection` is the active `IrcConnection*`. Return `true` to consume the command. |

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

add_library(MyPlugin SHARED MyPlugin.cpp)

target_link_libraries(MyPlugin PRIVATE Qt6::Core Qt6::Network)

# Include NUchat headers (PluginInterface.h, IrcConnection.h)
target_include_directories(MyPlugin PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Output to the plugins/ directory
set_target_properties(MyPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../plugins"
)

# Install
install(TARGETS MyPlugin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/nuchat/plugins)
```

### Header (MyPlugin.h)

```cpp
#pragma once

#include <QObject>
#include "PluginInterface.h"

class MyPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
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

1. **Discovery** — NUchat scans the `plugins/` directory for `.so` files on startup
2. **Loading** — Each shared library is loaded with `QPluginLoader`
3. **Initialization** — `initialize()` is called with the parent `QObject`
4. **Command dispatch** — When a user types a command, `handleCommand()` is called on each
   plugin in load order. The first plugin to return `true` consumes the command.
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

The bundled example plugin is in `plugins/exampleplugin/`. It demonstrates:

- Implementing `PluginInterface`
- Handling a custom `/hello` command
- Sending a message to a channel via `IrcConnection`

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

Type `/hello` in NUchat to trigger the plugin.

---

## Tips

- Plugins have full access to Qt APIs — you can create QTimers, network requests, file I/O, etc.
- Use `qDebug()` for logging — output appears in the terminal where NUchat was launched.
- Include `IrcConnection.h` to access the full IRC protocol interface.
- Plugins are loaded before scripts, so plugin commands take priority.
