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

# ── test_secrets (keychain migration; in-memory backend, no OS keychain) ─────
add_executable(test_secrets test_secrets.cpp)
target_link_libraries(test_secrets PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_secrets PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME secrets COMMAND test_secrets)

# ── test_redact ───────────────────────────────────────────────────────────────
add_executable(test_redact test_redact.cpp)
target_link_libraries(test_redact PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_redact PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME redact COMMAND test_redact)

# ── test_logger ───────────────────────────────────────────────────────────────
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE Qt6::Test nuchatcore)
target_include_directories(test_logger PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME logger COMMAND test_logger)

