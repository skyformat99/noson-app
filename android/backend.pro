TEMPLATE = lib
TARGET = $$qtLibraryTarget(NosonAppbackend)
QT += qml quick 
CONFIG += qt plugin c++11
CONFIG -= android_install
uri = NosonApp

DESTDIR = $$OUT_PWD/qml/$$replace(uri, \., /)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:LIBS += -L$$[QT_INSTALL_LIBS] -lnoson -lz -lssl -lcrypto

INCLUDEPATH += $$[QT_INSTALL_HEADERS]/noson

HEADERS += ../backend/modules/NosonApp/tools.h
HEADERS += ../backend/modules/NosonApp/backend.h
HEADERS += ../backend/modules/NosonApp/sonos.h
HEADERS += ../backend/modules/NosonApp/player.h
HEADERS += ../backend/modules/NosonApp/listmodel.h
HEADERS += ../backend/modules/NosonApp/albumsmodel.h
HEADERS += ../backend/modules/NosonApp/artistsmodel.h
HEADERS += ../backend/modules/NosonApp/genresmodel.h
HEADERS += ../backend/modules/NosonApp/tracksmodel.h
HEADERS += ../backend/modules/NosonApp/queuemodel.h
HEADERS += ../backend/modules/NosonApp/radiosmodel.h
HEADERS += ../backend/modules/NosonApp/playlistsmodel.h
HEADERS += ../backend/modules/NosonApp/zonesmodel.h
HEADERS += ../backend/modules/NosonApp/renderingmodel.h
HEADERS += ../backend/modules/NosonApp/roomsmodel.h
HEADERS += ../backend/modules/NosonApp/favoritesmodel.h
HEADERS += ../backend/modules/NosonApp/servicesmodel.h
HEADERS += ../backend/modules/NosonApp/mediamodel.h

SOURCES += ../backend/modules/NosonApp/backend.cpp
SOURCES += ../backend/modules/NosonApp/sonos.cpp
SOURCES += ../backend/modules/NosonApp/player.cpp
SOURCES += ../backend/modules/NosonApp/listmodel.cpp
SOURCES += ../backend/modules/NosonApp/albumsmodel.cpp
SOURCES += ../backend/modules/NosonApp/artistsmodel.cpp
SOURCES += ../backend/modules/NosonApp/genresmodel.cpp
SOURCES += ../backend/modules/NosonApp/tracksmodel.cpp
SOURCES += ../backend/modules/NosonApp/queuemodel.cpp
SOURCES += ../backend/modules/NosonApp/radiosmodel.cpp
SOURCES += ../backend/modules/NosonApp/playlistsmodel.cpp
SOURCES += ../backend/modules/NosonApp/zonesmodel.cpp
SOURCES += ../backend/modules/NosonApp/renderingmodel.cpp
SOURCES += ../backend/modules/NosonApp/roomsmodel.cpp
SOURCES += ../backend/modules/NosonApp/favoritesmodel.cpp
SOURCES += ../backend/modules/NosonApp/servicesmodel.cpp
SOURCES += ../backend/modules/NosonApp/mediamodel.cpp

#Copies the qmldir file to the build directory
!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$DESTDIR/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/../backend/modules/NosonApp/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

#Copies the qmldir file and the built plugin .so to the QT_INSTALL_QML directory
qmldir.files = ../backend/modules/NosonApp/qmldir

android {
    target.path = $$DESTDIR/$$replace(uri, \., /)
    qmldir.path = $$DESTDIR/$$replace(uri, \., /)
    INSTALLS += target qmldir
}
#unix {
#    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
#    qmldir.path = $$installPath
#    target.path = $$installPath
#    INSTALLS += target qmldir
#}
