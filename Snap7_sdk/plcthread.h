#ifndef PLCTHREAD_H
#define PLCTHREAD_H

#include <QObject>
#include "plc_siemens.h"
//#include "OPTController.h"

class PLCThread : public QObject
{
        Q_OBJECT
    public:
        explicit PLCThread(QObject *parent = nullptr);

        void ConnectPLC(QString ip, int Rack, int Slot);
//		void ConnectOPTLight(QString ip_str);
    signals:
        void ConnectPLCStatus(PLC_Siemens*, int);
//		void ConnectOPTLightStatus(OPTController_Handle*, long);

};

#endif // PLCTHREAD_H
