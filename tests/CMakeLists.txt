# Tests

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_messageparser test_messageparser.cpp)

target_link_libraries(test_messageparser PRIVATE Qt6::Test nuchatcore)

target_include_directories(test_messageparser PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME messageparser COMMAND test_messageparser)
