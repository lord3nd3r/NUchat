# Tests

find_package(Qt6 REQUIRED COMPONENTS Test)

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

