#-------------------------------------------------
#
# Project created by QtCreator 2012-07-26T20:02:53
#
#-------------------------------------------------

QT       += core gui

TARGET = pickAndPlaceMachine
TEMPLATE = app

# vtk
INCLUDEPATH += /usr/include/vtk-5.8
LIBS += -lQVTK -lvtkCommon -lvtkRendering -lvtkGraphics -lvtkFiltering

# gmp
LIBS += -lgmpxx -lgmp

SOURCES += main.cpp\
        mainwindow.cpp \
    gerberimporter.cpp

HEADERS  += mainwindow.h \
    gerberimporter.h

FORMS    += mainwindow.ui
