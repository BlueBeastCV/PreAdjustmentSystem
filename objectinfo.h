#pragma execution_character_set("utf-8")
#ifndef OBJECTINFO_H
#define OBJECTINFO_H
#include <QString>
#include <QDebug>

class User
{
public:
    User(){
        name = nullptr;
        pwd = nullptr;
        role = nullptr;
        is_used = nullptr;
    }
    friend QDebug& operator<<(QDebug out, const User& info)
    {
        out << "账号：" << info.name << "密码：" << info.pwd << "角色：" << info.role << "是否可用：" << info.is_used;
        return out;
    }
public:
    QString name;
    QString pwd;
    QString role;
    QString is_used;

};

class ProductionInfo
{
public:
    ProductionInfo()
    {
        batchNum = nullptr;
        typeNum = nullptr;
        processNum = 0;
        componentNum = nullptr;
        dates = nullptr;
    }
    friend QDebug& operator<<(QDebug out, const ProductionInfo& info)
    {
        out << "批次号：" << info.batchNum << "型号：" << info.typeNum << "工序号：" << info.processNum << "是否可用：" << info.componentNum
            <<"零组件号："<< info.componentNum << "日期：" << info.dates;
        return out;
    }
public:
    QString batchNum;
    QString typeNum;
    int processNum;
    QString componentNum;
    QString dates;

};

class ResultSet
{
public:
    ResultSet()
    {
        batchNum = nullptr;
        typeNum = nullptr;
        processNum = 0;
        componentNum = nullptr;
        valueOne = 0.0;
        valueTwo = 0.0;
        valueThree = 0.0;
        valueFour = 0.0;
        valueFive = 0.0;
        valueSix = 0.0;
        valueSeven = 0.0;
        result = nullptr;
        dateTime = nullptr;
    }

public:
    QString batchNum;
    QString typeNum;
    int processNum;
    QString componentNum;
    double valueOne;
    double valueTwo;
    double valueThree;
    double valueFour;
    double valueFive;
    double valueSix;
    double valueSeven;
    QString result;
    QString dateTime;


};

//控制系统读取对象
class ControllerObject
{
public:
    ControllerObject()
    {
        correctionValueOne = 0.0;
        correctionValueTwo = 0.0;
        correctionValueThree = 0.0;
        correctionValueFour = 0.0;
        correctionValueFive = 0.0;
        correctionValueSix = 0.0;
        correctionValueSeven = 0.0;
        officialValueOne = 0.0;
        officialValueTwo = 0.0;
        officialValueThree = 0.0;
        officialValueFour = 0.0;
        officialValueFive = 0.0;
        officialValueSix = 0.0;
        officialValueSeven = 0.0;
        stdValueOne = 0.0;
        stdValueTwo = 0.0;
        stdValueThree = 0.0;
        stdValueFour = 0.0;
        stdValueFive = 0.0;
        stdValueSix = 0.0;
        stdValueSeven = 0.0;
        gantryRealLocation = 0.0;
        platformRealLocation = 0.0;

    }
public:
    float correctionValueOne;
    float correctionValueTwo;
    float correctionValueThree;
    float correctionValueFour;
    float correctionValueFive;
    float correctionValueSix;
    float correctionValueSeven;
    float officialValueOne;
    float officialValueTwo;
    float officialValueThree;
    float officialValueFour;
    float officialValueFive;
    float officialValueSix;
    float officialValueSeven;
    float stdValueOne;
    float stdValueTwo;
    float stdValueThree;
    float stdValueFour;
    float stdValueFive;
    float stdValueSix;
    float stdValueSeven;
    //龙门
    bool gantryAbsLocationFinish = false; //定位完成信号
    bool gantryBackToZeroWarning = false; //回原点报警
    bool gantryRelativeLocationWarning = false; //相对定位报警
    bool gantryAbsLocationWarning = false; //绝对定位报警
    bool gantryEnableWarning = false; //龙门使能异常报警
    bool gantryResetWarning = false; //龙门复位异常报警
    float gantryRealLocation; //龙门实际位置
    bool gantryBackToZeroFinished = false; // 龙门回零完成信号
    bool gantrySensorValueWarning = false; // 龙门传感器数据异常
    //平台
    bool platformAbsLocationFinish = false; //平台绝对定位完成型号
    bool platformBackToZeroWarning = false; //平台回原点报警
    bool platformRelativeLocationWarning = false; //平台相对定位报警
    bool platformAbsLocationWarning = false; //平台绝对定位报警
    bool platformEnableWarning = false; //平台使能报警
    bool platformResetWarning = false; //平台复位报警
    float platformRealLocation; //平台实际位置
    bool platformSensorValueWarning = false; //平台传感器数据异常
    //旋转
    bool cycilingAbsLocationFinish = false; //旋转绝对定位完成型号
    bool cycilingBackToZeroWarning = false; //旋转回原点报警
    bool cycilingRelativeLocationWarning = false; //旋转相对定位报警
    bool cycilingAbsLocationWarning = false; //旋转绝对定位报警
    bool cycilingEnableWarning = false; //旋转使能报警
    bool cycilingResetWarning = false; //旋转复位报警
    float cycilingRealLocation; //旋转实际位置
    bool cyclingProcessAxisWarning = false; //旋转控制工艺轴报警
    //控制台
    bool controlPlatformWarning = false; // 控制台急停报警
    bool controlBoxWarning = false; //控制盒急停报警
    //油冷机
    bool oilCoolerWarning = false; //油冷机报警
    //点动
    bool gantryJoggingWarning = false; //龙门点动报警
    bool platformJoggingWarning = false; //平台点动报警
    bool cycilingJoggingWarning = false; //旋转点动报警
    //气压
    bool pressureLowWarning = false; //气压低
    //伺服驱动
    bool gantryY1ServoWarning = false; //龙门Y1伺服报警
    bool gantryY2ServoWarning = false; //龙门Y2伺服报警
    bool platformServoWarning = false; //平台伺服报警
    bool cycilingServoWarning = false; //选钻伺服报警
    //限位报警
    bool gantryY1LeftLimitWarning = false;    //龙门Y1左限位报警
    bool gantryY1RightLimitWarning = false;   //龙门Y1右限位报警
    bool gantryY2LeftLimitWarning = false;    //龙门Y2左限位报警
    bool gantryY2RightLimitWarning = false;   //龙门Y2右限位报警
    bool platformLeftLimitWarning = false;    //平台左限位报警
    bool platformRightLimitWarning = false;   //平台右限位报警
    //龙门，平台一起复位成功信号
    bool gantryAndPlatformAreReset = false;
};



#endif // OBJECTINFO_H
