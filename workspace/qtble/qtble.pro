include(../GESys/include.pro)
TEMPLATE = app
TARGET = qtble
BUILD_PATH = .build/release
QMAKE_CFLAGS += -O2 \
    -std=c11 \
    -Wall \
    -pedantic
QMAKE_CXXFLAGS += -O2 \
    -std=c++11 \
    -Wall \
    -pedantic
CODECFORTR = UTF-8
CODECFORSRC = UTF-8
CONFIG *= link_pkgconfig \
    silent
LIBS += -lcurl
debug { 
    DEFINES += DEBUG
    QMAKE_CFLAGS += -g \
        -O0
    QMAKE_CXXFLAGS += -g \
        -O0
    QMAKE_LFLAGS += -g \
        -rdynamic
    BUILD_PATH = .build/debug
}
MOC_DIR = $$BUILD_PATH/moc
UI_DIR = $$BUILD_PATH/ui
OBJECTS_DIR = $$BUILD_PATH/obj
RCC_DIR = $$BUILD_PATH/res
QT += core \
    gui
SOURCES += main.cpp \
    qtble.cpp
HEADERS += credentials.h \
    qtble.h
FORMS += qtble.ui
RESOURCES += 
