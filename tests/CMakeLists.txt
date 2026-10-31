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

# ── test_secrets (keychain migration; in-memory backend, no OS keychain) ─────
add_executable(test_secrets test_secrets.cpp)
target_link_libraries(test_secrets PRIVATE Qt6::Test Qt6::Network nuchatcore)
target_include_directories(test_secrets PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME secrets COMMAND test_secrets)

# ── test_redact ───────────────────────────────────────────────────────────────
add_executable(test_redact test_redact.cpp)
target_link_libraries(test_redact PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_redact PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME redact COMMAND test_redact)

# ── test_messagemodel ─────────────────────────────────────────────────────────
add_executable(test_messagemodel test_messagemodel.cpp)
target_link_libraries(test_messagemodel PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_messagemodel PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME messagemodel COMMAND test_messagemodel)

# ── test_logger ───────────────────────────────────────────────────────────────
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_logger PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME logger COMMAND test_logger)

# ── test_managerrouting ───────────────────────────────────────────────────────
add_executable(test_managerrouting test_managerrouting.cpp)
target_link_libraries(test_managerrouting PRIVATE Qt6::Test Qt6::Widgets nuchatcore)
target_include_directories(test_managerrouting PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME managerrouting COMMAND test_managerrouting)
# QTEST_MAIN builds a QApplication because this target links Qt Widgets.
# Headless CI (Linux, ASan, FreeBSD) has no display; the offscreen plugin is
# part of qtbase and is enough for these routing tests.
set_tests_properties(managerrouting PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ── test_netsecurity ──────────────────────────────────────────────────────────
add_executable(test_netsecurity test_netsecurity.cpp)
target_link_libraries(test_netsecurity PRIVATE Qt6::Test Qt6::Network nuchatcore)
target_include_directories(test_netsecurity PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME netsecurity COMMAND test_netsecurity)

# ── test_protocol (ISUPPORT, casemapping, SASL, timers, DCC prefs, dicts) ──
add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE Qt6::Test Qt6::Widgets Qt6::Network nuchatcore)
target_include_directories(test_protocol PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME protocol COMMAND test_protocol)
# Same Widgets/QApplication constraint as managerrouting above.
set_tests_properties(protocol PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# Linking Qt Gui would otherwise mark these as GUI executables on Windows, so
# a failed QCOMPARE never reaches the ctest log.
if(WIN32)
    set(_nuchat_console_tests
        test_messageparser test_settings test_ircparsing test_hexchatimport
        test_secrets test_redact test_messagemodel test_logger
        test_managerrouting test_netsecurity test_protocol test_plugins)
    foreach(t IN LISTS _nuchat_console_tests)
        set_target_properties(${t} PROPERTIES WIN32_EXECUTABLE FALSE)
    endforeach()
endif()

