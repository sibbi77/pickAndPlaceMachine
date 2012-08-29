#-------------------------------------------------
#
# Project created by QtCreator 2012-07-26T20:02:53
#
#-------------------------------------------------

QT       += core gui

TARGET = pickAndPlaceMachine
TEMPLATE = app

QMAKE_CXXFLAGS_DEBUG = "-ggdb -O0"

# vtk
INCLUDEPATH += /usr/include/vtk-5.8
LIBS += -lQVTK -lvtkCommon -lvtkRendering -lvtkGraphics -lvtkFiltering

# gmp
LIBS += -lgmpxx -lgmp

SOURCES += main.cpp\
        mainwindow.cpp \
    gerberimporter.cpp \
    csvimporter.cpp \
    centroid.cpp

HEADERS  += mainwindow.h \
    gerberimporter.h \
    csvimporter.h \
    centroid.h

FORMS    += mainwindow.ui
