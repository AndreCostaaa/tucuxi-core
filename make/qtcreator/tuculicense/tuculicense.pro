TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

TARGET      = tuculicense

LIBS += -lpthread


include(../general.pri)
include(../tucucommon.pri)

SOURCES += \
    ../../../src/tuculicense/main.cpp
