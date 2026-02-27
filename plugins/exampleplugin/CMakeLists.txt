cmake_minimum_required(VERSION 3.16)
project(ExamplePlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(ExamplePlugin SHARED ExamplePlugin.cpp)

target_link_libraries(ExamplePlugin PRIVATE Qt6::Core Qt6::Network)

target_include_directories(ExamplePlugin PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_target_properties(ExamplePlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../plugins"
)
