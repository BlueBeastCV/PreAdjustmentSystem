#ifndef DATAOPER_H
#define DATAOPER_H

#include <QObject>
#include <vector>
#include "objectinfo.h"
#include <QSqlDatabase>
#include <QtSql>
#include "myhelper.h"
using namespace std;


/**
 * @brief The DataOper class
 * 数据处理类   主要操作数据库。
 */
class DataOper : public QObject
{
        Q_OBJECT
    public:
        explicit DataOper(QObject *parent = nullptr);
        bool releaseDBspace();
        QString getUUID();
        bool isInteger(const QVariant& variant);
        bool isString(const QVariant& variant);
        int deleteTableData(QString oldTable, QMap<QString, QVariant> fieldMap);


        int getUserNameList(QStringList& userNameList);
        int queryAllUser(QVector<User>& users);
        int saveUsers(QVector<User> users);
        int deleteUser(QString name = nullptr);
        int getUserByName(User& user, QString name);
        int saveProductionInfo(QVector<ProductionInfo>& paramtersInfo);
        int saveDetectResult(QVector<ResultSet>& resultSet);
        int getBatchNumListByTime(QStringList &batchNumList, QString startTime,QString endTime);
        int getDetectDataByBatchNum(QVector<ResultSet> &resultSet,const QString typeNum,
                                    const QString &startTime, const QString &endTime);
    signals:
        void sendDataMessage(const QString& info);

    private:
        QSqlDatabase db;
};

#endif // DATAOPER_H
