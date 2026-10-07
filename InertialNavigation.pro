QT += widgets

TEMPLATE = app
TARGET = InertialNavigation
CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    simulationmodel.cpp

HEADERS += \
    mainwindow.h \
    simulationmodel.h

FORMS += mainwindow.ui

msvc: QMAKE_CXXFLAGS += /utf-8
