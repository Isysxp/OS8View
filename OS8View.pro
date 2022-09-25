#-------------------------------------------------
#
# Project created by QtCreator 2017-08-06T21:59:01
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = OS8View
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
QMAKE_LFLAGS += -no-pie
