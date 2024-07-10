#pragma execution_character_set("utf-8")
#ifndef MYHELPER_H
#define MYHELPER_H

#include <QtCore>
#include <QDesktopWidget>
#include "frmmessagebox.h"
#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QApplication>
#include <QGuiApplication>
#include "dataoper.h"
#include "objectinfo.h"
#include <QGroupBox>
#include <QTimer>
#include <QThread>

#ifdef _MSC_VER
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#pragma warning(disable : 4819)
#endif // _MSC_VER >= 1600
#endif // _MSC_VER

#define settings_version                    "1.1"
#define settings_name                       "Settings.ini"
#define dataBase_alias                      "sqllite"        //数据库别名  mysql     sqllite
//扩展qDebug以文件行列记录信息
#define qlog(msg) qDebug() << QString("[%1][threadID_%2]:%3")                                               \
                                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz"))  \
                                        .arg(QString::number(quintptr(QThread::currentThreadId()))).arg(msg);

static QString pushButtonGreenStyle = "QPushButton{"
                                      "color: #ffffff;"
                                      "background-color: green;"
                                      "border-width: 1px;"
                                      "border-color: #183881;"
                                      "border-style: solid;"
                                      "padding: 5px;"
                                      "border-radius:5px;}";

static QString defaultPushButtonStyle = "QPushButton{"
                                        "color: #000000;"
                                        "background-color: #A49E97;"
                                        "border-width: 1px;"
                                        "border-color: #183881;"
                                        "border-style: solid;"
                                        "padding: 5px;"
                                        "border-radius:5px;}";

static QString hoverPushButtonStyle = "QPushButton:hover{background-color: #0550A4;color: #ffffff;}";
static QString disablePushButtonStyle = "QPushButton:disabled{"
                                        "background-color: #C2C46B;"
                                        "border-width: 1px;"
                                        "border-color: rgb(24, 56, 129);"
                                        "border-style: solid;"
                                        "border-radius: 5px;"
                                        "color: #000000;}";



static QString normal = "background-color: rgb(6, 141, 61);font-size:18px;color:white";
static QString fail = "background-color: rgb(255, 0, 0);font-size:18px;color:white";
static QString connecting = "background-color: rgb(239, 237, 15);font-size:18px;color:white";
static QString disconnectStyle = "background-color: rgb(239, 237, 15);font-size:18px;color:white";

static QString normal_radius = "background-color: rgb(26,250,41);font-size:18px;color:white;border-radius: 15px;";
static QString fail_radius = "background-color: #F42215;font-size:18px;color:white;border-radius: 15px;";


class myHelper: public QObject
{

public:

    //显示信息框,仅确定按钮 isAutoClose = 0 不开启自动关闭
    static void ShowMessageBoxInfo(QString info,bool isAutoClose = false, int ms = 300)
    {
        frmMessageBox *msg = new frmMessageBox;
        msg->SetMessage(info, 0);
        if(isAutoClose == true){
            msg->start_Timer(ms);
            msg->setModal(false);
            msg->show();
        }else{
            msg->exec();
        }
    }

    //显示错误框,仅确定按钮
    static void ShowMessageBoxError(QString info, bool isAutoClose = false, int ms = 3000)
    {
        frmMessageBox *msg = new frmMessageBox;
        msg->SetMessage(info, 2);
        if(isAutoClose == true){
            msg->start_Timer(ms);
            msg->setModal(false);
            msg->show();
        }else{
            msg->exec();
        }

    }

