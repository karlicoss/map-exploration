######################################################################
# Automatically generated by qmake (2.01a) ?? ???. 8 20:30:00 2011
######################################################################

CONFIG += qt debug_and_release

TEMPLATE = app

DEPENDPATH += .
INCLUDEPATH += .

CONFIG(debug, debug|release) {
     TARGET = mapexploration_debug
 } else {
     TARGET = mapexploration
 }

QMAKE_CXXFLAGS_DEBUG += -g -DDEBUG

# Input
HEADERS += Visualisation.h editor/MapEditor.h \
    tools.h \
    editor/EditArea.h \
    MapExploration.h
SOURCES += main.cpp Visualisation.cpp editor/MapEditor.cpp \
    tools.cpp \
    editor/EditArea.cpp \
    MapExploration.cpp

OTHER_FILES += \
    README \
    sample.map
