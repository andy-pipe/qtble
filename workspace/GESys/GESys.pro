TEMPLATE = lib
CONFIG *= staticlib create_prl link_pkgconfig silent
TARGET = GESys
BUILD_PATH = .build/release

DEPENDPATH += .
INCLUDEPATH += .

QMAKE_CFLAGS += -O2 -std=c11 -Wall -pedantic
QMAKE_CXXFLAGS += -O2 -std=c++11 -Wall -pedantic
CODECFORTR = UTF-8
CODECFORSRC = UTF-8

debug {
	DEFINES += DEBUG
	QMAKE_CFLAGS += -g -O0
	QMAKE_CXXFLAGS += -g -O0
	QMAKE_LFLAGS += -g -rdynamic
	BUILD_PATH = .build/debug
}

MOC_DIR = $$BUILD_PATH/moc
UI_DIR = $$BUILD_PATH/ui
OBJECTS_DIR = $$BUILD_PATH/obj
RCC_DIR = $$BUILD_PATH/res

QT += \
	core

SOURCES += \
	ge_persistent_value.cpp \
	ge_sysfile.cpp \
	ge_system_load.cpp

HEADERS += \
	ge_persistent_value.h \
	ge_sysfile.h \
	ge_system_load.h