    //显示询问框,确定和取消按钮 0取消
    static int ShowMessageBoxQuesion(QString info, bool isAutoClose = false, int ms = 3000)
    {
        frmMessageBox *msg = new frmMessageBox;
        msg->SetMessage(info, 1);
        if(isAutoClose == true){
            msg->start_Timer(ms);
            return msg->exec();
        }else{
            return msg->exec();
        }
    }
//    static cv::Mat QImageToMat(const QImage &image)
//	{
//		switch(image.format())
//		{
//			//8-bit, 4 channel
//			case QImage::Format_RGB32:
//			{
//				cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), size_t(image.bytesPerLine()));
//				return mat;
//			}
//				//8-bit, 3 channel
//			case QImage::Format_RGB888:
//			{
//				QImage swapped = image.rgbSwapped();
//				cv::Mat mat(swapped.height(), swapped.width(), CV_8UC3, const_cast<uchar*>(swapped.bits()), size_t(swapped.bytesPerLine()));
//				return mat;
//			}
//				//8-bit, 1 channel
//			case QImage::Format_Indexed8:
//			{
//				cv::Mat mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), size_t(image.bytesPerLine()));
//				return mat;
//			}
//			default:
//			break;
//		}
//		return cv::Mat();
//	}
//    //cv::Mat转QImage
//	static QImage matToQImage(const cv::Mat &inMat)
//	{
//		switch (inMat.type() )
//		{
//			// 8-bit, 4 channel
//			case CV_8UC4:
//			{
//				QImage image( inMat.data,inMat.cols, inMat.rows,static_cast<int>(inMat.step),QImage::Format_ARGB32 );
//				return image;
//			}
//				// 8-bit, 3 channel
//			case CV_8UC3:
//			{
//				QImage image( inMat.data,inMat.cols, inMat.rows,static_cast<int>(inMat.step),QImage::Format_RGB888 );
//				return image.rgbSwapped();
//			}

//				// 8-bit, 1 channel
//			case CV_8UC1:
//			{
//				QImage image( inMat.data,inMat.cols, inMat.rows,static_cast<int>(inMat.step),QImage::Format_Grayscale8);
//				return image;
//			}

//			default:
//				qWarning() << "CVS::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
//			break;
//		}

//		return QImage();
//	}
    //显示询问框,确定和取消按钮  0取消 主要是用于产品提示
//    static int ShowProtipsformQuesion(DiameterM info)
//    {
//        ProTipsForm *msg = new ProTipsForm;
//        msg->SetMessage(info);
//        return msg->exec();
//    }
    //延时
    static void Sleep(int sec)
    {
        QTime dieTime = QTime::currentTime().addMSecs(sec);
        while ( QTime::currentTime() < dieTime ) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }

    //窗体居中显示
    static void FormInCenter(QWidget *frm)
    {
        int frmX = frm->width();
        int frmY = frm->height();

        QDesktopWidget* desktopWidget = QApplication::desktop();
        QRect appRect = desktopWidget->availableGeometry();//显示屏幕宽高，不包括任务栏
        int deskWidth = appRect.width();
        int deskHeight = appRect.height();
        QPoint movePoint(deskWidth / 2 - frmX / 2, deskHeight / 2 - frmY / 2);
        frm->move(movePoint);
    }

    //初始化ini配置文件
    static void initSettings()
    {
        QSettings settings(settings_name, QSettings::IniFormat);
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        if (settings.value("SettingsVersion").toString() == settings_version)
            return;

        QFileInfo fi(settings_name);
        if (fi.exists())
        {
            QDir dir;
            dir.remove(settings_name);
        }

        settings.setValue("SettingsVersion", settings_version);
        //sqllite 数据库
        settings.setValue("sqllite/name", "PreAdjustmentPlatformSystem.db");

        //Snap7协议 接口配置
        settings.setValue("GPLC/IP", "192.168.1.5");
        settings.setValue("GPLC/Rack", 0);
        settings.setValue("GPLC/Slot", 1);
        settings.setValue("GPLC/readDB", 10);
        settings.setValue("GPLC/writeDB", 9);
//        //轴运动速度设置
//        settings.setValue("SpeedSetting/Speed",2.0);
//        settings.setValue("SpeedSetting/tableSpeed",2.0);
        //相机和算法参数
//        settings.setValue("AlgorithmParams/thresholdLeft",125);
//        settings.setValue("AlgorithmParams/thresholdRight",120);
//        settings.setValue("AlgorithmParams/blurLeft",10);
//        settings.setValue("AlgorithmParams/blurRight",10);
//        settings.setValue("CameraParams/AFRLeft",1);
//        settings.setValue("CameraParams/AFRRight",1);
//        settings.setValue("CameraParams/leftCameraID","K49476147");
//        settings.setValue("CameraParams/rightCameraID","K49476170");

    }


    //读取ini配置文件
    static QVariant readSettings(const QString key,QString s_name = settings_name)
    {
        QSettings settings(s_name, QSettings::IniFormat);
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        return settings.value(key);
    }

    //写入ini配置文件
    static void writeSettings(const QString key, QVariant val, QString s_name = settings_name)
    {
        QSettings settings(s_name, QSettings::IniFormat);
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        settings.setValue(key, val);
    }
    //更改按钮样式函数
    static void pushButtonStyleChange(QString content, QPushButton *btn, bool isEnable, bool isOriginalStatus)
    {
        if(isOriginalStatus == true){
            btn->setStyleSheet(defaultPushButtonStyle);
            btn->setStyleSheet(hoverPushButtonStyle);
            btn->setStyleSheet(disablePushButtonStyle);
        }else{
            btn->setStyleSheet(pushButtonGreenStyle);
        }
        btn->setText(content);

        btn->setEnabled(isEnable);
    }

};

#endif // MYHELPER_H
