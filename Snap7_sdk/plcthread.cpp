#include "plcthread.h"

PLCThread::PLCThread(QObject *parent)
    : QObject{parent}
{

}


void PLCThread::ConnectPLC(QString ip, int Rack, int Slot)
{
    PLC_Siemens* snap7_plc = new PLC_Siemens();
    snap7_plc->setConnectType(CONNTYPE_OP);//设置连接方式，避免变成软件连不上PLC了，必须在连接之前设置
    int ok = snap7_plc->connectByIP(ip, Rack, Slot);
    emit ConnectPLCStatus(snap7_plc, ok);
}

////连接OPT光源控制器
//void PLCThread::ConnectOPTLight(QString ip_str)
//{
//	//连接OPT，一个控制器只能被一个上位机连接
//	OPTController_Handle *m_OPTControllerHanlde = new OPTController_Handle();
//	QByteArray ip_byte = ip_str.toLocal8Bit();
//	char* ip = ip_byte.data();
//	long lRet = OPTController_CreateEthernetConnectionByIP(ip, m_OPTControllerHanlde);
//	emit ConnectOPTLightStatus(m_OPTControllerHanlde, lRet);
//}
