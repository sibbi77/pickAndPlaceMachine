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
LIBS += -lQVTK -lvtkCommon -lvtkRendering -lvtkGraphics -lvtkFiltering -lvtkGenericFiltering

# gmp
LIBS += -lgmpxx -lgmp

SOURCES += main.cpp\
        mainwindow.cpp \
    gerberimporter.cpp \
    csvimporter.cpp \
    centroid.cpp \
    centroiddialog.cpp \
    graphicsview.cpp

HEADERS  += mainwindow.h \
    gerberimporter.h \
    csvimporter.h \
    centroid.h \
    centroiddialog.h \
    graphicsview.h

FORMS    += mainwindow.ui \
    centroiddialog.ui

RESOURCES += \
    pickAndPlaceMachine.qrc
