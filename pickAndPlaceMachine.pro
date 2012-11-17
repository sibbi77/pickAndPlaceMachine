# ---
# To build pickAndPlaceMachine you need to customize this file
#
# Windows:
#  - Mingw: get the dependencies "vtk" and "gmp" and specify the correct paths
#    TESTED: gcc-4.4.0 (from QtSDK)
#            gmp-5.0.5
#            Qt-4.8.1
#
# Linux:
#  - gcc: vtk-5.8 should work out of the box; otherwise specify the correct include path
#    TESTED: Ubuntu 12.04, Ubuntu 12.10
# ---

QT       += core gui

TARGET = pickAndPlaceMachine
TEMPLATE = app

QMAKE_CXXFLAGS_DEBUG = "-ggdb -O0 -Wall -Wextra"

# vtk
unix:INCLUDEPATH += /usr/include/vtk-5.8
win32:INCLUDEPATH += c:/dev/projects/openems-w32/vtk/include/vtk-5.10
win32:LIBS += -Lc:/dev/projects/openems-w32/vtk/bin
LIBS += -lQVTK -lvtkCommon -lvtkRendering -lvtkGraphics -lvtkFiltering -lvtkGenericFiltering

# gmp
win32:INCLUDEPATH += c:/dev/projects/gmp-bin/include
win32:LIBS += -Lc:/dev/projects/gmp-bin/lib
LIBS += -lgmpxx -lgmp


SOURCES += main.cpp\
    mainwindow.cpp \
    gerberimporter.cpp \
    csvimporter.cpp \
    centroid.cpp \
    centroiddialog.cpp \
    graphicsview.cpp \
    excellonimporter.cpp

HEADERS  += mainwindow.h \
    gerberimporter.h \
    csvimporter.h \
    centroid.h \
    centroiddialog.h \
    graphicsview.h \
    excellonimporter.h

FORMS    += mainwindow.ui \
    centroiddialog.ui

RESOURCES += \
    pickAndPlaceMachine.qrc
