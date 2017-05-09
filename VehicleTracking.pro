# -------------------------------------------------
# Project created by QtCreator 2017-03-08T09:10:10
# -------------------------------------------------
QT += network \
    sql
QT -= gui
TARGET = VehicleTracking
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += src/main.cpp \
    src/vehicletrackingserver.cpp \
    src/cprtfcdatabase.cpp \
    src/vehicleconnection.cpp
HEADERS += src/vehicletrackingserver.h \
    src/cprtfcdatabase.h \
    src/vehicleconnection.h
use_real_mysql { 
    SOURCES += src/qsql_mysql.cpp
    HEADERS += src/qsql_mysql.h
    DEFINES += USE_REAL_MYSQL_CONNECTION
    LIBS += -L/usr/lib/mysql \
        -L/usr/lib64/mysql \
        -lmysqlclient
}
unix { 
    UI_DIR = build/ui
    OBJECTS_DIR = build/obj
    MOC_DIR = build/moc
}
DESTDIR = ./bin/

