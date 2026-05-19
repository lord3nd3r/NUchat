cmake_minimum_required(VERSION 3.16)
project(NUchat VERSION 1.0.0 LANGUAGES CXX)

# enable automoc so Qt signal/slot classes generate moc files automatically
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Quick QuickControls2)
find_package(Python3 COMPONENTS Development)

# Lua scripting (optional)
# CMake's built-in FindLua works on all platforms; fall back to pkg-config on Unix only.
find_package(Lua QUIET)
if(NOT LUA_FOUND AND UNIX)
    include(FindPkgConfig)
    pkg_check_modules(LUA lua5.4)
    if(NOT LUA_FOUND)
        pkg_check_modules(LUA lua54)
    endif()
    if(NOT LUA_FOUND)
        pkg_check_modules(LUA lua)
    endif()
endif()
if(LUA_FOUND AND NOT LUA_INCLUDE_DIRS)
    # Normalize: CMake FindLua uses LUA_INCLUDE_DIR (singular)
    set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
endif()

if(NOT Qt6Quick_FOUND)
    message(WARNING "Qt6 Quick component not found; QML UI may not work. Install qt6-declarative-dev (or equivalent) and re-run CMake.")
else()
    message(STATUS "Qt6::Quick found")
endif()

if(NOT Python3_FOUND)
    message(WARNING "Python3 development libraries not found; Python scripting will be disabled.")
endif()

if(NOT LUA_FOUND)
    message(WARNING "Lua development libraries not found; Lua scripting will be disabled.")
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
    
    # Install SVG icon
    install(FILES resources/icons/nuchat.svg
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
    
    # Install PNG icons at various sizes
    install(FILES resources/icons/nuchat-16.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-24.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-32.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-48.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-64.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-128.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-256.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
            RENAME nuchat.png)
    install(FILES resources/icons/nuchat-512.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
            RENAME nuchat.png)
    
    # Update icon cache after installation
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor ERROR_QUIET)")
endif()

# macOS app bundle
if(APPLE)
    install(TARGETS nuchat BUNDLE DESTINATION /Applications)
endif()

# Windows: install binary into a flat directory and deploy Qt DLLs
if(WIN32)
    install(TARGETS nuchat RUNTIME DESTINATION .)

    # Locate windeployqt next to qmake in the Qt bin directory
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        # Capture source-time paths so they expand correctly inside install(CODE)
        set(_nuchat_qml_dir "${CMAKE_SOURCE_DIR}/qml")
        install(CODE "
            message(STATUS \"Running windeployqt to deploy Qt runtime...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                        --no-translations
                        --qmldir \"${_nuchat_qml_dir}\"
                        \"\${CMAKE_INSTALL_PREFIX}/nuchat.exe\"
                RESULT_VARIABLE _wdq_result
                OUTPUT_VARIABLE _wdq_out
                ERROR_VARIABLE  _wdq_err
            )
            if(NOT _wdq_result EQUAL 0)
                message(WARNING \"windeployqt failed (exit \${_wdq_result}): \${_wdq_err}\")
            else()
                message(STATUS \"windeployqt finished successfully\")
            endif()
        ")
    else()
        message(WARNING
            "windeployqt not found – Qt DLLs will not be bundled automatically.\n"
            "Add the Qt bin directory to PATH (e.g. C:/Qt/6.x.y/msvc2022_64/bin) and re-run CMake.")
    endif()
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

# ── CPack (Installers) ──
set(CPACK_PACKAGE_NAME "NUchat")
set(CPACK_PACKAGE_VENDOR "lord3nd3r")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NUchat - IRC Client")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "NUchat")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "NUchat")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "nuchat.exe")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/lord3nd3r/NUchat")
    set(CPACK_PACKAGE_FILE_NAME "NUchat_${PROJECT_VERSION}_Windows_x64")
endif()

include(CPack)
