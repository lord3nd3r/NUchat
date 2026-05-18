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
)

include(FindPkgConfig)
pkg_check_modules(HUNSPELL hunspell)
if(NOT HUNSPELL_FOUND)
    message(WARNING "hunspell library not found; spell checking disabled.")
else()
    list(APPEND CORE_SRC SpellChecker.cpp SpellHighlighter.cpp)
    list(APPEND CORE_HEADERS SpellChecker.h SpellHighlighter.h)
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
    Qt6::Quick
)

if(HUNSPELL_FOUND)
    target_include_directories(nuchatcore PRIVATE ${HUNSPELL_INCLUDE_DIRS})
    target_link_libraries(nuchatcore PRIVATE ${HUNSPELL_LIBRARIES})
    target_compile_definitions(nuchatcore PUBLIC HAVE_HUNSPELL=1)
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

# ── Main executable ──
# Use WIN32 subsystem on Windows (no console window) and a proper macOS bundle.
if(WIN32)
    add_executable(nuchat WIN32 main.cpp)
elseif(APPLE)
    add_executable(nuchat MACOSX_BUNDLE main.cpp)
    set_target_properties(nuchat PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER  "dev.nuchat.NUchat"
        MACOSX_BUNDLE_BUNDLE_VERSION  "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_NAME     "NUchat"
        MACOSX_BUNDLE_COPYRIGHT       "GPL-3.0"
    )
else()
    add_executable(nuchat main.cpp)
endif()

# include resources directly in the library so they are available
set(nuchatcore_RESOURCE_FILES resources.qrc)

target_link_libraries(nuchat PRIVATE nuchatcore Qt6::Gui Qt6::Widgets Qt6::Quick Qt6::QuickControls2)

# Windows: Winsock (needed transitively by Qt Network, but explicit is safer)
if(WIN32)
    target_link_libraries(nuchat PRIVATE ws2_32)
endif()

# ── Qt deployment helpers ──

# Windows: run windeployqt after build to gather Qt DLLs next to the .exe
if(WIN32)
    find_program(WINDEPLOYQT NAMES windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET nuchat POST_BUILD
            COMMAND "${WINDEPLOYQT}"
                --qmldir "${CMAKE_SOURCE_DIR}/qml"
                --no-translations
                "$<TARGET_FILE:nuchat>"
            COMMENT "Running windeployqt..."
            VERBATIM)
    endif()
endif()

# macOS: run macdeployqt after build to create a self-contained .app bundle
if(APPLE)
    find_program(MACDEPLOYQT NAMES macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT)
        add_custom_command(TARGET nuchat POST_BUILD
            COMMAND "${MACDEPLOYQT}"
                "$<TARGET_BUNDLE_DIR:nuchat>"
                -qmldir="${CMAKE_SOURCE_DIR}/qml"
            COMMENT "Running macdeployqt..."
            VERBATIM)
    endif()
endif()

# Install the binary (Linux / BSD)
if(NOT WIN32 AND NOT APPLE)
    install(TARGETS nuchat RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
