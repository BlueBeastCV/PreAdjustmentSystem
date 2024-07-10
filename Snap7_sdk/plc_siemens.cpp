#include "plc_siemens.h"



PLC_Siemens::PLC_Siemens(QObject* parent):
    QObject(parent)
{
    MyS7Client = new TS7Client();
}

PLC_Siemens::~PLC_Siemens()
{
    if(MyS7Client){
        delete MyS7Client;
        MyS7Client = nullptr;
    }
}


int PLC_Siemens::connectByIP(QString ip, int Rack, int Slot)
{
    if ( !MyS7Client->Connected() ) {
        return MyS7Client->ConnectTo(ip.toStdString().c_str() ,Rack,Slot);
    } else {
        return MyS7Client->Connected();
    }
}

bool PLC_Siemens::connectStatus()
{
    return MyS7Client->Connected();
}

int PLC_Siemens::disConnect()
{
    return MyS7Client->Disconnect();
}

int PLC_Siemens::DBRead(int DBNumber, int Start, int Size, void *pUsrData, QString writeContent)
{
    int res = MyS7Client->DBRead(DBNumber, Start, Size, pUsrData);
    if(writeContent.trimmed().isEmpty() == false){
//        qlog(QString("DBRead %1: %2").arg(writeContent).arg(ErrorText(res)));
    }
    return res;
}

int PLC_Siemens::DBWrite(int DBNumber, int Start, int Size, void *pUsrData, QString writeContent)
{
    int res = MyS7Client->DBWrite(DBNumber, Start, Size, pUsrData);
    if(writeContent.trimmed().isEmpty() == false){
        if(Size == 2){//int
            qlog(QString("DBWrite %1: %2  val:%3").arg(writeContent).arg(ErrorText(res)).arg(S7_GetIntAt((byte*)pUsrData, 0)));
        }else if(Size == 4){//real
            qlog(QString("DBWrite %1: %2  val:%3").arg(writeContent).arg(ErrorText(res)).arg(S7_GetRealAt((byte*)pUsrData, 0)));
        }else{
            qlog(QString("DBWrite %1: %2").arg(writeContent).arg(ErrorText(res)));
        }
    }
    return res;
}
//发送位信号
int PLC_Siemens::sendBitToRobot(int offset, int bit, bool on_off, int DBNum, QString writeContent)
{
    byte bitBool[1];
    int res = DBRead(DBNum, offset, 1, &bitBool, "");
    for(int i = 0; i < 8; i++){
        if(i == bit){
            S7_SetBitAt(bitBool, 0, i,  on_off);
        }else{
            S7_SetBitAt(bitBool, 0, i,  S7_GetBitAt(bitBool, 0, i));
        }
    }
    res = DBWrite(DBNum, offset, 1, &bitBool, "");
    qlog(QString("sendBitToRobot %1: %2 --- %3").arg(writeContent).arg(ErrorText(res)).arg(on_off == true ? "true" : "false"));
    return res;
}
int PLC_Siemens::ReadArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData, QString writeContent)
{
   int res = MyS7Client->ReadArea(Area, DBNumber, Start, Amount, WordLen, pUsrData);
   qlog(QString("ReadArea %1: %2").arg(writeContent).arg(ErrorText(res)));
   return res;
}

int PLC_Siemens::WriteArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData, QString writeContent)
{
    int res = MyS7Client->WriteArea(Area, DBNumber, Start, Amount, WordLen, pUsrData);
    qlog(QString("WriteArea %1: %2").arg(writeContent).arg(ErrorText(res)));
    return res;
}

QString PLC_Siemens::ErrorText(int Error)
{
    return QString::fromLocal8Bit(MyS7Client->ErrorText(Error).c_str());
}

int PLC_Siemens::ABRead(int Start, int Size, void *pUsrData)
{
    return MyS7Client->ABRead(Start, Size, pUsrData);
}

int PLC_Siemens::ABWrite(int Start, int Size, void *pUsrData)
{
    return MyS7Client->ABWrite(Start, Size, pUsrData);
}


int PLC_Siemens::setConnectType(word ConnectionType)
{
    //之前出现该界面连接上PLC后，200smart的step7编程界面就连不上PLC了。或者200smart的step7编程界面连接上了，该界面就连接不上PLC。
    //原因是，使用snap7连接plc可能默认连接类型会设为PG连接类型，这个类型是Step7编程软件与PLC的连接类型，而且该连接类型就只提供一个连接。
    //因此我们用snap7连接plc时，要更改其连接类型，选择OP连接类型，详细可见snap7官方文档，现在我po官方api
    //PG 0X01    OP 0X02
    return MyS7Client->SetConnectionType(ConnectionType);
}

