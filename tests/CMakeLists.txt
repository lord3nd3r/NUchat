# Tests

find_package(Qt6 REQUIRED COMPONENTS Test Qml)

# ── test_messageparser ────────────────────────────────────────────────────────
add_executable(test_messageparser test_messageparser.cpp)
target_link_libraries(test_messageparser PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_messageparser PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME messageparser COMMAND test_messageparser)

# ── test_settings ─────────────────────────────────────────────────────────────
add_executable(test_settings test_settings.cpp)
target_link_libraries(test_settings PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_settings PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME settings COMMAND test_settings)

# ── test_ircparsing ───────────────────────────────────────────────────────────
add_executable(test_ircparsing test_ircparsing.cpp)
target_link_libraries(test_ircparsing PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_ircparsing PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME ircparsing COMMAND test_ircparsing)

# ── test_hexchatimport ────────────────────────────────────────────────────────
add_executable(test_hexchatimport test_hexchatimport.cpp)
target_link_libraries(test_hexchatimport PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_hexchatimport PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME hexchatimport COMMAND test_hexchatimport)

# ── test_netsecurity ──────────────────────────────────────────────────────────
add_executable(test_netsecurity test_netsecurity.cpp)
target_link_libraries(test_netsecurity PRIVATE Qt6::Test Qt6::Network nuchatcore)
target_include_directories(test_netsecurity PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME netsecurity COMMAND test_netsecurity)

# ── test_plugins (C++ plugins, JS, Python/Lua hook dispatch) ─────────────────
add_executable(test_plugins test_plugins.cpp)
# Qt6::Qml so ScriptManager.h sees QJSEngine, matching nuchatcore's layout.
target_link_libraries(test_plugins PRIVATE Qt6::Test Qt6::Qml nuchatcore)
target_include_directories(test_plugins PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_plugins PRIVATE
    "NUCHAT_PLUGIN_BUILD_DIR=\"${CMAKE_BINARY_DIR}/plugins\"")
if(Python3_FOUND)
    target_compile_definitions(test_plugins PRIVATE HAVE_PYTHON=1)
    target_include_directories(test_plugins PRIVATE ${Python3_INCLUDE_DIRS})
    target_link_libraries(test_plugins PRIVATE ${Python3_LIBRARIES})
endif()
if(LUA_FOUND)
    target_compile_definitions(test_plugins PRIVATE HAVE_LUA=1)
    target_include_directories(test_plugins PRIVATE ${LUA_INCLUDE_DIRS})
    target_link_libraries(test_plugins PRIVATE ${LUA_LIBRARIES})
endif()
add_dependencies(test_plugins ExamplePlugin)
add_test(NAME plugins COMMAND test_plugins)

