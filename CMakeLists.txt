cmake_minimum_required(VERSION 3.20)
project(securecapture VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Qml Widgets)
find_package(KF6 REQUIRED COMPONENTS Config CoreAddons)

find_path(KWIN_INCLUDE_DIR effect/effect.h PATH_SUFFIXES kwin REQUIRED)
find_library(KWIN_LIBRARY NAMES kwin REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
add_subdirectory(src)
add_subdirectory(applet)
