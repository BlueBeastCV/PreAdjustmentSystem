#include "widget.h"
#include "ui_widget.h"

Widget::Widget(User u,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    currentUser = u;
    myHelper::FormInCenter(this);//窗体居中显示
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    ui->titleWidget->set_lab_Title("高精度快换预调台系统");
    ui->titleWidget->setShowLabIco(false);
    ui->titleWidget->setShowMaxBtn(true);
    this->showNormal();
    setting = new rangesetting(currentUser);
    changeWidgetButtonStyle(ui->Btn_mainPage);
    ui->stackedWidget->setCurrentWidget(ui->mainPage);
    init();
    initDeviceTimer.singleShot(100, this, [this](){
        InitPLCThread();

    });

    QTimer::singleShot(1000,[this](){
        initEnable();
    });
    connect(&timingWriteController,&QTimer::timeout,[this](){
        timingSendZeroPointToPLC();
    });
    timingWriteController.start(500);
    initOriginalPointCorrectionValue();

}

Widget::~Widget()
{
    systemTime.stop();
    //销毁PLC初始化线程
    plcThread.quit();
    plcThread.wait();

    delete setting;
    delete ui;

}

void Widget::init()
{
    initConnect();

    QDate measureDates = QDate::currentDate();
    ui->startTime->setDate(measureDates.addDays(-1));
    ui->endTime->setDate(measureDates);
    initPushbuttonByFrame(ui->frame_4);
    initPushbuttonByFrame(ui->frame3);
    ui->roleName->setText(currentUser.role);
    ui->userName->setText(currentUser.name);
    ui->Edit_dateTime->setText(QDate::currentDate().toString("yyyy-MM-dd"));
}
//初始化PLC线程函数
void Widget::InitPLCThread()
{
    plcThreadOBJ = new PLCThread();
    plcThreadOBJ->moveToThread(&plcThread);
    plcThread.start();

    connect(&plcThread, &QThread::finished, plcThreadOBJ, &PLCThread::deleteLater);
    connect(this,         &Widget::ConnectPLC,  plcThreadOBJ,   &PLCThread::ConnectPLC);
    connect(plcThreadOBJ, &PLCThread::ConnectPLCStatus,   this,    &Widget::ConnectPLCStatus);
    emit ConnectPLC(PLCIP, PLCRack, PLCSlot);
}
//PLC线程初始化返回函数
void Widget::ConnectPLCStatus(PLC_Siemens* plc, int ok)
{
    snap7_plc = plc;
    QString error = snap7_plc->ErrorText(ok);
    qlog(QString("snap7_plc connect : %1").arg(error));
    if(ok == 0){
        intPLCount = 0;
        ui->controllerLabel->setStyleSheet(normal_radius);
        controllerInitStatus = true;
        connect(&timeReadController, &QTimer::timeout, [this](){
            readControllerObject();

        });
        timeReadController.start(100);
    }else{
        intPLCount = intPLCount + 1;
        if(intPLCount == 1){
            ui->controllerLabel->setStyleSheet(fail_radius);
            controllerInitStatus = false;
        }else{
            emit ConnectPLC(PLCIP, PLCRack, PLCSlot);
        }
    }
}

void Widget::initEnable()
{
    if(controllerInitStatus == false)
    {
        return;
    }
    int open = snap7_plc->sendBitToRobot(48, 5, true, writeDbNum, "初始化时使能信号");
    if (open == 0) {
        ui->enableStatus->setStyleSheet("background-color:green;color: rgb(255, 255, 255);");
        ui->enableStatus->setText("已使能");
        myHelper::pushButtonStyleChange("去使能",ui->setEnergy,true,true);
        setEnabled = true;
    }
}

void Widget::timingSendZeroPointToPLC()
{
    if(controllerInitStatus == false)
    {
        return;
    }
    float zeroValue = static_cast<float>(ui->zeroCorrectionValue->value());
    byte zeroValueBuffer[4];
    S7_SetRealAt(zeroValueBuffer,0,zeroValue);
    snap7_plc->DBWrite(writeDbNum,58,4,&zeroValueBuffer,"实时发送原点修正值");
}

