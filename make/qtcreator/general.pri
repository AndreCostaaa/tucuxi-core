

CONFIG += c++14
CONFIG -= qt

#LIBS += ../../../libs/tiny-js-master-20170629/objs/tinyjs.a \
#        ../../../libs/botan-2.1.0/objs/botan.a

LIBS += $$PWD/../../bin/tinyjs.a \
        $$PWD/../../bin/botan.a

QMAKE_LFLAGS += -Wl,--no-as-needed -ldl

INCLUDEPATH += $$PWD/../../src \
    $$PWD/../../libs/spdlog-master-20170622/include/ \
    $$PWD/../../libs/boost-1.63.0 \
    $$PWD/../../libs/eigen-3.3.2/ \
    $$PWD/../../libs/botan-2.1.0/build/include/ \
    $$PWD/../../libs/eigen-3.3.2/ \
    $$PWD/../../libs/fructose-1.3/fructose/include \
    $$PWD/../../libs/date-master-20170711 \
    $$PWD/../../libs/tiny-js-master-20170629 \
    $$PWD/../../libs/rapidxml-1.13
