#-------------------------------------------------
#
# Project created by QtCreator 2020-07-08T09:31:52
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VideoTracking
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        zmainui.cpp \
    zethercatthread.cpp \
    zgblpara.cpp \
    zcapturethread.cpp \
    zmatfifo.cpp \
    zprocessingthread.cpp \
    CSK_Tracker.cpp \
    zctrlbar.cpp \
    zdirectionbar.cpp

HEADERS += \
        zmainui.h \
    zethercatthread.h \
    zgblpara.h \
    zcapturethread.h \
    zmatfifo.h \
    zprocessingthread.h \
    CSK_Tracker.h \
    zctrlbar.h \
    zdirectionbar.h

INCLUDEPATH += /opt/ethercat/include
LIBS +=  -lethercat
LIBS += -lopencv_core -lopencv_flann -lopencv_videostab -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_objdetect -lopencv_superres -lopencv_photo -lopencv_calib3d  -lopencv_highgui  -lopencv_features2d -lopencv_stitching -lopencv_ml -lopencv_imgcodecs -lopencv_dnn -lopencv_shape

RESOURCES += \
    resource/style1.qrc

DISTFILES += \
    resource/skin_default.qss
