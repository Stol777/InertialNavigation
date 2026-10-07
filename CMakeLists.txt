cmake_minimum_required(VERSION 3.16)

project(InertialNavigation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(InertialNavigation
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    simulationmodel.cpp
    simulationmodel.h
)

target_link_libraries(InertialNavigation PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(InertialNavigation PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(MSVC)
    target_compile_options(InertialNavigation PRIVATE /utf-8)
endif()
