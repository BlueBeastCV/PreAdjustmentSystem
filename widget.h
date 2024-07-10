#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <objectinfo.h>
#include <dataoper.h>
#include <myhelper.h>
#include "BaseGraphicsview.h"
#include "qtstreambuf.h"
#include "Snap7_sdk/plc_siemens.h"
#include "Snap7_sdk/s7.h"
#include "Snap7_sdk/plcthread.h"
#include <QCheckBox>
#include <rangesetting.h>
#include <limitform.h>
#include <QFileDialog>
#include <QtXlsx/xlsxdocument.h>
#include <QtXlsx/xlsxworksheet.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(User u,QWidget *parent = nullptr);
    ~Widget();
    void init();
    void initConnect();

    void initPLC();
    void readControllerObject();
    void initPushbuttonByFrame(QFrame* frame);
    void initPushbuttonByFrame_slot();
    void initPushbuttonByGroup(QGroupBox *box);
    void initPushbuttonByGroup_slot();
    void changeWidgetButtonStyle(QPushButton* checkButton);
    void showUserTable(QVector<User> users);
    bool isAdmin();
    bool areAllSpinboxesZero(QGroupBox *groupBox);
    void exportStdData();
    void exportOfficialData();
    void exportUltimteData();
    void showDetectDataOnTable();
    void saveDetectDataToTable();
    void showMeasureDataOnTable(const QString measureBatchNum, const QString startTime, const QString endTime);
    void writeWarningMessage(const QString &warningMessage);
    void InitPLCThread();
    void ConnectPLCStatus(PLC_Siemens* plc, int ok);
    void initEnable();
    void timingSendZeroPointToPLC();
    void initOriginalPointCorrectionValue();
private slots:
    void touchScreenPriority_Slot(int state);
    void zeroCorrectionValue_Slot(int state);
    bool saveZeroCorrectionValue_Slot();
signals:
    void ConnectPLC(QString ip, int Rack, int Slot);
protected slots:
    void closeEvent(QCloseEvent* event);
private:
    Ui::Widget *ui;
    QTimer systemTime;
    User currentUser;
    PLC_Siemens *snap7_plc = nullptr;
    int readDbNum = myHelper::readSettings("GPLC/readDB").toInt();
    int writeDbNum = myHelper::readSettings("GPLC/writeDB").toInt();
    QString PLCIP = myHelper::readSettings("GPLC/IP").toString();
    int PLCRack = myHelper::readSettings("GPLC/Rack").toInt();
    int PLCSlot = myHelper::readSettings("GPLC/Slot").toInt();
    int intPLCount = 0;
    QTimer initDeviceTimer;//程序启动定时初始化 硬件设备
    QTimer timeReadController;
    QTimer timingWriteController;
    DataOper oper;
    ControllerObject controllerObject; //实时PLC数据
    ControllerObject oneConObject; //只处理一次的PLC对象
    bool controllerTX = false;//心跳
    bool controllerInitStatus;
    bool loadMeasureProCodeList;
    bool isShowUserTable;
    bool isLocked = false; //参数设置锁定状态
    bool setEnabled = false; //使能状态

    bool gantryResetIsFinished = false;
    bool platformResetIsFinished = false;
    bool cyclingResetIsFinished = false;
    bool platformAndGantryResetAreFinished = false;
    //龙门、平台、旋转运动状态
    bool isGantryMeasuringStatus = false;
    bool isPlatformMeasuringStatus = false;
    bool isCyclingMeasuringStatus = false;

    rangesetting *setting;
    LimitForm *limitform;

    QVector<ResultSet> resultSet;
    //PLC线程对象
    PLCThread *plcThreadOBJ = nullptr;
    QThread plcThread;
};
#endif // WIDGET_H
