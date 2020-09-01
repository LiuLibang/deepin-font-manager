
QT += core gui svg sql xml dtkwidget dtkgui dbus testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = deepin-font-manager

CONFIG += c++11 link_pkgconfig
PKGCONFIG += freetype2 fontconfig

INCLUDEPATH += $$PWD/../libdeepin-font-manager \
               $$PWD/interfaces
LIBS += -L$$OUT_PWD/../../build-font-manager-unknown-Debug/libdeepin-font-manager -ldeepin-font-manager
#error(-L$$OUT_PWD)

DEFINES += QT_MESSAGELOGCONTEXT

include(src.pri)

RESOURCES += ../deepin-font-manager-assets/deepin-font-manager.qrc
TRANSLATIONS += ../translations/deepin-font-manager.ts


isEmpty(BINDIR):BINDIR=/usr/bin
isEmpty(APPDIR):APPDIR=/usr/share/applications
isEmpty(DSRDIR):DSRDIR=/usr/share/deepin-font-manager

target.path = $$INSTROOT$$BINDIR
desktop.path = $$INSTROOT$$APPDIR
desktop.files = $$PWD/deepin-font-manager.desktop

# Automating generation .qm files from .ts files
!system($$PWD/translate_generation.sh): error("Failed to generate translation")

QMAKE_CXXFLAGS += -g -Wall -fprofile-arcs -ftest-coverage -O0
QMAKE_LFLAGS += -g -Wall -fprofile-arcs -ftest-coverage  -O0

include(../third-party/googletest/gtest_dependency.pri)
include(tests/test.pri)


translations.path = /usr/share/deepin-font-manager/translations
translations.files = $$PWD/../translations/*.qm

icon_files.path = /usr/share/icons/hicolor/scalable/apps
icon_files.files = $$PWD/../deepin-font-manager-assets/deepin-font-manager.svg

INSTALLS += target desktop translations icon_files

DISTFILES +=
