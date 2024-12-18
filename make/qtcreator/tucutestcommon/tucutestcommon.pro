TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

include(../botan.pri)
include(../general.pri)
include(../tucucommon.pri)
include(../tinyjs.pri)
include(../gtest.pri)

config_coverage {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_CXXFLAGS += -O0
    QMAKE_LFLAGS += --coverage
    # After executing the tests, do:
    # gcovr . -r ../../../../../src/ -r ../../../../../src/ --html report.html
    # lcov --no-external --capture --directory . --base-directory ../../../../../src/ --output-file report.info
}

HEADERS +=

SOURCES += \
    ../../../test/tucucommon/gtest_common.cpp \
    ../../../test/tucucommon/gtest_componentmanager.cpp \
    ../../../test/tucucommon/gtest_datetime.cpp \
    ../../../test/tucucommon/gtest_iterator.cpp \
    ../../../test/tucucommon/gtest_logger.cpp \
    ../../../test/tucucommon/gtest_scriptengine.cpp \
    ../../../test/tucucommon/gtest_unit.cpp \
    ../../../test/tucucommon/gtest_xml.cpp \
    ../../../test/tucucommon/gtest_xmlimporter.cpp \
    #../../../test/tucucommon/tests.cpp


!win32 {
    # Because of Eigen:
    QMAKE_CXXFLAGS += -Wno-int-in-bool-context

    # Because of macros and clang:
    QMAKE_CXXFLAGS += -Wno-extra-semi-stmt
}
