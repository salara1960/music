#-------------------------------------------------
#
# Project created by QtCreator 2016-05-28T17:07:01
#
#-------------------------------------------------

QT += widgets core gui

QMAKE_CXX += -std=gnu++11 -O2

INCLUDEPATH += /usr/include/fmodex/
LIBS += -L/usr/lib -lfmodex64

TARGET = qmusic
TEMPLATE = app

CONFIG += exception

SOURCES += main.cpp mainwindow.cpp

HEADERS += mainwindow.h

FORMS += mainwindow.ui

RESOURCES += icons.qrc


