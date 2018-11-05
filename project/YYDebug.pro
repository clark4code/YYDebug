#-------------------------------------------------
#
# Project created by QtCreator 2015-01-30T15:12:18
#
#-------------------------------------------------

QT       += core gui serialport xml network script

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = YYDebug
TEMPLATE = app
CONFIG += c++11
VERSION = 1.0.0.0

RC_ICONS = logo.ico
QMAKE_TARGET_COMPANY = Freeman
QMAKE_TARGET_PRODUCT = YYDebug
QMAKE_TARGET_DESCRIPTION = The debugging tool of YYDebug
QMAKE_TARGET_COPYRIGHT = Copyrights 2014-2015 MrY .All rights reserved.

SOURCES += main.cpp\
        ymainwindow.cpp \
    yuartdock.cpp \
    sobject.cpp \
    ycommdock.cpp \
    yusbdock.cpp \
    sthread.cpp \
    ynetdock.cpp

HEADERS  += ymainwindow.h \
    yuartdock.h \
    sobject.h \
    ycommdock.h \
    yusbdock.h \
    sthread.h \
    ynetdock.h

unix:INCLUDEPATH += /usr/local/include/libusb-1.0
unix:LIBS += -L/usr/local/lib -lusb-1.0

win32: LIBS += -L$$PWD/libusb/ -llibusb-1.0
win32: INCLUDEPATH += $$PWD/libusb
win32: DEPENDPATH += $$PWD/libusb
