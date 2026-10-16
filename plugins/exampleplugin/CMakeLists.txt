cmake_minimum_required(VERSION 3.16)
project(ExamplePlugin)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

add_library(ExamplePlugin SHARED ExamplePlugin.cpp)

target_link_libraries(ExamplePlugin PRIVATE Qt6::Core Qt6::Network)

target_include_directories(ExamplePlugin PRIVATE ${CMAKE_SOURCE_DIR}/src)

# In-tree builds look in <build>/plugins (the executable lives in <build>/src).
# Do not write the library into the source tree.
set(_nuchat_plugin_dir "${CMAKE_BINARY_DIR}/plugins")
set_target_properties(ExamplePlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${_nuchat_plugin_dir}"
    RUNTIME_OUTPUT_DIRECTORY "${_nuchat_plugin_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${_nuchat_plugin_dir}"
)
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(ExamplePlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${cfg} "${_nuchat_plugin_dir}"
        RUNTIME_OUTPUT_DIRECTORY_${cfg} "${_nuchat_plugin_dir}"
    )
endforeach()

include(GNUInstallDirs)
# LIBRARY is the .so/.dylib; RUNTIME is the Windows DLL.
install(TARGETS ExamplePlugin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/nuchat/plugins
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/nuchat/plugins)
