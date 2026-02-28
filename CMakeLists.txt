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

if(NOT Qt6Quick_FOUND)
    message(WARNING "Qt6 Quick component not found; QML UI will not be available. Install Qt6 Quick (qt6-declarative) for full functionality.")
endif()

if(NOT Python3_FOUND)
    message(WARNING "Python3 development libraries not found; Python scripting will be disabled. Install python3-dev for HexChat script support.")
endif()

include(GNUInstallDirs)

# add subdirectories
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)

# ── Install rules ──

# Desktop file
install(FILES resources/nuchat.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

# SVG icon (freedesktop hicolor theme)
install(FILES resources/icons/nuchat.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)

# Example scripts
install(DIRECTORY scripts/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/nuchat/scripts
        FILES_MATCHING PATTERN "*.py" PATTERN "*.js")

# Uninstall target (convenience)
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
