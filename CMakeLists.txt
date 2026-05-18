cmake_minimum_required(VERSION 3.16)
project(NUchat LANGUAGES CXX)

# enable automoc so Qt signal/slot classes generate moc files automatically
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find Qt6 components; Quick is optional for headless builds
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent QuickControls2)
find_package(Qt6 COMPONENTS Quick)
find_package(Python3 COMPONENTS Development)

# Lua scripting (optional)
# CMake's built-in FindLua works on all platforms; fall back to pkg-config on Unix.
find_package(Lua QUIET)
if(LUA_FOUND)
    # Normalize: CMake FindLua uses LUA_INCLUDE_DIR (singular)
    if(NOT LUA_INCLUDE_DIRS)
        set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
    endif()
else()
    include(FindPkgConfig)
    pkg_check_modules(LUA lua5.4)
    if(NOT LUA_FOUND)
        pkg_check_modules(LUA lua54)
    endif()
    if(NOT LUA_FOUND)
        pkg_check_modules(LUA lua)
    endif()
endif()

if(NOT Qt6Quick_FOUND)
    message(FATAL_ERROR "Qt6 Quick component not found; NUchat requires Qt6::Quick for its QML UI. Install Qt6 Quick (qt6-declarative) and re-run CMake.")
endif()

if(NOT Python3_FOUND)
    message(WARNING "Python3 development libraries not found; Python scripting will be disabled. Install python3-dev for HexChat script support.")
endif()

if(NOT LUA_FOUND)
    message(WARNING "Lua development libraries not found; Lua scripting will be disabled. Install liblua5.4-dev for HexChat Lua script support.")
endif()

include(GNUInstallDirs)

# enable CTest support
enable_testing()

# add subdirectories
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)

# ── Install rules ──

# Desktop file and icon (Linux / freedesktop platforms only)
if(UNIX AND NOT APPLE)
    install(FILES resources/nuchat.desktop
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES resources/icons/nuchat.svg
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
endif()

# macOS app bundle
if(APPLE)
    install(TARGETS nuchat BUNDLE DESTINATION /Applications)
endif()

# Windows: install binary into a flat directory
if(WIN32)
    install(TARGETS nuchat RUNTIME DESTINATION .)
endif()

# Example scripts (all platforms)
install(DIRECTORY scripts/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/nuchat/scripts
        FILES_MATCHING PATTERN "*.py" PATTERN "*.lua" PATTERN "*.js")

# Uninstall target (convenience)
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
