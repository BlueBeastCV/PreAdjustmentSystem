QT       += core gui sql xlsx

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        dataoper.cpp \
        frmmessagebox.cpp \
        iconhelper.cpp \
        Snap7_sdk/plc_siemens.cpp \
        Snap7_sdk/s7.cpp \
        Snap7_sdk/snap7.cpp \
        Snap7_sdk/plcthread.cpp \
        loginform.cpp \
        rangesetting.cpp \
        savelog.cpp \
        titlewidget.cpp \
        qtstreambuf.cpp \
        BaseGraphicsview.cpp \
        limitform.cpp \
        ImageWidget.cpp \
        main.cpp \
        widget.cpp

HEADERS += \
        rangesetting.h \
        widget.h \
        dataoper.h \
        frmmessagebox.h \
        iconhelper.h \
        Snap7_sdk/plc_siemens.h \
        Snap7_sdk/s7.h \
        Snap7_sdk/snap7.h \
        Snap7_sdk/plcthread.h \
        loginform.h \
        savelog.h \
        limitform.h \
        titlewidget.h \
        qtstreambuf.h \
        connection.h \
        BaseGraphicsview.h \
        myhelper.h \
        objectinfo.h \
        ImageWidget.h
FORMS += \
        rangesetting.ui \
        widget.ui \
        frmmessagebox.ui \
        limitform.ui \
        loginform.ui



RC_ICONS=log.ico
#产品名称
QMAKE_TARGET_PRODUCT = PreAdjustmentPlatformSystem
#版权所有
QMAKE_TARGET_COPYRIGHT = CDDMWK
#文件说明
QMAKE_TARGET_DESCRIPTION = QQ:1098879222
RESOURCES += \
    images.qrc
TRANSLATIONS += \
    PreAdjustmentPlatformSystem_zh_CN.ts
include(Snap7_sdk.pri)
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
