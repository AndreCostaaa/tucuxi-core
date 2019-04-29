TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

TARGET      = tucucli

LIBS += -lpthread


include(../general.pri)
include(../tucucommon.pri)
include(../tucucore.pri)
include(../tucuquery.pri)

HEADERS += \
    ../../../src/tucucli/clicomputer.h

SOURCES += \
    ../../../src/tucucli/tucucli.cpp \
    ../../../src/tucucli/clicomputer.cpp