TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lavcodec -lavformat -lswscale -lavutil

SOURCES += main.cpp
