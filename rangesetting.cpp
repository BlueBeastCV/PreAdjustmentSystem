#include "rangesetting.h"
#include "ui_rangesetting.h"

rangesetting::rangesetting(User u,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::rangesetting)
{
    ui->setupUi(this);
    myHelper::FormInCenter(this);//窗体居中显示
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    ui->titleWidget->set_lab_Title("修正范围设置");
    setFixedSize(this->width(),this->height());

    connect(ui->Lock,&QPushButton::clicked,this,&rangesetting::locked_Slot);
    connect(ui->Load,&QPushButton::clicked,this,&rangesetting::readParametersFromSettings);
    connect(ui->WriteIn,&QPushButton::clicked,this,[=](){
        if(writeParametersInSettings() == true)
        {
            myHelper::ShowMessageBoxInfo("写入成功！");
        }
    });
}

rangesetting::~rangesetting()
{
    delete ui;
}

void rangesetting::readParametersFromSettings()
{
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
    ui->smallRangeOne->setValue(sensorOneLowValue);
    ui->bigRangeOne->setValue(sensorOneHighValue);
    ui->smallRangeTwo->setValue(sensorTwoLowValue);
    ui->bigRangeTwo->setValue(sensorTwoHighValue);
    ui->smallRangeThree->setValue(sensorThreeLowValue);
    ui->bigRangeThree->setValue(sensorThreeHighValue);
    ui->smallRangeFour->setValue(sensorFourLowValue);
    ui->bigRangeFour->setValue(sensorFourHighValue);
    ui->smallRangeFive->setValue(sensorFiveLowValue);
    ui->bigRangeFive->setValue(sensorFiveHighValue);
    ui->smallRangeSix->setValue(sensorSixLowValue);
    ui->bigRangeSix->setValue(sensorSixHighValue);
    ui->smallRangeSeven->setValue(sensorSevenLowValue);
    ui->bigRangeSeven->setValue(sensorSevenHighValue);
}

bool rangesetting::writeParametersInSettings()
{
    if(ui->smallRangeOne->value() >= ui->bigRangeOne->value() || ui->smallRangeTwo->value() >= ui->bigRangeTwo->value()
            || ui->smallRangeThree->value() >= ui->bigRangeThree->value() || ui->smallRangeFour->value() >= ui->bigRangeFour->value()
            || ui->smallRangeFive->value() >= ui->bigRangeFive->value() || ui->smallRangeSix->value() >= ui->bigRangeSix->value()
            || ui->smallRangeSeven->value() >= ui->bigRangeSeven->value())
    {
        myHelper::ShowMessageBoxError("最小值不能大于或等于最大值！");
        return false;
    }
    myHelper::writeSettings("correctionParameters/sensorOneLowValue",ui->smallRangeOne->value());
    myHelper::writeSettings("correctionParameters/sensorOneHighValue",ui->bigRangeOne->value());
    myHelper::writeSettings("correctionParameters/sensorTwoLowValue",ui->smallRangeTwo->value());
    myHelper::writeSettings("correctionParameters/sensorTwoHighValue",ui->bigRangeTwo->value());
    myHelper::writeSettings("correctionParameters/sensorThreeLowValue",ui->smallRangeThree->value());
    myHelper::writeSettings("correctionParameters/sensorThreeHighValue",ui->bigRangeThree->value());
    myHelper::writeSettings("correctionParameters/sensorFourLowValue",ui->smallRangeFour->value());
    myHelper::writeSettings("correctionParameters/sensorFourHighValue",ui->bigRangeFour->value());
    myHelper::writeSettings("correctionParameters/sensorFiveLowValue",ui->smallRangeFive->value());
    myHelper::writeSettings("correctionParameters/sensorFiveHighValue",ui->bigRangeFive->value());
    myHelper::writeSettings("correctionParameters/sensorSixLowValue",ui->smallRangeSix->value());
    myHelper::writeSettings("correctionParameters/sensorSixHighValue",ui->bigRangeSix->value());
    myHelper::writeSettings("correctionParameters/sensorSevenLowValue",ui->smallRangeSeven->value());
    myHelper::writeSettings("correctionParameters/sensorSevenHighValue",ui->bigRangeSeven->value());
    return true;
}

void rangesetting::locked_Slot()
{
    if(isLocked)
    {
        // 解锁控件
        ui->smallRangeOne->setEnabled(true);
        ui->smallRangeTwo->setEnabled(true);
        ui->smallRangeThree->setEnabled(true);
        ui->smallRangeFour->setEnabled(true);
        ui->smallRangeFive->setEnabled(true);
        ui->smallRangeSix->setEnabled(true);
        ui->smallRangeSeven->setEnabled(true);

        ui->bigRangeOne->setEnabled(true);
        ui->bigRangeTwo->setEnabled(true);
        ui->bigRangeThree->setEnabled(true);
        ui->bigRangeFour->setEnabled(true);
        ui->bigRangeFive->setEnabled(true);
        ui->bigRangeSix->setEnabled(true);
        ui->bigRangeSeven->setEnabled(true);
        ui->Lock->setText("锁定");
        isLocked = false;
    }
    else
    {
        // 锁定控件
        ui->smallRangeOne->setEnabled(false);
        ui->smallRangeTwo->setEnabled(false);
        ui->smallRangeThree->setEnabled(false);
        ui->smallRangeFour->setEnabled(false);
        ui->smallRangeFive->setEnabled(false);
        ui->smallRangeSix->setEnabled(false);
        ui->smallRangeSeven->setEnabled(false);

        ui->bigRangeOne->setEnabled(false);
        ui->bigRangeTwo->setEnabled(false);
        ui->bigRangeThree->setEnabled(false);
        ui->bigRangeFour->setEnabled(false);
        ui->bigRangeFive->setEnabled(false);
        ui->bigRangeSix->setEnabled(false);
        ui->bigRangeSeven->setEnabled(false);
        ui->Lock->setText("解锁");

        // 更新按钮状态
        isLocked = true;
    }
}


void rangesetting::on_Reset_clicked()
{

    ui->smallRangeOne->setValue(0);
    ui->smallRangeTwo->setValue(0);
    ui->smallRangeThree->setValue(0);
    ui->smallRangeFour->setValue(0);
    ui->smallRangeFive->setValue(0);
    ui->smallRangeSix->setValue(0);
    ui->smallRangeSeven->setValue(0);

    ui->bigRangeOne->setValue(0);
    ui->bigRangeTwo->setValue(0);
    ui->bigRangeThree->setValue(0);
    ui->bigRangeFour->setValue(0);
    ui->bigRangeFive->setValue(0);
    ui->bigRangeSix->setValue(0);
    ui->bigRangeSeven->setValue(0);
}
