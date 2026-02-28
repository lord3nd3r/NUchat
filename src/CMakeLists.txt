# Source directory for NUchat

# core library containing IRC logic and utility classes
set(CORE_SRC
    IrcConnection.cpp
    IRCConnectionManager.cpp
    MessageParser.cpp
    PluginManager.cpp
    Settings.cpp
    Logger.cpp
    NotificationManager.cpp
    ThemeManager.cpp
    ServerChannelModel.cpp
    MessageModel.cpp
    ScriptManager.cpp
    ImageDownloader.cpp
    SpellChecker.cpp
    SpellHighlighter.cpp
)

set(CORE_HEADERS
    IrcConnection.h
    IRCConnectionManager.h
    MessageParser.h
    PluginInterface.h
    PluginManager.h
    Settings.h
    Logger.h
    NotificationManager.h
    ThemeManager.h
    ServerChannelModel.h
    MessageModel.h
    ScriptManager.h
    ImageDownloader.h
    SpellChecker.h
    SpellHighlighter.h
)

include(FindPkgConfig)
pkg_check_modules(HUNSPELL hunspell)
if(NOT HUNSPELL_FOUND)
    message(WARNING "hunspell library not found; spell checking disabled.")
endif()

# Python scripting engine (optional, requires Python3 dev)
if(Python3_FOUND)
    list(APPEND CORE_SRC PythonScriptEngine.cpp)
    list(APPEND CORE_HEADERS PythonScriptEngine.h)
endif()

# Lua scripting engine (optional, requires Lua 5.4 dev)
if(LUA_FOUND)
    list(APPEND CORE_SRC LuaScriptEngine.cpp)
    list(APPEND CORE_HEADERS LuaScriptEngine.h)
endif()

# include qml and theme resources
set(nuchatcore_RESOURCE_FILES resources.qrc)
qt6_add_resources(nuchatcore_resources ${nuchatcore_RESOURCE_FILES})

add_library(nuchatcore STATIC ${CORE_SRC} ${CORE_HEADERS} ${nuchatcore_resources})

target_link_libraries(nuchatcore PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Widgets
)

if(HUNSPELL_FOUND)
    target_include_directories(nuchatcore PRIVATE ${HUNSPELL_INCLUDE_DIRS})
    target_link_libraries(nuchatcore PRIVATE ${HUNSPELL_LIBRARIES})
    target_compile_definitions(nuchatcore PUBLIC HAVE_HUNSPELL=1)
    if(Qt6Quick_FOUND)
        target_link_libraries(nuchatcore PRIVATE Qt6::Quick)
    endif()
endif()

if(Python3_FOUND)
    target_include_directories(nuchatcore PRIVATE ${Python3_INCLUDE_DIRS})
    target_link_libraries(nuchatcore PRIVATE ${Python3_LIBRARIES})
    target_compile_definitions(nuchatcore PRIVATE HAVE_PYTHON=1)
endif()

if(LUA_FOUND)
    target_include_directories(nuchatcore PRIVATE ${LUA_INCLUDE_DIRS})
    target_link_libraries(nuchatcore PRIVATE ${LUA_LIBRARIES})
    target_compile_definitions(nuchatcore PRIVATE HAVE_LUA=1)
endif()

target_compile_definitions(nuchatcore PRIVATE QT_DEPRECATED_WARNINGS)

target_include_directories(nuchatcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# main executable
add_executable(nuchat main.cpp)

# include resources directly in the library so they are available
set(nuchatcore_RESOURCE_FILES resources.qrc)

target_link_libraries(nuchat PRIVATE nuchatcore Qt6::Gui Qt6::Widgets Qt6::QuickControls2)

if(Qt6Quick_FOUND)
    target_link_libraries(nuchat PRIVATE Qt6::Quick)
endif()

# Install the binary
install(TARGETS nuchat RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
