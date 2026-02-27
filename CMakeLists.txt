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

# add subdirectories
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
