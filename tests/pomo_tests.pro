QT += core gui quick quickcontrols2 testlib
CONFIG += c++17 testcase
TARGET = pomo_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/theme.h ../src/translator.h
SOURCES += pomo_tests.cpp ../src/backend.cpp ../src/theme.cpp ../src/translator.cpp
