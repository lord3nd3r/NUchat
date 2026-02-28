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

# Python scripting engine (optional, requires Python3 dev)
if(Python3_FOUND)
    list(APPEND CORE_SRC PythonScriptEngine.cpp)
    list(APPEND CORE_HEADERS PythonScriptEngine.h)
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

if(Python3_FOUND)
    target_include_directories(nuchatcore PRIVATE ${Python3_INCLUDE_DIRS})
    target_link_libraries(nuchatcore PRIVATE ${Python3_LIBRARIES})
    target_compile_definitions(nuchatcore PRIVATE HAVE_PYTHON=1)
endif()

target_compile_definitions(nuchatcore PRIVATE QT_DEPRECATED_WARNINGS)

target_include_directories(nuchatcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# main executable
add_executable(NUchat main.cpp)

# include resources directly in the library so they are available
set(nuchatcore_RESOURCE_FILES resources.qrc)

target_link_libraries(NUchat PRIVATE nuchatcore Qt6::Gui Qt6::Widgets Qt6::QuickControls2)

if(Qt6Quick_FOUND)
    target_link_libraries(NUchat PRIVATE Qt6::Quick)
endif()

# Install the binary
install(TARGETS NUchat RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