void Widget::initOriginalPointCorrectionValue()
{
   double frontCorrectionValue =  myHelper::readSettings("correctionParameters/originalPointCorrectionValue").toDouble();
   ui->zeroCorrectionValue->setValue(frontCorrectionValue);
}
//初始化连接函数
void Widget::initConnect()
{
    connect(&systemTime, &QTimer::timeout, [this](){
        QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        ui->systemTime->setText(currentDateTime);
    });
    systemTime.start(1000);
    //修改参数和锁定参数设置
    connect(ui->Btn_modifyAndlockInfos,&QPushButton::clicked,this,[=](){
        if(isLocked)
        {
            // 解锁控件
            ui->Edit_batchNum->setEnabled(true);
            ui->Edit_typeNum->setEnabled(true);
            ui->Box_Type->setEnabled(true);
            ui->Edit_processNum->setEnabled(true);
            ui->Edit_componentNum->setEnabled(true);
            ui->Btn_modifyAndlockInfos->setText("锁定\n信息");
            // 更新按钮状态
            isLocked = false;
        }
        else
        {
            // 锁定控件
            ui->Edit_batchNum->setEnabled(false);
            ui->Edit_typeNum->setEnabled(false);
            ui->Box_Type->setEnabled(false);
            ui->Edit_processNum->setEnabled(false);
            ui->Edit_componentNum->setEnabled(false);
            ui->Btn_modifyAndlockInfos->setText("修改\n信息");
            // 更新按钮状态
            isLocked = true;
        }
    });

    //使能
    connect(ui->setEnergy,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(setEnabled == false)
        {
            int open = snap7_plc->sendBitToRobot(48,5,true,writeDbNum,"使能信号");
            if(0 == open)
            {
                myHelper::pushButtonStyleChange("去使能",ui->setEnergy,true,true);
                ui->enableStatus->setStyleSheet("background-color:green;color: rgb(255, 255, 255);");
                ui->enableStatus->setText("已使能");
                setEnabled = true;
            }
        }
        else
        {
            int close = snap7_plc->sendBitToRobot(48,5,false,writeDbNum,"去使能信号");
            if(0 == close)
            {
                myHelper::pushButtonStyleChange("使能",ui->setEnergy,true,true);
                ui->enableStatus->setStyleSheet("background-color:#F42215;color: rgb(255, 255, 255);");
                ui->enableStatus->setText("未使能");
                setEnabled = false;
            }
        }
    });
    //保存信息
    connect(ui->Btn_saveInfos,&QPushButton::clicked,this,[=](){
        if(ui->Edit_batchNum->text().isEmpty() || ui->Edit_typeNum->text().isEmpty()
                || ui->Edit_processNum->text().isEmpty() || ui->Edit_componentNum->text().isEmpty())
        {
            myHelper::ShowMessageBoxError("信息不完整，请填写完整信息！");
            return;
        }
        if(myHelper::ShowMessageBoxQuesion("是否要保存当前产品信息？") == QDialog::Accepted)
        {
            QVector<ProductionInfo> infos;
            ProductionInfo info;
            info.batchNum = ui->Edit_batchNum->text();
            info.typeNum = ui->Edit_typeNum->text();
            info.processNum = ui->Edit_processNum->text().toInt();
            info.componentNum = ui->Edit_componentNum->text();
            info.dates = ui->Edit_dateTime->text();
            infos.push_back(info);
            if(0 == oper.saveProductionInfo(infos))
            {
                myHelper::ShowMessageBoxInfo("保存信息成功！");
            }
            else
            {
                myHelper::ShowMessageBoxError("保存信息失败！");

            }
        }

    });
    //测量开始日期修改事件
    connect(ui->startTime,&QDateEdit::dateChanged,[this](const QDate &date){
        QString endTime = ui->endTime->date().toString("yyyy-MM-dd");
        QStringList typeList;
        oper.getBatchNumListByTime(typeList, date.toString("yyyy-MM-dd"), endTime);
        loadMeasureProCodeList = true;
        ui->TypeList->clear();
        ui->TypeList->addItem("选择批次号");
        ui->TypeList->addItems(typeList);
        loadMeasureProCodeList = false;
    });
    //保存当前原点修正值
    connect(ui->saveZeroCorrectionValue,&QPushButton::clicked,this,[=](){
        if(myHelper::ShowMessageBoxQuesion("是否要保存当前原点修正置？") == QDialog::Accepted)
        {
            if(saveZeroCorrectionValue_Slot() == true)
            {
                myHelper::ShowMessageBoxInfo("保存成功！");
            }
        }

    });
    //测量结束日期修改事件
    connect(ui->endTime,&QDateEdit::dateChanged,[this](const QDate &date){
        QString startTime = ui->startTime->date().toString("yyyy-MM-dd");
        QStringList typeList;
        oper.getBatchNumListByTime(typeList, startTime, date.toString("yyyy-MM-dd"));
        loadMeasureProCodeList = true;
        ui->TypeList->clear();
        ui->TypeList->addItem("选择批次号");
        ui->TypeList->addItems(typeList);
        loadMeasureProCodeList = false;
    });
    //数据查询
    connect(ui->queryBtn,&QPushButton::clicked,this,[=](){
        QString measureBatchNum = ui->TypeList->currentText();
        if(measureBatchNum== "选择批次号" || measureBatchNum.isNull())
        {
            myHelper::ShowMessageBoxError("请先选择批次号！");
            qlog("请先选择批次号！");
            return;
        }
        showMeasureDataOnTable(measureBatchNum,ui->startTime->text(),ui->endTime->text());

    });
    //标准数据导出到Excel
    connect(ui->exportStdData,&QPushButton::clicked,this,&Widget::exportStdData);
    //正式数据导出到Excel
    connect(ui->exportOfficialData,&QPushButton::clicked,this,&Widget::exportOfficialData);
    //最终数据导出到Excel
    connect(ui->exportData,&QPushButton::clicked,this,&Widget::exportUltimteData);
    //触控屏权限开关响应
    connect(ui->touchscreenPermission,&QCheckBox::stateChanged,this,&Widget::touchScreenPriority_Slot);
    //原点修正值checkedBox相应
    connect(ui->zeroCorrectionValueBox,&QCheckBox::stateChanged,this,&Widget::zeroCorrectionValue_Slot);
    //龙门运动开始
    connect(ui->gantryStart,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(ui->Edit_batchNum->text().isEmpty() || ui->Edit_typeNum->text().isEmpty()
                || ui->Edit_processNum->text().isEmpty() || ui->Edit_componentNum->text().isEmpty())
        {
            myHelper::ShowMessageBoxError("信息不完整，请填写完整信息！");
            return;
        }
        int gantryMoveOk = myHelper::ShowMessageBoxQuesion("是否要进行龙门运动？");
        if(gantryMoveOk == QDialog::Accepted)
        {
            float gantrySpeed = static_cast<float>(ui->gantrySpeed->value());
            byte gantrySpeedBuffer[4];
            S7_SetRealAt(gantrySpeedBuffer,0,gantrySpeed);
            snap7_plc->DBWrite(writeDbNum,2,4,&gantrySpeedBuffer,"龙门速度给定");

            float gantryShifting = static_cast<float>(ui->gantryLocation->value());
            byte gantryShiftingBuffer[4];
            S7_SetRealAt(gantryShiftingBuffer,0,gantryShifting);
            snap7_plc->DBWrite(writeDbNum,44,4,&gantryShiftingBuffer,"龙门到达测量位置设定");

            snap7_plc->sendBitToRobot(48,0,true,writeDbNum,"龙门测量开始信号");
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,false);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            isGantryMeasuringStatus = true;
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(48,0,false,writeDbNum,"龙门测量开始信号");
            });
        }

    });
    //龙门运动停止
    connect(ui->gantryStop,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        snap7_plc->sendBitToRobot(0,0,true,writeDbNum,"龙门运动停止信号");
        myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
        myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
        myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
        myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
        myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
        myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
        myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
        myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
        myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
        myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
        myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
        myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
        myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);

        isGantryMeasuringStatus = false;
        QTimer::singleShot(500,[this](){
            snap7_plc->sendBitToRobot(0,0,false,writeDbNum,"龙门运动停止信号");
        });
    });
    //龙门复位
    connect(ui->gantryReset,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        int gantryResetOk = myHelper::ShowMessageBoxQuesion("是否要对龙门进行复位？");
        if(gantryResetOk == QDialog::Accepted)
        {
            float gantrySpeed = static_cast<float>(ui->gantrySpeed->value());
            byte gantrySpeedBuffer[4];
            S7_SetRealAt(gantrySpeedBuffer,0,gantrySpeed);
            snap7_plc->DBWrite(writeDbNum,2,4,&gantrySpeedBuffer,"龙门复位速度给定");
            float gantrySafeLocation = ui->gantrySafeLocation->value();
            byte gantrySafeLocationBuffer[4];
            S7_SetRealAt(gantrySafeLocationBuffer,0,gantrySafeLocation);
            snap7_plc->DBWrite(writeDbNum,36,4,&gantrySafeLocationBuffer,"龙门安全位置设定");
            snap7_plc->sendBitToRobot(48,3,true,writeDbNum,"龙门运动复位信号");
            gantryResetIsFinished = true;
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,false);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            isGantryMeasuringStatus = true;
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(48,3,false,writeDbNum,"龙门运动复位信号清空");
            });
            ui->groupBox_3->setFocus();
        }
    });
    //龙门回零
    connect(ui->gantryBackToZero,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(myHelper::ShowMessageBoxQuesion("是否要进行龙门回零操作？") == QDialog::Accepted)
        {
            snap7_plc->sendBitToRobot(0,2,true,writeDbNum,"龙门回零信号触发！");
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,false);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(0,2,false,writeDbNum,"龙门回零信号清空！");
            });
            ui->groupBox_3->setFocus();
        }

    });
    //平台运动开始
    connect(ui->platformStart,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(ui->Edit_batchNum->text().isEmpty() || ui->Edit_typeNum->text().isEmpty()
                || ui->Edit_processNum->text().isEmpty() || ui->Edit_componentNum->text().isEmpty())
        {
            myHelper::ShowMessageBoxError("信息不完整，请填写完整信息！");
            return;
        }
        int platformMoveOk = myHelper::ShowMessageBoxQuesion("是否要进行平台运动？");
        if(platformMoveOk == QDialog::Accepted)
        {
            float platformSpeed = static_cast<float>(ui->platformSpeed->value());
            byte platformSpeedBuffer[4];
            S7_SetRealAt(platformSpeedBuffer,0,platformSpeed);
            snap7_plc->DBWrite(writeDbNum,12,4,&platformSpeedBuffer,"平台速度给定");
            float platformShifting = static_cast<float>(ui->platformLocation->value());
            byte platformShiftingBuffer[4];
            S7_SetRealAt(platformShiftingBuffer,0,platformShifting);
            snap7_plc->DBWrite(writeDbNum,40,4,&platformShiftingBuffer,"平台到达测量位置设定");
            snap7_plc->sendBitToRobot(48,1,true,writeDbNum,"平台测量开始信号");
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,false);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            isPlatformMeasuringStatus = true;
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(48,1,false,writeDbNum,"平台测量开始信号清空");
            });
        }

    });
    //平台运动停止
    connect(ui->platformStop,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        snap7_plc->sendBitToRobot(10,0,true,writeDbNum,"平台运动停止信号开始");
        myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
        myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
        myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
        myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
        myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
        myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
        myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
        myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
        myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
        myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
        myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
        myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
        myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
        isPlatformMeasuringStatus = false;
        QTimer::singleShot(500,[this](){
            snap7_plc->sendBitToRobot(10,0,false,writeDbNum,"平台运动停止信号清空");
        });

    });
    //平台复位
    connect(ui->platformReset,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        int platforResetOk = myHelper::ShowMessageBoxQuesion("是否要对平台进行复位？");
        if(platforResetOk == QDialog::Accepted)
        {
            float platformSpeed = static_cast<float>(ui->platformSpeed->value());
            byte platformSpeedBuffer[4];
            S7_SetRealAt(platformSpeedBuffer,0,platformSpeed);
            snap7_plc->DBWrite(writeDbNum,12,4,&platformSpeedBuffer,"平台复位速度给定");
            float platformSafeLocation = ui->platformSafeLocation->value();
            byte platformSafeLocationBuffer[4];
            S7_SetRealAt(platformSafeLocationBuffer,0,platformSafeLocation);
            snap7_plc->DBWrite(writeDbNum,32,4,&platformSafeLocationBuffer,"平台复位位置设定");
            snap7_plc->sendBitToRobot(48,2,true,writeDbNum,"平台运动复位信号");
            platformResetIsFinished = true;
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,false);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            isPlatformMeasuringStatus = true;
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(48,2,false,writeDbNum,"平台运动复位信号清空");
            });
            ui->groupBox_4->setFocus();
        }

    });
    //旋转运动开始
    connect(ui->cyclingStart,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(ui->Edit_batchNum->text().isEmpty() || ui->Edit_typeNum->text().isEmpty()
                || ui->Edit_processNum->text().isEmpty() || ui->Edit_componentNum->text().isEmpty())
        {
            myHelper::ShowMessageBoxError("信息不完整，请填写完整信息！");

            return;
        }
        int cyclingMoveOk = myHelper::ShowMessageBoxQuesion("是否要进行旋转运动？");
        if(cyclingMoveOk == QDialog::Accepted)
        {
            float cyclingSpeed = static_cast<float>(ui->cyclingSpeed->value());
            byte cyclingSpeedBuffer[4];
            S7_SetRealAt(cyclingSpeedBuffer,0,cyclingSpeed);
            snap7_plc->DBWrite(writeDbNum,22,4,&cyclingSpeedBuffer,"旋转速度写入！");
            float cyclingAngle = static_cast<float>(ui->cyclingAngle->value());
            byte cyclingAngleBuffer[4];
            S7_SetRealAt(cyclingAngleBuffer,0,cyclingAngle);
            snap7_plc->DBWrite(writeDbNum,26,4,&cyclingAngleBuffer,"旋转角度写入！");
            snap7_plc->sendBitToRobot(20,3,true,writeDbNum,"旋转绝对定位启动!");
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,false);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            isCyclingMeasuringStatus = true;
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(20,3,false,writeDbNum,"旋转绝对定位启动信号清除!");
            });
        }
    });
    //旋转运动停止
    connect(ui->cyclingStop,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");

            return;
        }
        snap7_plc->sendBitToRobot(20,0,true,writeDbNum,"旋转停止信号");
        myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
        myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
        myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
        myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
        myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
        myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
        myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
        myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
        myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
        myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
        myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
        myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
        myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
        QTimer::singleShot(500,[this](){
            snap7_plc->sendBitToRobot(20,0,false,writeDbNum,"旋转停止信号清除");
        });
        isCyclingMeasuringStatus = false;

    });
    //旋转回零
    connect(ui->cyclingBackToZero,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(myHelper::ShowMessageBoxQuesion("是否要进行旋转回零操作？") == QDialog::Accepted)
        {
            snap7_plc->sendBitToRobot(20,2,true,writeDbNum,"旋转回零信号触发！");
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,false);
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(20,2,false,writeDbNum,"旋转回零信号清空！");
                myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            });

            ui->groupBox_5->setFocus();
        }
    });
    //旋转复位
    connect(ui->cyclingReset,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        if(myHelper::ShowMessageBoxQuesion("是否要进行旋转复位操作？") == QDialog::Accepted)
        {
            float cyclingSpeed = static_cast<float>(ui->cyclingSpeed->value());
            byte cyclingSpeedBuffer[4];
            S7_SetRealAt(cyclingSpeedBuffer,0,cyclingSpeed);
            snap7_plc->DBWrite(writeDbNum,22,4,&cyclingSpeedBuffer,"旋转复位速度写入！");
            snap7_plc->sendBitToRobot(20,1,true,writeDbNum,"旋转复位信号触发！");
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,false);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,false,true);
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(20,1,false,writeDbNum,"旋转复位信号清空！");

            });
            cyclingResetIsFinished = true;
            ui->groupBox_5->setFocus();
        }
    });
    //一键复位
    connect(ui->Reset,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        int onePressResetOk = myHelper::ShowMessageBoxQuesion("龙门、平台是否进行一键复位？");
        if(onePressResetOk == QDialog::Accepted)
        {
            float platformSpeed = static_cast<float>(ui->platformSpeed->value());
            byte platformSpeedBuffer[4];
            S7_SetRealAt(platformSpeedBuffer,0,platformSpeed);
            snap7_plc->DBWrite(writeDbNum,12,4,&platformSpeedBuffer,"平台一键复位速度给定");
            float gantrySpeed = static_cast<float>(ui->gantrySpeed->value());
            byte gantrySpeedBuffer[4];
            S7_SetRealAt(gantrySpeedBuffer,0,gantrySpeed);
            snap7_plc->DBWrite(writeDbNum,2,4,&gantrySpeedBuffer,"龙门一键复位速度给定");
            //发送龙门复位位置
            float gantryResetLocation = static_cast<float>(ui->gantrySafeLocation->value());
            byte gantryResetLocationBuffer[4];
            S7_SetRealAt(gantryResetLocationBuffer,0,gantryResetLocation);
            snap7_plc->DBWrite(writeDbNum,36,4,&gantryResetLocationBuffer,"发送龙门复位位置");
            float paltformResetLocation = static_cast<float>(ui->platformSafeLocation->value());
            byte paltformResetLocationBuffer[4];
            S7_SetRealAt(paltformResetLocationBuffer,0,paltformResetLocation);
            snap7_plc->DBWrite(writeDbNum,32,4,&paltformResetLocationBuffer,"发送平台复位位置");
            snap7_plc->sendBitToRobot(30,2,true,writeDbNum,"发送龙门和平台一起复位信号");

            myHelper::pushButtonStyleChange("复位",ui->Reset,false,false);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,false,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,false,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,false,true);

            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,false,true);

            myHelper::pushButtonStyleChange("开始",ui->gantryStart,false,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,false,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,false,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,false,true);
            platformAndGantryResetAreFinished = true;
            QTimer::singleShot(500,[this](){
                snap7_plc->sendBitToRobot(30,2,false,writeDbNum,"发送龙门和平台一起复位信号清空");
            });
            ui->groupBox_9->setFocus();
        }

    });
    //报警复位
    connect(ui->warningReset,&QPushButton::clicked,this,[=](){
        if(controllerInitStatus == false)
        {
            myHelper::ShowMessageBoxError("控制系统未连接");
            return;
        }
        snap7_plc->sendBitToRobot(48,4,true,writeDbNum,"报警复位信号");
        myHelper::pushButtonStyleChange("报警复位",ui->warningReset,false,false);
        QTimer::singleShot(500,[this](){
            snap7_plc->sendBitToRobot(48,4,false,writeDbNum,"报警复位信号清空");
        });
        QTimer::singleShot(1500,[this](){
            myHelper::pushButtonStyleChange("报警复位",ui->warningReset,true,true);
        });
    });

}
//这种方式已经被淘汰，采用线程连接
void Widget::initPLC()
{
    snap7_plc = new PLC_Siemens(this);
    snap7_plc->setConnectType(CONNTYPE_OP);//设置连接方式，避免变成软件连不上PLC了，必须在连接之前设置
    int ok = snap7_plc->connectByIP(myHelper::readSettings("GPLC/IP").toString(),
                                    myHelper::readSettings("GPLC/Rack").toInt(),
                                    myHelper::readSettings("GPLC/Slot").toInt());
    readDbNum = myHelper::readSettings("GPLC/readDB").toInt();
    writeDbNum = myHelper::readSettings("GPLC/writeDB").toInt();

    if(0 == ok){
        ui->controllerLabel->setStyleSheet(normal_radius);
        controllerInitStatus = true;

        connect(&timeReadController, &QTimer::timeout, [this](){
            readControllerObject();
        });
        timeReadController.start(10);
    }else{
        QString error = snap7_plc->ErrorText(ok);
        QString str = "snap7_plc connect :" + error;
        qlog(str);
        ui->controllerLabel->setStyleSheet(fail_radius);
        controllerInitStatus = false;

    }

}
//定时读取PLC数据
void Widget::readControllerObject()
{
    byte readByte[62];
    snap7_plc->DBRead(readDbNum, 0, 62, readByte, "从PLC的0偏移量地址读取64字节数据");// DBRead(int DBNumber, int Start, int Size 字节数, void *pUsrData)
    //处理心跳
    controllerTX = S7_GetBitAt(readByte, 54, 7);//控制一直发送true，
    if(controllerTX == false){
        //控制掉线了,停止定时器。
        ui->controllerLabel->setStyleSheet(fail_radius);
        controllerInitStatus = false;
        timeReadController.stop();
        //qlog("PLC掉线了。。。。");
        writeWarningMessage("PLC掉线了。。。。");
        myHelper::ShowMessageBoxError("PLC掉线了。。。。");
    }
    controllerTX = false;
    if(ui->Box_Type->currentText() == "标准件")
    {
        controllerObject.stdValueOne = S7_GetRealAt(readByte, 0);
        if(S7_GetRealAt(readByte, 0) != oneConObject.stdValueOne)
        {
            oneConObject.stdValueOne = S7_GetRealAt(readByte, 0);
//            ui->sensorOneStdData->setValue(oneConObject.stdValueOne);
        }
        controllerObject.stdValueTwo = S7_GetRealAt(readByte, 4);
        if(S7_GetRealAt(readByte, 4) != oneConObject.stdValueTwo)
        {
            oneConObject.stdValueTwo = S7_GetRealAt(readByte, 4);
//            ui->sensorTwoStdData->setValue(oneConObject.stdValueTwo);
        }
        controllerObject.stdValueThree = S7_GetRealAt(readByte, 8);
        if(S7_GetRealAt(readByte, 8) != oneConObject.stdValueThree)
        {
            oneConObject.stdValueThree = S7_GetRealAt(readByte, 8);
//            ui->sensorThreeStdData->setValue(oneConObject.stdValueThree);
        }
        controllerObject.stdValueFour = S7_GetRealAt(readByte, 12);
        if(S7_GetRealAt(readByte, 12) != oneConObject.stdValueFour)
        {
            oneConObject.stdValueFour = S7_GetRealAt(readByte, 12);
//            ui->sensorFourStdData->setValue(oneConObject.stdValueFour);
        }
        controllerObject.stdValueFive = S7_GetRealAt(readByte, 16);
        if(S7_GetRealAt(readByte, 16) != oneConObject.stdValueFive)
        {
            oneConObject.stdValueFive = S7_GetRealAt(readByte, 16);
//            ui->sensorFiveStdData->setValue(oneConObject.stdValueFive);
        }
        controllerObject.stdValueSix = S7_GetRealAt(readByte, 20);
        if(S7_GetRealAt(readByte, 20) != oneConObject.stdValueSix)
        {
            oneConObject.stdValueSix = S7_GetRealAt(readByte, 20);
//            ui->sensorSixStdData->setValue(oneConObject.stdValueSix);
        }
        controllerObject.stdValueSeven = S7_GetRealAt(readByte, 24);
        if(S7_GetRealAt(readByte, 24) != oneConObject.stdValueSeven)
        {
            oneConObject.stdValueSeven = S7_GetRealAt(readByte, 24);
//            ui->sensorSevenStdData->setValue(oneConObject.stdValueSeven);
        }

    }
    if(ui->Box_Type->currentText() == "正式件")
    {
        controllerObject.officialValueOne = S7_GetRealAt(readByte, 0);
        if(S7_GetRealAt(readByte, 0) != oneConObject.officialValueOne)
        {
            oneConObject.officialValueOne = S7_GetRealAt(readByte, 0);
            //                ui->sensorOneData->setValue(oneConObject.officialValueOne);
        }
        controllerObject.officialValueTwo = S7_GetRealAt(readByte, 4);
        if(S7_GetRealAt(readByte, 4) != oneConObject.officialValueOne)
        {
            oneConObject.officialValueTwo = S7_GetRealAt(readByte, 4);
            //                ui->sensorTwoData->setValue(oneConObject.officialValueTwo);
        }
        controllerObject.officialValueThree = S7_GetRealAt(readByte, 8);
        if(S7_GetRealAt(readByte, 8) != oneConObject.officialValueThree)
        {
            oneConObject.officialValueThree = S7_GetRealAt(readByte, 8);
            //                ui->sensorThreeData->setValue(oneConObject.officialValueThree);
        }
        controllerObject.officialValueFour = S7_GetRealAt(readByte, 12);
        if(S7_GetRealAt(readByte, 12) != oneConObject.officialValueFour)
        {
            oneConObject.officialValueFour = S7_GetRealAt(readByte, 12);
            //                ui->sensorFourData->setValue(oneConObject.officialValueFour);
        }
        controllerObject.officialValueFive = S7_GetRealAt(readByte, 16);
        if(S7_GetRealAt(readByte, 16) != oneConObject.officialValueFive)
        {
            oneConObject.officialValueFive = S7_GetRealAt(readByte, 16);
            //                ui->sensorFiveData->setValue(oneConObject.officialValueFive);
        }
        controllerObject.officialValueSix = S7_GetRealAt(readByte, 20);
        if(S7_GetRealAt(readByte, 20) != oneConObject.officialValueSix)
        {
            oneConObject.officialValueSix = S7_GetRealAt(readByte, 20);
            //                ui->sensorSixData->setValue(oneConObject.officialValueSix);
        }
        controllerObject.officialValueSeven = S7_GetRealAt(readByte, 24);
        if(S7_GetRealAt(readByte, 24) != oneConObject.officialValueSeven)
        {
            oneConObject.officialValueSeven = S7_GetRealAt(readByte, 24);
            //                ui->sensorSevenData->setValue(oneConObject.officialValueSeven);
        }
    }
    //龙门实时位置
    controllerObject.gantryRealLocation = S7_GetRealAt(readByte,32);
    if(S7_GetByteAt(readByte,32) != oneConObject.gantryRealLocation)
    {
        oneConObject.gantryRealLocation = S7_GetRealAt(readByte,32);
        ui->gantryRealTimeLocation->setValue(oneConObject.gantryRealLocation);
    }
    //平台实时位置
    controllerObject.platformRealLocation = S7_GetRealAt(readByte,40);
    if(S7_GetByteAt(readByte,40) != oneConObject.platformRealLocation)
    {
        oneConObject.platformRealLocation = S7_GetRealAt(readByte,40);
        ui->platformRealTimeLocation->setValue(oneConObject.platformRealLocation);
    }
    //旋转实时位置
    controllerObject.cycilingRealLocation = S7_GetRealAt(readByte,56);
    if(S7_GetByteAt(readByte,56) != oneConObject.cycilingRealLocation)
    {
        oneConObject.cycilingRealLocation = S7_GetRealAt(readByte,56);
//        float realAngel = fmod(oneConObject.cycilingRealLocation,360.0f);
//        byte realAngelBuffer[4];
//        S7_SetRealAt(realAngelBuffer,0,realAngel);
//        snap7_plc->DBWrite(writeDbNum,50,4,&realAngelBuffer,"真实旋转角度发送");
        ui->cyclingRealTimeAngle->setValue(oneConObject.cycilingRealLocation);
        // 50
        //        ui->cyclingRealTimeAngle->setValue(oneConObject.cycilingRealLocation);

    }
    //龙门绝对定位完成
    controllerObject.gantryAbsLocationFinish = S7_GetBitAt(readByte,28,2);
    if(S7_GetBitAt(readByte,28,2) != oneConObject.gantryAbsLocationFinish)
    {
        oneConObject.gantryAbsLocationFinish = S7_GetBitAt(readByte,28,2);
        if(oneConObject.gantryAbsLocationFinish == true && gantryResetIsFinished == false)
        {
            myHelper::ShowMessageBoxInfo("龙门运动已经到位！",true,1000);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            isGantryMeasuringStatus = false;
            if(ui->Box_Type->currentText() == "标准件" && platformAndGantryResetAreFinished == false)
            {
                ui->sensorThreeStdData->setValue(oneConObject.stdValueThree);
                ui->sensorFourStdData->setValue(oneConObject.stdValueFour);
                ui->sensorFiveStdData->setValue(oneConObject.stdValueFive);
                ui->sensorSixStdData->setValue(oneConObject.stdValueSix);
                ui->sensorSevenStdData->setValue(oneConObject.stdValueSeven);
            }
            if(ui->Box_Type->currentText() == "正式件" && platformAndGantryResetAreFinished == false)
            {
                ui->sensorThreeData->setValue(oneConObject.officialValueThree);
                ui->sensorFourData->setValue(oneConObject.officialValueFour);
                ui->sensorFiveData->setValue(oneConObject.officialValueFive);
                ui->sensorSixData->setValue(oneConObject.officialValueSix);
                ui->sensorSevenData->setValue(oneConObject.officialValueSeven);

                QTimer::singleShot(2500,[this](){
                    showDetectDataOnTable();
                    saveDetectDataToTable();
                });


            }

        }
        if(oneConObject.gantryAbsLocationFinish == true && gantryResetIsFinished == true)
        {
            myHelper::ShowMessageBoxInfo("龙门运动复位完成！");
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            isGantryMeasuringStatus = false;
            gantryResetIsFinished = false;
        }
    }
    //平台绝对定位完成
    controllerObject.platformAbsLocationFinish = S7_GetBitAt(readByte,29,2);
    if(S7_GetBitAt(readByte,29,2) != oneConObject.platformAbsLocationFinish)
    {
        oneConObject.platformAbsLocationFinish = S7_GetBitAt(readByte,29,2);
        if(oneConObject.platformAbsLocationFinish == true && platformResetIsFinished == false)
        {
            myHelper::ShowMessageBoxInfo("平台运动已经到位！",true,1000);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            isPlatformMeasuringStatus = false;
            if(ui->Box_Type->currentText() == "标准件" && platformAndGantryResetAreFinished == false)
            {
                ui->sensorOneStdData->setValue(oneConObject.stdValueOne);
                ui->sensorTwoStdData->setValue(oneConObject.stdValueTwo);
            }
            if(ui->Box_Type->currentText() == "正式件" && platformAndGantryResetAreFinished == false)
            {
                ui->sensorOneData->setValue(oneConObject.officialValueOne);
                ui->sensorTwoData->setValue(oneConObject.officialValueTwo);
            }

        }
        if(oneConObject.platformAbsLocationFinish == true && platformResetIsFinished == true)
        {
            myHelper::ShowMessageBoxInfo("平台运动复位完成！");
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            isPlatformMeasuringStatus = false;
            platformResetIsFinished = false;
        }
    }
    //龙门、平台一起复位到位信号
    controllerObject.gantryAndPlatformAreReset = S7_GetBitAt(readByte,55,6);
    if(S7_GetBitAt(readByte,55,6) != oneConObject.gantryAndPlatformAreReset)
    {
        oneConObject.gantryAndPlatformAreReset = S7_GetBitAt(readByte,55,6);
        if(oneConObject.gantryAndPlatformAreReset == true && platformAndGantryResetAreFinished == true)
        {
            myHelper::ShowMessageBoxInfo("龙门和平台一键复位到位");
            writeWarningMessage("龙门和平台一键复位到位");
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            platformAndGantryResetAreFinished = false;

        }

    }
    //旋转绝对定位完成
    controllerObject.cycilingAbsLocationFinish = S7_GetBitAt(readByte,30,1);
    if(S7_GetBitAt(readByte,30,1) != oneConObject.cycilingAbsLocationFinish)
    {
        oneConObject.cycilingAbsLocationFinish = S7_GetBitAt(readByte,30,1);
        if(oneConObject.cycilingAbsLocationFinish == true && cyclingResetIsFinished == false)
        {
            myHelper::ShowMessageBoxInfo("旋转运动已经到位！",true,1000);
            writeWarningMessage("旋转运动已经到位！");
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            isCyclingMeasuringStatus = false;
        }
        if(oneConObject.cycilingAbsLocationFinish == true && cyclingResetIsFinished == true)
        {
            myHelper::ShowMessageBoxInfo("旋转复位已经完成！",true,1000);
            writeWarningMessage("旋转复位已经完成！");
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
            cyclingResetIsFinished = false;
        }
    }
    //龙门回零完成
    controllerObject.gantryBackToZeroFinished = S7_GetBitAt(readByte,28,0);
    if(S7_GetBitAt(readByte,28,0) != oneConObject.gantryBackToZeroFinished)
    {
        oneConObject.gantryBackToZeroFinished = S7_GetBitAt(readByte,28,0);
        if(oneConObject.gantryBackToZeroFinished == true)
        {
            myHelper::ShowMessageBoxInfo("龙门回零完成！");
            myHelper::pushButtonStyleChange("复位",ui->gantryReset,true,true);
            myHelper::pushButtonStyleChange("开始",ui->gantryStart,true,true);
            myHelper::pushButtonStyleChange("回零",ui->gantryBackToZero,true,true);
            myHelper::pushButtonStyleChange("停止",ui->gantryStop,true,true);
            myHelper::pushButtonStyleChange("开始",ui->cyclingStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->cyclingStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->cyclingReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->cyclingBackToZero,true,true);
            myHelper::pushButtonStyleChange("开始",ui->platformStart,true,true);
            myHelper::pushButtonStyleChange("停止",ui->platformStop,true,true);
            myHelper::pushButtonStyleChange("复位",ui->platformReset,true,true);
            myHelper::pushButtonStyleChange("回零",ui->platformBackToZero,true,true);
            myHelper::pushButtonStyleChange("复位",ui->Reset,true,true);
        }

    }

    //龙门回原点异常报警
    controllerObject.gantryBackToZeroWarning = S7_GetBitAt(readByte,28,3);
    if(S7_GetBitAt(readByte,28,3) != oneConObject.gantryBackToZeroWarning)
    {
        oneConObject.gantryBackToZeroWarning = S7_GetBitAt(readByte,28,3);
        if(oneConObject.gantryBackToZeroWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门回原点异常");
            writeWarningMessage("龙门回原点异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门相对定位异常报警
    controllerObject.gantryRelativeLocationWarning = S7_GetBitAt(readByte,28,4);
    if(S7_GetBitAt(readByte,28,4) != oneConObject.gantryRelativeLocationWarning)
    {
        oneConObject.gantryRelativeLocationWarning = S7_GetBitAt(readByte,28,4);
        if(oneConObject.gantryRelativeLocationWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门相对定位异常");
            writeWarningMessage("龙门相对定位异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门绝对定位异常报警
    controllerObject.gantryAbsLocationWarning = S7_GetBitAt(readByte,28,5);
    if(S7_GetBitAt(readByte,28,5) != oneConObject.gantryAbsLocationWarning)
    {
        oneConObject.gantryAbsLocationWarning = S7_GetBitAt(readByte,28,5);
        if(oneConObject.gantryAbsLocationWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门绝对定位异常");
            writeWarningMessage("龙门绝对定位异常");

        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门使能异常报警
    controllerObject.gantryEnableWarning = S7_GetBitAt(readByte,28,6);
    if(S7_GetBitAt(readByte,28,6) != oneConObject.gantryEnableWarning)
    {
        oneConObject.gantryEnableWarning = S7_GetBitAt(readByte,28,6);
        if(oneConObject.gantryEnableWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门使能异常");
            writeWarningMessage("龙门使能异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门复位异常报警
    controllerObject.gantryResetWarning = S7_GetBitAt(readByte,28,7);
    if(S7_GetBitAt(readByte,28,7) != oneConObject.gantryResetWarning)
    {
        oneConObject.gantryResetWarning = S7_GetBitAt(readByte,28,7);
        if(oneConObject.gantryResetWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门复位异常");
            writeWarningMessage("龙门复位异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门传感器数据异常
    controllerObject.gantrySensorValueWarning = S7_GetBitAt(readByte,60,0);
    if(S7_GetBitAt(readByte,60,0) != oneConObject.gantrySensorValueWarning)
    {
        oneConObject.gantrySensorValueWarning = S7_GetBitAt(readByte,60,0);
        if(oneConObject.gantrySensorValueWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门传感器数据异常");
            writeWarningMessage("龙门传感器数据异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //平台回原点异常报警
    controllerObject.platformBackToZeroWarning = S7_GetBitAt(readByte,29,3);
    if(S7_GetBitAt(readByte,29,3) != oneConObject.platformBackToZeroWarning)
    {
        oneConObject.platformBackToZeroWarning = S7_GetBitAt(readByte,29,3);
        if(oneConObject.platformBackToZeroWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台回原点异常");
            writeWarningMessage("平台回原点异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台相对定位异常报警
    controllerObject.platformRelativeLocationWarning = S7_GetBitAt(readByte,29,4);
    if(S7_GetBitAt(readByte,29,4) != oneConObject.platformRelativeLocationWarning)
    {
        oneConObject.platformRelativeLocationWarning = S7_GetBitAt(readByte,29,4);
        if(oneConObject.platformRelativeLocationWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台相对定位异常");
            writeWarningMessage("平台相对定位异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台绝对定位异常报警
    controllerObject.platformAbsLocationWarning = S7_GetBitAt(readByte,29,5);
    if(S7_GetBitAt(readByte,29,5) != oneConObject.platformAbsLocationWarning)
    {
        oneConObject.platformAbsLocationWarning = S7_GetBitAt(readByte,29,5);
        if(oneConObject.platformAbsLocationWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台绝对定位异常");
            writeWarningMessage("平台绝对定位异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台使能异常报警
    controllerObject.platformEnableWarning = S7_GetBitAt(readByte,29,6);
    if(S7_GetBitAt(readByte,29,6) != oneConObject.platformEnableWarning)
    {
        oneConObject.platformEnableWarning = S7_GetBitAt(readByte,29,6);
        if(oneConObject.platformEnableWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台使能异常");
            writeWarningMessage("平台使能异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台复位异常报警
    controllerObject.platformResetWarning = S7_GetBitAt(readByte,29,7);
    if(S7_GetBitAt(readByte,29,7) != oneConObject.platformResetWarning)
    {
        oneConObject.platformResetWarning = S7_GetBitAt(readByte,29,7);
        if(oneConObject.platformResetWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台复位异常");
            writeWarningMessage("平台复位异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台传感器数据异常
    controllerObject.platformSensorValueWarning = S7_GetBitAt(readByte,60,1);
    if(S7_GetBitAt(readByte,60,1) != oneConObject.platformSensorValueWarning)
    {
        oneConObject.platformSensorValueWarning = S7_GetBitAt(readByte,60,1);
        if(oneConObject.platformSensorValueWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台传感器数据异常");
            writeWarningMessage("平台传感器数据异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //旋转回原点异常报警
    controllerObject.cycilingBackToZeroWarning = S7_GetBitAt(readByte,30,3);
    if(S7_GetBitAt(readByte,30,3) != oneConObject.cycilingBackToZeroWarning)
    {
        oneConObject.cycilingBackToZeroWarning = S7_GetBitAt(readByte,30,3);
        if(oneConObject.cycilingBackToZeroWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转回原点异常");
            writeWarningMessage("旋转回原点异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转相对定位异常报警
    controllerObject.cycilingRelativeLocationWarning = S7_GetBitAt(readByte,30,4);
    if(S7_GetBitAt(readByte,30,4) != oneConObject.cycilingRelativeLocationWarning)
    {
        oneConObject.cycilingRelativeLocationWarning = S7_GetBitAt(readByte,30,4);
        if(oneConObject.cycilingRelativeLocationWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转相对定位异常报警");
            writeWarningMessage("旋转相对定位异常报警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转相对定位异常告警
    controllerObject.cycilingAbsLocationWarning = S7_GetBitAt(readByte,30,5);
    if(S7_GetBitAt(readByte,30,5) != oneConObject.cycilingAbsLocationWarning)
    {
        oneConObject.cycilingAbsLocationWarning = S7_GetBitAt(readByte,30,5);
        if(oneConObject.cycilingAbsLocationWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转相对定位异常告警");
            writeWarningMessage("旋转相对定位异常告警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转使能异常报警
    controllerObject.cycilingEnableWarning = S7_GetBitAt(readByte,30,6);
    if(S7_GetBitAt(readByte,30,6) != oneConObject.cycilingEnableWarning)
    {
        oneConObject.cycilingEnableWarning = S7_GetBitAt(readByte,30,6);
        if(oneConObject.cycilingEnableWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转使能异常");
            writeWarningMessage("旋转使能异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转复位异常报警
    controllerObject.cycilingResetWarning = S7_GetBitAt(readByte,30,7);
    if(S7_GetBitAt(readByte,30,7) != oneConObject.cycilingResetWarning)
    {
        oneConObject.cycilingResetWarning = S7_GetBitAt(readByte,30,7);
        if(oneConObject.cycilingResetWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转复位异常");
            writeWarningMessage("旋转复位异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转控制工艺轴报警
    controllerObject.cyclingProcessAxisWarning = S7_GetBitAt(readByte,60,2);
    if(S7_GetBitAt(readByte,60,2) != oneConObject.cyclingProcessAxisWarning)
    {
        oneConObject.cyclingProcessAxisWarning = S7_GetBitAt(readByte,60,2);
        if(oneConObject.cyclingProcessAxisWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转控制工艺轴报警");
            writeWarningMessage("旋转控制工艺轴报警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //控制台急停报警
    controllerObject.controlPlatformWarning = S7_GetBitAt(readByte,31,0);
    if(S7_GetBitAt(readByte,31,0) != oneConObject.controlPlatformWarning)
    {
        oneConObject.controlPlatformWarning = S7_GetBitAt(readByte,31,0);
        if(oneConObject.controlPlatformWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("控制台急停");
            writeWarningMessage("控制台急停");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //控制盒急停报警
    controllerObject.controlBoxWarning = S7_GetBitAt(readByte,31,1);
    if(S7_GetBitAt(readByte,31,1) != oneConObject.controlBoxWarning)
    {
        oneConObject.controlBoxWarning = S7_GetBitAt(readByte,31,1);
        if(oneConObject.controlBoxWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("控制盒急停");
            writeWarningMessage("控制盒急停");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //油冷机报警
    controllerObject.oilCoolerWarning = S7_GetBitAt(readByte,44,0);
    if(S7_GetBitAt(readByte,44,0) != oneConObject.oilCoolerWarning)
    {
        oneConObject.oilCoolerWarning = S7_GetBitAt(readByte,44,0);
        if(oneConObject.oilCoolerWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("控制盒急停");
            writeWarningMessage("控制盒急停");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门点动异常报警
    controllerObject.gantryJoggingWarning = S7_GetBitAt(readByte,44,1);
    if(S7_GetBitAt(readByte,44,1) != oneConObject.gantryJoggingWarning)
    {
        oneConObject.gantryJoggingWarning = S7_GetBitAt(readByte,44,1);
        if(oneConObject.gantryJoggingWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门点动异常");
            writeWarningMessage("龙门点动异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台点动异常报警
    controllerObject.platformJoggingWarning = S7_GetBitAt(readByte,44,2);
    if(S7_GetBitAt(readByte,44,2) != oneConObject.platformJoggingWarning)
    {
        oneConObject.platformJoggingWarning = S7_GetBitAt(readByte,44,2);
        if(oneConObject.platformJoggingWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台点动异常");
            writeWarningMessage("平台点动异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转点动异常报警
    controllerObject.cycilingJoggingWarning = S7_GetBitAt(readByte,44,3);
    if(S7_GetBitAt(readByte,44,3) != oneConObject.cycilingJoggingWarning)
    {
        oneConObject.cycilingJoggingWarning = S7_GetBitAt(readByte,44,3);
        if(oneConObject.cycilingJoggingWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转点动异常");
            writeWarningMessage("旋转点动异常");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //气体压力低报警
    controllerObject.pressureLowWarning = S7_GetBitAt(readByte,54,0);
    if(S7_GetBitAt(readByte,54,0) != oneConObject.pressureLowWarning)
    {
        oneConObject.pressureLowWarning = S7_GetBitAt(readByte,54,0);
        if(oneConObject.pressureLowWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("气体压力过低");
            writeWarningMessage("气体压力过低");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门Y1伺服报警
    controllerObject.gantryY1ServoWarning = S7_GetBitAt(readByte,54,3);
    if(S7_GetBitAt(readByte,54,3) != oneConObject.gantryY1ServoWarning)
    {
        oneConObject.gantryY1ServoWarning = S7_GetBitAt(readByte,54,3);
        if(oneConObject.gantryY1ServoWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门Y1伺服驱动器报警");
            writeWarningMessage("龙门Y1伺服驱动器报警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //龙门Y2伺服报警
    controllerObject.gantryY2ServoWarning = S7_GetBitAt(readByte,54,4);
    if(S7_GetBitAt(readByte,54,4) != oneConObject.gantryY2ServoWarning)
    {
        oneConObject.gantryY2ServoWarning = S7_GetBitAt(readByte,54,4);
        if(oneConObject.gantryY2ServoWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("龙门Y2伺服驱动器报警");
            writeWarningMessage("龙门Y2伺服驱动器报警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //平台伺服报警
    controllerObject.platformServoWarning = S7_GetBitAt(readByte,54,5);
    if(S7_GetBitAt(readByte,54,5) != oneConObject.platformServoWarning)
    {
        oneConObject.platformServoWarning = S7_GetBitAt(readByte,54,5);
        if(oneConObject.platformServoWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("平台伺服驱动器报警");
            writeWarningMessage("平台伺服驱动器报警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }

    }
    //旋转伺服报警
    controllerObject.cycilingServoWarning = S7_GetBitAt(readByte,54,6);
    if(S7_GetBitAt(readByte,54,6) != oneConObject.cycilingServoWarning)
    {
        oneConObject.cycilingServoWarning = S7_GetBitAt(readByte,54,6);
        if(oneConObject.cycilingServoWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("旋转伺服驱动器报警");
            writeWarningMessage("旋转伺服驱动器报警");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //龙门Y1左限位报警
    controllerObject.gantryY1LeftLimitWarning = S7_GetBitAt(readByte,55,0);
    if(S7_GetBitAt(readByte,55,0) != oneConObject.gantryY1LeftLimitWarning)
    {
        oneConObject.gantryY1LeftLimitWarning = S7_GetBitAt(readByte,55,0);
        if(oneConObject.gantryY1LeftLimitWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("已经到达龙门Y1左限位");
            writeWarningMessage("已经到达龙门Y1左限位");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //龙门Y1右限位报警
    controllerObject.gantryY1RightLimitWarning = S7_GetBitAt(readByte,55,1);
    if(S7_GetBitAt(readByte,55,1) != oneConObject.gantryY1RightLimitWarning)
    {
        oneConObject.gantryY1RightLimitWarning = S7_GetBitAt(readByte,55,1);
        if(oneConObject.gantryY1RightLimitWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("已经到达龙门Y1右限位");
            writeWarningMessage("已经到达龙门Y1右限位");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //龙门Y2左限位报警
    controllerObject.gantryY2LeftLimitWarning = S7_GetBitAt(readByte,55,2);
    if(S7_GetBitAt(readByte,55,2) != oneConObject.gantryY2LeftLimitWarning)
    {
        oneConObject.gantryY2LeftLimitWarning = S7_GetBitAt(readByte,55,2);
        if(oneConObject.gantryY2LeftLimitWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("已经到达龙门Y2左限位");
            writeWarningMessage("已经到达龙门Y2左限位");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //龙门Y2右限位报警
    controllerObject.gantryY2RightLimitWarning = S7_GetBitAt(readByte,55,3);
    if(S7_GetBitAt(readByte,55,3) != oneConObject.gantryY2RightLimitWarning)
    {
        oneConObject.gantryY2RightLimitWarning = S7_GetBitAt(readByte,55,3);
        if(oneConObject.gantryY2RightLimitWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("已经到达龙门Y2右限位");
            writeWarningMessage("已经到达龙门Y2右限位");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //平台左限位报警
    controllerObject.platformLeftLimitWarning = S7_GetBitAt(readByte,55,4);
    if(S7_GetBitAt(readByte,55,4) != oneConObject.platformLeftLimitWarning)
    {
        oneConObject.platformLeftLimitWarning = S7_GetBitAt(readByte,55,4);
        if(oneConObject.platformLeftLimitWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("已经到达平台左限位");
            writeWarningMessage("已经到达平台左限位");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }
    //平台右限位报警
    controllerObject.platformRightLimitWarning = S7_GetBitAt(readByte,55,5);
    if(S7_GetBitAt(readByte,55,5) != oneConObject.platformRightLimitWarning)
    {
        oneConObject.platformRightLimitWarning = S7_GetBitAt(readByte,55,5);
        if(oneConObject.platformRightLimitWarning == true)
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/redAlarm.png"));
            myHelper::ShowMessageBoxError("已经到达平台右限位");
            writeWarningMessage("已经到达平台右限位");
        }
        else
        {
            ui->warningLabel->setPixmap(QPixmap(":/img/images/normalAlarm.png"));
        }
    }

}
//初始化QFrame里面的按钮
void Widget::initPushbuttonByFrame(QFrame *frame)
{
    QList<QPushButton *> btns = frame->findChildren<QPushButton *>();
    foreach (QPushButton * btn, btns)
    {
        connect(btn, &QPushButton::clicked, this, &Widget::initPushbuttonByFrame_slot);
    }
}
//初始化QFrame里面的按钮槽函数
void Widget::initPushbuttonByFrame_slot()
{
    QPushButton *btn = (QPushButton *)sender();
    QString objName = btn->objectName();
    //切换主页
    if(objName == "Btn_mainPage")
    {
        ui->stackedWidget->setCurrentWidget(ui->mainPage);
        changeWidgetButtonStyle(ui->Btn_mainPage);
        qlog("成功切换到主页！");
    }

    //切换用户管理页面
    if(objName == "Btn_userManage")
    {
        if(isAdmin() == false)
        {
            myHelper::ShowMessageBoxError("只有管理员账号才可以操作！");
            return;
        }
        ui->stackedWidget->setCurrentWidget(ui->userPage);
        changeWidgetButtonStyle(ui->Btn_userManage);
        QVector<User> users;
        oper.queryAllUser(users);
        showUserTable(users);
        qlog("成功切换到用户管理页面！");
    }
    //切换到数据管理页面
    if(objName == "Btn_dataQuery")
    {
        ui->stackedWidget->setCurrentWidget(ui->dataQuery);
        changeWidgetButtonStyle(ui->Btn_dataQuery);
        qlog("成功切换到数据管理页面！");
    }
    if(objName == "Btn_correctionSetting")
    {
        if(isAdmin() == false)
        {
            myHelper::ShowMessageBoxError("只有管理员才有操作权限！");
            return;
        }
        limitform = new LimitForm();
        limitform->setAttribute(Qt::WA_DeleteOnClose);
        limitform->setWindowModality(Qt::ApplicationModal);
        connect(limitform,&LimitForm::hideCurrentWindow,[=](){
            setting->show();
        });
        limitform->show();
    }
    //用户管理-- 新增按钮
    if(objName == "addUserBtn")
    {
        isShowUserTable = true;
        int rowCount = ui->userTable->rowCount();
        ui->userTable->insertRow(rowCount);

        QTableWidgetItem *item0 = new QTableWidgetItem();
        item0->setFlags(item0->flags() & 33);
        item0->setTextAlignment(Qt::AlignCenter);
        item0->setText(QString::number(rowCount + 1));

        QTableWidgetItem *item1 = new QTableWidgetItem();
        item1->setTextAlignment(Qt::AlignCenter);
        item1->setText("");

        QTableWidgetItem *item2 = new QTableWidgetItem();
        item2->setTextAlignment(Qt::AlignCenter);
        item2->setText("1");

        ui->userTable->setItem(rowCount, 0, item0);
        ui->userTable->setItem(rowCount, 1, item1);
        ui->userTable->setItem(rowCount, 2, item2);

        QStringList roleList;
        roleList.append("管理员");
        roleList.append("操作员");
        QComboBox *box = new QComboBox();
        box->addItems(roleList);
        ui->userTable->setCellWidget(rowCount, 3, box);

        QCheckBox *check = new QCheckBox();
        check->setText("是否可用");
        check->setChecked(true);
        ui->userTable->setCellWidget(rowCount, 4, check);

        ui->userTable->clearSelection();
        ui->userTable->setCurrentItem(nullptr);

        isShowUserTable = false;
    }
    //用户管理-- 删除按钮
    if(objName == "deleteUserBtn")
    {
        int currentRow = ui->userTable->currentRow();
        if(currentRow == -1)
        {
            myHelper::ShowMessageBoxInfo("选择需要删除的行");
        }
        else
        {
            //删除数据库
            if((ui->userTable->item(currentRow, 1) != nullptr) && (!ui->userTable->item(currentRow, 1)->text().remove(QRegExp("\\s")).isEmpty()))
            {
                oper.deleteUser(ui->userTable->item(currentRow, 1)->text());
            }
            ui->userTable->removeRow(currentRow);
            ui->userTable->clearSelection();
            ui->userTable->setCurrentItem(nullptr);
        }

    }
    //用户管理-- 保存按钮
    if(objName == "saveUserBtn")
    {
        QVector<User> users;
        for(int i = 0; i < ui->userTable->rowCount(); i++)
        {
            User user;
            for(int j = 1; j < ui->userTable->columnCount(); j++)
            {
                if(j == 1)
                {
                    if(ui->userTable->item(i, j) == nullptr ||  ui->userTable->item(i, j)->text().remove(QRegExp("\\s")).isEmpty())
                    {
                        myHelper::ShowMessageBoxError("账号不能为空");
                        return;
                    }
                    else
                    {
                        user.name = ui->userTable->item(i, j)->text();
                    }
                }
                if(j == 2)
                {
                    if(ui->userTable->item(i, j) == nullptr ||  ui->userTable->item(i, j)->text().remove(QRegExp("\\s")).isEmpty())
                    {
                        user.pwd = "0";
                    }else
                    {
                        user.pwd = ui->userTable->item(i, j)->text();
                    }
                }
                if(j == 3)
                {
                    QComboBox *box = dynamic_cast<QComboBox*>(ui->userTable->cellWidget(i, j));
                    user.role = box->currentText();
                }
                if(j == 4)
                {
                    QCheckBox *box = (QCheckBox*)ui->userTable->cellWidget(i, j);
                    user.is_used = box->isChecked() ? "1" : "0";
                }

            }
            users.push_back(user);
        }
        int ok = oper.saveUsers(users);
        if(ok == 0)
        {
            myHelper::ShowMessageBoxInfo("保存成功");
            QVector<User> users;
            oper.queryAllUser(users);
            showUserTable(users);
            ui->userTable->clearSelection();
            ui->userTable->setCurrentItem(nullptr);
        }else
        {
            myHelper::ShowMessageBoxError("保存失败");
        }

    }

}

void Widget::initPushbuttonByGroup(QGroupBox *box)
{
    QList<QPushButton *> btns = box->findChildren<QPushButton *>();
    foreach (QPushButton * btn, btns)
    {
        connect(btn, &QPushButton::clicked, this, &Widget::initPushbuttonByGroup_slot);
    }
}

void Widget::initPushbuttonByGroup_slot()
{
    QPushButton *btn = (QPushButton *)sender();
    QString objName = btn->objectName();
}

void Widget::changeWidgetButtonStyle(QPushButton *checkButton)
{
    QList<QPushButton *> btns = ui->frame_4->findChildren<QPushButton *>();
    foreach (QPushButton * btn, btns) {
        if(btn == checkButton){
            btn->setStyleSheet(pushButtonGreenStyle);
        }else{
            btn->setStyleSheet(defaultPushButtonStyle);
            btn->setStyleSheet(hoverPushButtonStyle);
            btn->setStyleSheet(disablePushButtonStyle);
        }
    }
}

void Widget::showUserTable(QVector<User> users)
{

    isShowUserTable = true;
    int row = users.size();
    int col = ui->userTable->columnCount();
    ui->userTable->setRowCount(row);
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            if(j == 0 || j == 1 || j == 2){
                QTableWidgetItem *item = new QTableWidgetItem();
                item->setTextAlignment(Qt::AlignCenter);
                if(j == 0){
                    item->setFlags(item->flags() & 33);
                    item->setText(QString::number(i + 1));
                }
                if(j == 1){
                    item->setFlags(item->flags() & 33);
                    item->setText(users[i].name);
                }
                if(j == 2){
                    item->setText(users[i].pwd);
                }
                ui->userTable->setItem(i, j, item);
            }else{
                if(j == 3){
                    QStringList roleList;
                    roleList.append("管理员");
                    roleList.append("操作员");
                    QComboBox *box = new QComboBox();
                    box->addItems(roleList);
                    box->setCurrentText(users[i].role);
                    ui->userTable->setCellWidget(i, j, box);
                }
                if(j == 4){
                    QCheckBox *check = new QCheckBox();
                    check->setText("是否可用");
                    if(users[i].is_used.compare("0") == 0){
                        check->setChecked(false);
                    }else{
                        check->setChecked(true);
                    }
                    ui->userTable->setCellWidget(i, j, check);
                }
            }
        }
    }
    isShowUserTable = false;
}
//判断是否为管理员
bool Widget::isAdmin()
{
    if(currentUser.role == "管理员"){
        //管理员
        return true;
    }else{
        //操作员
        return false;
    }
}
//判断DoubleSpinBox的值是否全为0
bool Widget::areAllSpinboxesZero(QGroupBox *groupBox)
{
    QList<QDoubleSpinBox *> boxes = groupBox->findChildren<QDoubleSpinBox *>();
    // 遍历所有QDoubleSpinBox，检查值是否全为0
    for(QDoubleSpinBox *box : boxes)
    {
        if(box->value() == 0)
        {
            return false;
        }
    }
    return true;
}
//导出标准数据
void Widget::exportStdData()
{
    int reply = myHelper::ShowMessageBoxQuesion("是否要导出标准数据到Excel？确定前请检查数据是否为有效数据！");
    if(reply == QDialog::Accepted)
    {
        if(ui->Edit_batchNum->text().isEmpty() || ui->Edit_typeNum->text().isEmpty()
                || ui->Edit_processNum->text().isEmpty() || ui->Edit_componentNum->text().isEmpty())
        {
            myHelper::ShowMessageBoxError("无法导出Excel，产品信息有空值！");
            return;
        }
        if(ui->sensorOneStdData->value() == 0 && ui->sensorTwoStdData->value() == 0 && ui->sensorThreeStdData->value() == 0
                && ui->sensorFourStdData->value() == 0 && ui->sensorFiveStdData->value() == 0 && ui->sensorSixStdData->value() == 0
                && ui->sensorSevenStdData->value() == 0)
        {
            myHelper::ShowMessageBoxError("无法导出Excel，标准数据全为0！");
            return;
        }
        // 打开 Excel 文件
        QXlsx::Document xlsx("./datas/standardData.xlsx");
        //固定列宽20
        for(int col = 1; col <= 12; col++)
        {
            xlsx.setColumnWidth(col, col, 20);
        }
        QXlsx::Format centerAlignFormat;
        centerAlignFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        centerAlignFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

        // 在第一行添加表头
        if (xlsx.dimension().rowCount() == 0)
        {
            QStringList headers = {"批次号", "型号", "工序号", "零组件号", "#1传感器值",
                                   "#2传感器值", "#3传感器值", "#4传感器值", "#5传感器值", "#6传感器值",
                                   "#7传感器值", "日期/时间"};
            for (int col = 1; col <= headers.size(); ++col)
            {
                xlsx.write(1, col, headers.at(col - 1), centerAlignFormat);
            }
        }
        // 添加新数据
        int row = xlsx.dimension().rowCount() + 1;
        xlsx.write(row, 1, ui->Edit_batchNum->text(), centerAlignFormat);
        xlsx.write(row, 2, ui->Edit_typeNum->text(), centerAlignFormat);
        xlsx.write(row, 3, ui->Edit_processNum->text(), centerAlignFormat);
        xlsx.write(row, 4, ui->Edit_componentNum->text(), centerAlignFormat);
        xlsx.write(row, 5, ui->sensorOneStdData->value(), centerAlignFormat);
        xlsx.write(row, 6, ui->sensorTwoStdData->value(), centerAlignFormat);
        xlsx.write(row, 7, ui->sensorThreeStdData->value(), centerAlignFormat);
        xlsx.write(row, 8, ui->sensorFourStdData->value(), centerAlignFormat);
        xlsx.write(row, 9, ui->sensorFiveStdData->value(), centerAlignFormat);
        xlsx.write(row, 10, ui->sensorSixStdData->value(), centerAlignFormat);
        xlsx.write(row, 11, ui->sensorSevenStdData->value(), centerAlignFormat);
        xlsx.write(row, 12, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), centerAlignFormat);

        if(xlsx.save())
        {
            myHelper::ShowMessageBoxInfo("导入标准数据到Excel成功！");
        }
        else
        {
            myHelper::ShowMessageBoxError("导入标准数据到Excel失败！");
        }
    }
}
//导出正式数据
void Widget::exportOfficialData()
{
    int reply = myHelper::ShowMessageBoxQuesion("是否要导出正式数据到Excel？确定前请检查数据是否为有效数据！");
    if(reply == QDialog::Accepted)
    {
        if(ui->Edit_batchNum->text().isEmpty() || ui->Edit_typeNum->text().isEmpty()
                || ui->Edit_processNum->text().isEmpty() || ui->Edit_componentNum->text().isEmpty())
        {
            myHelper::ShowMessageBoxError("无法导出Excel，产品信息有空值！");
            return;
        }
        if(ui->sensorOneData->value() == 0 && ui->sensorTwoData->value() == 0 && ui->sensorThreeData->value() == 0
                && ui->sensorFourData->value() == 0 && ui->sensorFiveData->value() == 0 && ui->sensorSixData->value() == 0
                && ui->sensorSevenData->value() == 0)
        {
            myHelper::ShowMessageBoxError("无法导出Excel，正式数据全为0！");
            return;
        }
        // 打开 Excel 文件
        QXlsx::Document xlsx("./datas/officialData.xlsx");
        //设置居中格式
        QXlsx::Format centerAlignFormat;
        centerAlignFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        centerAlignFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
        //固定列宽20
        for(int col = 1; col <= 12; col++)
        {
            xlsx.setColumnWidth(col, col, 20);
        }
        // 在第一行添加表头
        if (xlsx.dimension().rowCount() == 0)
        {
            QStringList headers = {"批次号", "型号", "工序号", "零组件号", "#1传感器值",
                                   "#2传感器值", "#3传感器值", "#4传感器值", "#5传感器值", "#6传感器值",
                                   "#7传感器值", "日期/时间"};
            for (int col = 1; col <= headers.size(); ++col)
            {
                xlsx.write(1, col, headers.at(col - 1), centerAlignFormat);
            }
        }
        // 添加新数据
        int row = xlsx.dimension().rowCount() + 1;
        xlsx.write(row, 1, ui->Edit_batchNum->text(), centerAlignFormat);
        xlsx.write(row, 2, ui->Edit_typeNum->text(), centerAlignFormat);
        xlsx.write(row, 3, ui->Edit_processNum->text(), centerAlignFormat);
        xlsx.write(row, 4, ui->Edit_componentNum->text(), centerAlignFormat);
        xlsx.write(row, 5, ui->sensorOneData->value(), centerAlignFormat);
        xlsx.write(row, 6, ui->sensorTwoData->value(), centerAlignFormat);
        xlsx.write(row, 7, ui->sensorThreeData->value(), centerAlignFormat);
        xlsx.write(row, 8, ui->sensorFourData->value(), centerAlignFormat);
        xlsx.write(row, 9, ui->sensorFiveData->value(), centerAlignFormat);
        xlsx.write(row, 10, ui->sensorSixData->value(), centerAlignFormat);
        xlsx.write(row, 11, ui->sensorSevenData->value(), centerAlignFormat);
        xlsx.write(row, 12, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), centerAlignFormat);

        if(xlsx.save())
        {
            myHelper::ShowMessageBoxInfo("导入正式数据到Excel成功！");
        }
        else
        {
            myHelper::ShowMessageBoxError("导入正式数据到Excel失败！");
        }
    }
}
//导出最终查询数据
void Widget::exportUltimteData()
{
    if(ui->result_tableWidget->rowCount() == 0)
    {
        myHelper::ShowMessageBoxError("没有数据可供导出！");
        qlog("没有数据可供导出！");
        return;
    }
    QXlsx::Document xlsx;
    int columnCount = ui->result_tableWidget->columnCount();
    int rowCount = ui->result_tableWidget->rowCount();
    //设置居中显示
    QXlsx::Format centerAlignFormat;
    centerAlignFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    centerAlignFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    //设置列宽
    for(int col = 1; col <= 13; col++)
    {
        xlsx.setColumnWidth(col, col, 20);
    }
    QStringList headerLabels;
    for(int column = 0; column < columnCount; ++column)
    {
        QTableWidgetItem* headerItem = ui->result_tableWidget->horizontalHeaderItem(column);
        if (headerItem != nullptr) {
            QString headerText = headerItem->text();
            headerLabels.append(headerText);

            xlsx.write(1, column + 1, headerText, centerAlignFormat);
        }
    }
    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            QTableWidgetItem* item = ui->result_tableWidget->item(row, column);
            if (item != nullptr) {
                QString text = item->text();
                xlsx.write(row + 2, column + 1, text, centerAlignFormat);

            }
        }
    }
    // 弹出文件保存对话框并导出Excel文件
    QString filePath = QFileDialog::getSaveFileName(this, "保存Excel文件", "", "Excel 文件 (*.xlsx)");
    if (!filePath.isEmpty()) {
        if (xlsx.saveAs(filePath)) {
            myHelper::ShowMessageBoxInfo("导出成功！");
        } else {
            myHelper::ShowMessageBoxError("导出失败！");
        }
    }
}
//将数据展示到表格中
void Widget::showDetectDataOnTable()
{

    oneConObject.correctionValueOne = ui->sensorOneData->value() - ui->sensorOneStdData->value();
    oneConObject.correctionValueTwo = ui->sensorTwoData->value() - ui->sensorTwoStdData->value();
    oneConObject.correctionValueThree = ui->sensorThreeData->value() - ui->sensorThreeStdData->value();
    oneConObject.correctionValueFour = ui->sensorFourData->value() - ui->sensorFourStdData->value();
    oneConObject.correctionValueFive = ui->sensorFiveData->value() - ui->sensorFiveStdData->value();
    oneConObject.correctionValueSix = ui->sensorSixData->value() - ui->sensorSixStdData->value();
    oneConObject.correctionValueSeven = ui->sensorSevenData->value() - ui->sensorSevenStdData->value();
    double sensorOneLowValue = myHelper::readSettings("correctionParameters/sensorOneLowValue").toDouble();
    double sensorOneHighValue = myHelper::readSettings("correctionParameters/sensorOneHighValue").toDouble();
    double sensorTwoLowValue = myHelper::readSettings("correctionParameters/sensorTwoLowValue").toDouble();
    double sensorTwoHighValue = myHelper::readSettings("correctionParameters/sensorTwoHighValue").toDouble();
    double sensorThreeLowValue = myHelper::readSettings("correctionParameters/sensorThreeLowValue").toDouble();
    double sensorThreeHighValue = myHelper::readSettings("correctionParameters/sensorThreeHighValue").toDouble();
    double sensorFourLowValue = myHelper::readSettings("correctionParameters/sensorFourLowValue").toDouble();
    double sensorFourHighValue = myHelper::readSettings("correctionParameters/sensorFourHighValue").toDouble();
    double sensorFiveLowValue = myHelper::readSettings("correctionParameters/sensorFiveLowValue").toDouble();
    double sensorFiveHighValue = myHelper::readSettings("correctionParameters/sensorFiveHighValue").toDouble();
    double sensorSixLowValue = myHelper::readSettings("correctionParameters/sensorSixLowValue").toDouble();
    double sensorSixHighValue = myHelper::readSettings("correctionParameters/sensorSixHighValue").toDouble();
    double sensorSevenLowValue = myHelper::readSettings("correctionParameters/sensorSevenLowValue").toDouble();
    double sensorSevenHighValue = myHelper::readSettings("correctionParameters/sensorSevenHighValue").toDouble();
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    for(int j=0; j<ui->tableWidget->columnCount();j++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() & 33);
        item->setTextAlignment(Qt::AlignHCenter);
        if(j==0)
        {
            item->setText(QString::number(oneConObject.correctionValueOne,'f',3));
            if(oneConObject.correctionValueOne >= sensorOneLowValue && oneConObject.correctionValueOne <= sensorOneHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueOne < sensorOneLowValue || oneConObject.correctionValueOne > sensorOneHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==1)
        {
            item->setText(QString::number(oneConObject.correctionValueTwo,'f',3));
            if(oneConObject.correctionValueTwo >= sensorTwoLowValue && oneConObject.correctionValueTwo <= sensorTwoHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueTwo < sensorTwoLowValue || oneConObject.correctionValueTwo > sensorTwoHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==2)
        {
            item->setText(QString::number(oneConObject.correctionValueThree,'f',3));
            if(oneConObject.correctionValueThree >= sensorThreeLowValue && oneConObject.correctionValueThree <= sensorThreeHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueThree < sensorThreeLowValue || oneConObject.correctionValueThree > sensorThreeHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==3)
        {
            item->setText(QString::number(oneConObject.correctionValueFour,'f',3));
            if(oneConObject.correctionValueFour >= sensorFourLowValue && oneConObject.correctionValueFour <= sensorFourHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueFour < sensorFourLowValue || oneConObject.correctionValueFour > sensorFourHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==4)
        {
            item->setText(QString::number(oneConObject.correctionValueFive,'f',3));
            if(oneConObject.correctionValueFive >= sensorFiveLowValue && oneConObject.correctionValueFive <= sensorFiveHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueFive < sensorFiveLowValue || oneConObject.correctionValueFive > sensorFiveHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==5)
        {
            item->setText(QString::number(oneConObject.correctionValueSix,'f',3));
            if(oneConObject.correctionValueSix >= sensorSixLowValue && oneConObject.correctionValueSix <= sensorSixHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueSix < sensorSixLowValue || oneConObject.correctionValueSix > sensorSixHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==6)
        {
            item->setText(QString::number(oneConObject.correctionValueSeven,'f',3));
            if(oneConObject.correctionValueSeven >= sensorSevenLowValue && oneConObject.correctionValueSeven <= sensorSevenHighValue)
            {
                item->setBackgroundColor(Qt::green);
            }
            if(oneConObject.correctionValueSeven < sensorSevenLowValue || oneConObject.correctionValueSeven > sensorSevenHighValue)
            {
                item->setBackgroundColor(Qt::red);
            }
        }
        if(j==7)
        {
            if((oneConObject.correctionValueOne >= sensorOneLowValue && oneConObject.correctionValueOne <= sensorOneHighValue)&&
                    (oneConObject.correctionValueTwo >= sensorTwoLowValue && oneConObject.correctionValueTwo <= sensorTwoHighValue)&&
                    (oneConObject.correctionValueThree >= sensorThreeLowValue && oneConObject.correctionValueThree <= sensorThreeHighValue)&&
                    (oneConObject.correctionValueFour >= sensorFourLowValue && oneConObject.correctionValueFour <= sensorFourHighValue)&&
                    (oneConObject.correctionValueFive >= sensorFiveLowValue && oneConObject.correctionValueFive <= sensorFiveHighValue)&&
                    (oneConObject.correctionValueSix >= sensorSixLowValue && oneConObject.correctionValueSix <= sensorSixHighValue)&&
                    (oneConObject.correctionValueSeven >= sensorSevenLowValue && oneConObject.correctionValueSeven <= sensorSevenHighValue))
            {
                item->setText("合格");
            }
            else
            {
                item->setText("不合格");
            }
        }
        ui->tableWidget->setItem(row,j,item);
    }

}
//存储相关数据到数据库
void Widget::saveDetectDataToTable()
{
    QVector<ResultSet> resultSet;
    ResultSet results;
    results.batchNum = ui->Edit_batchNum->text();
    results.typeNum = ui->Edit_typeNum->text();
    results.processNum = ui->Edit_processNum->text().toInt();
    results.componentNum = ui->Edit_componentNum->text();
    results.valueOne = QString::number(oneConObject.correctionValueOne, 'f', 3).toDouble();
    results.valueTwo = QString::number(oneConObject.correctionValueTwo, 'f', 3).toDouble();
    results.valueThree = QString::number(oneConObject.correctionValueThree, 'f', 3).toDouble();
    results.valueFour = QString::number(oneConObject.correctionValueFour, 'f', 3).toDouble();
    results.valueFive = QString::number(oneConObject.correctionValueFive, 'f', 3).toDouble();
    results.valueSix = QString::number(oneConObject.correctionValueSix, 'f', 3).toDouble();
    results.valueSeven = QString::number(oneConObject.correctionValueSeven, 'f', 3).toDouble();
    double sensorOneLowValue = myHelper::readSettings("correctionParameters/sensorOneLowValue").toDouble();
    double sensorOneHighValue = myHelper::readSettings("correctionParameters/sensorOneHighValue").toDouble();
    double sensorTwoLowValue = myHelper::readSettings("correctionParameters/sensorTwoLowValue").toDouble();
    double sensorTwoHighValue = myHelper::readSettings("correctionParameters/sensorTwoHighValue").toDouble();
    double sensorThreeLowValue = myHelper::readSettings("correctionParameters/sensorThreeLowValue").toDouble();
    double sensorThreeHighValue = myHelper::readSettings("correctionParameters/sensorThreeHighValue").toDouble();
    double sensorFourLowValue = myHelper::readSettings("correctionParameters/sensorFourLowValue").toDouble();
    double sensorFourHighValue = myHelper::readSettings("correctionParameters/sensorFourHighValue").toDouble();
    double sensorFiveLowValue = myHelper::readSettings("correctionParameters/sensorFiveLowValue").toDouble();
    double sensorFiveHighValue = myHelper::readSettings("correctionParameters/sensorFiveHighValue").toDouble();
    double sensorSixLowValue = myHelper::readSettings("correctionParameters/sensorSixLowValue").toDouble();
    double sensorSixHighValue = myHelper::readSettings("correctionParameters/sensorSixHighValue").toDouble();
    double sensorSevenLowValue = myHelper::readSettings("correctionParameters/sensorSevenLowValue").toDouble();
    double sensorSevenHighValue = myHelper::readSettings("correctionParameters/sensorSevenHighValue").toDouble();
    if((oneConObject.correctionValueOne >= sensorOneLowValue && oneConObject.correctionValueOne <= sensorOneHighValue)&&
            (oneConObject.correctionValueTwo >= sensorTwoLowValue && oneConObject.correctionValueTwo <= sensorTwoHighValue)&&
            (oneConObject.correctionValueThree >= sensorThreeLowValue && oneConObject.correctionValueThree <= sensorThreeHighValue)&&
            (oneConObject.correctionValueFour >= sensorFourLowValue && oneConObject.correctionValueFour <= sensorFourHighValue)&&
            (oneConObject.correctionValueFive >= sensorFiveLowValue && oneConObject.correctionValueFive <= sensorFiveHighValue)&&
            (oneConObject.correctionValueSix >= sensorSixLowValue && oneConObject.correctionValueSix <= sensorSixHighValue)&&
            (oneConObject.correctionValueSeven >= sensorSevenLowValue && oneConObject.correctionValueSeven <= sensorSevenHighValue))
    {
        results.result = "合格";
    }
    else
    {
        results.result = "不合格";
    }
    results.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    resultSet.push_back(results);
    oper.saveDetectResult(resultSet);

}
//展示最终数据到表格
void Widget::showMeasureDataOnTable(const QString measureBatchNum, const QString startTime, const QString endTime)
{
    resultSet.clear();
    oper.getDetectDataByBatchNum(resultSet,measureBatchNum,startTime,endTime);
    int row = resultSet.size();
    ui->result_tableWidget->setRowCount(row);
    int col = ui->result_tableWidget->columnCount();
    for(int i=0; i<row; i++)
    {
        for(int j=0; j<col; j++)
        {
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(item->flags() & 33);
            if(j==0)
            {
                item->setText(resultSet[i].batchNum);
            }
            if(j==1)
            {
                item->setText(resultSet[i].typeNum);
            }
            if(j==2)
            {
                item->setText(QString::number(resultSet[i].processNum));
            }
            if(j==3)
            {
                item->setText(resultSet[i].componentNum);
            }
            if(j==4)
            {
                item->setText(QString::number(resultSet[i].valueOne));
            }
            if(j==5)
            {
                item->setText(QString::number(resultSet[i].valueTwo));
            }
            if(j==6)
            {
                item->setText(QString::number(resultSet[i].valueThree));
            }
            if(j==7)
            {
                item->setText(QString::number(resultSet[i].valueFour));
            }
            if(j==8)
            {
                item->setText(QString::number(resultSet[i].valueFive));
            }
            if(j==9)
            {
                item->setText(QString::number(resultSet[i].valueSix));
            }
            if(j==10)
            {
                item->setText(QString::number(resultSet[i].valueSeven));
            }
            if(j==11)
            {
                item->setText(resultSet[i].result);
                if(resultSet[i].result == "合格")
                {
                    item->setBackgroundColor(Qt::green);
                }
                else if(resultSet[i].result == "不合格")
                {
                    item->setBackgroundColor(Qt::red);
                }
            }
            if(j==12)
            {
                ui->result_tableWidget->setColumnWidth(j,250);
                item->setText(resultSet[i].dateTime);
            }
            ui->result_tableWidget->setItem(i,j,item);
        }
    }
}
//触摸屏权限开关
void Widget::touchScreenPriority_Slot(int state)
{
    if(controllerInitStatus == false)
    {
        ui->touchscreenPermission->setCheckable(false);
        return;
    }
    //触摸屏权限打开后锁定界面操作控制按钮
    if(state == Qt::Checked)
    {
        int open = snap7_plc->sendBitToRobot(30,1,true,writeDbNum,"触摸屏权限打开信号");
        if(0 == open)
        {
            ui->touchSceenStatus->setStyleSheet("background-color:green;color: rgb(255, 255, 255);");
            ui->touchSceenStatus->setText("已开启");
            ui->gantryStart->setEnabled(state != Qt::Checked);
            ui->gantryStop->setEnabled(state != Qt::Checked);
            ui->gantryReset->setEnabled(state != Qt::Checked);
            ui->gantryBackToZero->setEnabled(state != Qt::Checked);
            ui->platformStart->setEnabled(state != Qt::Checked);
            ui->platformStop->setEnabled(state != Qt::Checked);
            ui->platformReset->setEnabled(state != Qt::Checked);
            ui->platformBackToZero->setEnabled(state != Qt::Checked);
            ui->cyclingStart->setEnabled(state != Qt::Checked);
            ui->cyclingStop->setEnabled(state != Qt::Checked);
            ui->cyclingReset->setEnabled(state != Qt::Checked);
            ui->cyclingBackToZero->setEnabled(state != Qt::Checked);
            ui->Reset->setEnabled(state != Qt::Checked);
        }

    }
    //触摸屏权限关闭后解开控制按钮锁
    if(state == Qt::Unchecked)
    {
        int close = snap7_plc->sendBitToRobot(30,1,false,writeDbNum,"触摸屏权限关闭信号");
        if(0 == close)
        {
            ui->touchSceenStatus->setStyleSheet("background-color:#F42215;color: rgb(255, 255, 255);");
            ui->touchSceenStatus->setText("已关闭");
            ui->gantryStart->setEnabled(state == Qt::Unchecked);
            ui->gantryStop->setEnabled(state == Qt::Unchecked);
            ui->gantryReset->setEnabled(state == Qt::Unchecked);
            ui->gantryBackToZero->setEnabled(state == Qt::Unchecked);
            ui->platformStart->setEnabled(state == Qt::Unchecked);
            ui->platformStop->setEnabled(state == Qt::Unchecked);
            ui->platformReset->setEnabled(state == Qt::Unchecked);
            ui->platformBackToZero->setEnabled(state == Qt::Unchecked);
            ui->cyclingStart->setEnabled(state == Qt::Unchecked);
            ui->cyclingStop->setEnabled(state == Qt::Unchecked);
            ui->cyclingReset->setEnabled(state == Qt::Unchecked);
            ui->cyclingBackToZero->setEnabled(state == Qt::Unchecked);
            ui->Reset->setEnabled(state == Qt::Unchecked);
        }
    }
}

void Widget::zeroCorrectionValue_Slot(int state)
{
    ui->zeroCorrectionValue->setEnabled(state == Qt::Checked);
    ui->saveZeroCorrectionValue->setEnabled(state == Qt::Checked);
}

bool Widget::saveZeroCorrectionValue_Slot()
{
    myHelper::writeSettings("correctionParameters/originalPointCorrectionValue",ui->zeroCorrectionValue->value());
    return true;
}
//程序关闭事件
void Widget::closeEvent(QCloseEvent *event)
{
    int reply = myHelper::ShowMessageBoxQuesion("是否要关闭程序？关闭前请检查所有运动机构是否都处于停止状态以及平台、龙门是否进行复位操作！");
    if(reply == QDialog::Rejected)
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }

}
//记录报警日志
void Widget::writeWarningMessage(const QString &warningMessage)
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logMessage = currentTime +"-----"+ warningMessage;
    QString currentPath = QDir::currentPath();
    QString filePath = currentPath + "/warningMessage.txt";
    QFile file(filePath);
    if(file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream textStream(&file);
        textStream.setCodec("UTF-8");
        textStream << logMessage << endl;
        file.close();
    }
}
