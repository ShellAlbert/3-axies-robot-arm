#-------------------------------------------------
#
# Project created by QtCreator 2020-07-08T09:31:52
#
#-------------------------------------------------

QT       += core gui network

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
    zgblpara.cpp \
    zcapturethread.cpp \
    zmatfifo.cpp \
    zprocessingthread.cpp \
    zctrlbar.cpp \
    zdirectionbar.cpp \
    zpidcalc.cpp \
    zdialoghome.cpp \
    zservothread.cpp \
    zdifffifo.cpp

HEADERS += \
        zmainui.h \
    zgblpara.h \
    zcapturethread.h \
    zmatfifo.h \
    zprocessingthread.h \
    zctrlbar.h \
    zdirectionbar.h \
    zpidcalc.h \
    zdialoghome.h \
    zservothread.h \
    zdifffifo.h


#openCV 4.1.1
INCLUDEPATH += /usr/local/include/opencv4
LIBS += -L/usr/local/lib
LIBS += -lopencv_cudabgsegm \
-lopencv_cudaobjdetect \
-lopencv_cudastereo \
-lopencv_dnn \
-lopencv_ml \
-lopencv_shape \
-lopencv_stitching \
-lopencv_cudafeatures2d \
-lopencv_superres \
-lopencv_cudacodec \
-lopencv_videostab \
-lopencv_cudaoptflow \
-lopencv_cudalegacy \
-lopencv_calib3d \
-lopencv_features2d \
-lopencv_highgui \
-lopencv_videoio \
-lopencv_photo \
-lopencv_imgcodecs \
-lopencv_cudawarping \
-lopencv_cudaimgproc \
-lopencv_cudafilters \
-lopencv_video \
-lopencv_objdetect \
-lopencv_imgproc \
-lopencv_flann \
-lopencv_cudaarithm \
-lopencv_core \
-lopencv_cudev \
-lopencv_tracking

#ethernet
INCLUDEPATH += -I/opt/ethercat/include
LIBS += -lethercat

RESOURCES += \
    resource/style1.qrc

DISTFILES += \
    resource/skin_default.qss
