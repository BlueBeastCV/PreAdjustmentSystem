#include "dataoper.h"

DataOper::DataOper(QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::database(dataBase_alias);//连接数据库2
}


//释放数据库删除文件占用的空间。清空表数据，并不会改变db文件大小。索要要执行次命令
bool DataOper::releaseDBspace()
{
    QSqlQuery query(db);
    query.prepare("VACUUM");
    if (query.exec()) {
        qDebug() <<  "releaseDBspace success.";
        return true;
    }else{
        return false;
        qDebug() <<  "Can't compress database.";
    }
}
//生成UUID
QString DataOper::getUUID()
{
    QUuid id = QUuid::createUuid();
    QString strID = id.toString();
    strID.remove("{").remove("}").remove("-");
    return strID;
}
//快速删除大量数据  fieldMap 需要删除的数据
int DataOper::deleteTableData(QString oldTable, QMap<QString, QVariant> fieldMap)
{
    QSqlQuery query(db);
    db.transaction();
    //创建临时表存储不需要删除的数据
    QString sql = QString("CREATE TABLE %1temp AS SELECT * FROM %2 where  1=1 ").arg(oldTable).arg(oldTable);
    QMap<QString, QVariant>::const_iterator it = fieldMap.constBegin();
    while (it != fieldMap.constEnd())
    {
        qDebug() << it.key() << ": " << it.value() << endl;
        if(isInteger(it.value())){//QVariant 是整数
            sql.append( QString("and %1 != %2 ").arg(it.key()).arg(it.value().toInt()) );
        }else{
            sql.append( QString("and %1 != '%2' ").arg(it.key()).arg(it.value().toString()) );
        }
        ++it;
    }
    if(query.exec(sql)){
        db.commit();
        //删除旧表
        sql = QString("DROP TABLE %1").arg(oldTable);
        if(query.exec(sql)){
            db.commit();
            //更改临时表到新表
            sql = QString("ALTER TABLE %1temp RENAME TO %2").arg(oldTable).arg(oldTable);
            if(query.exec(sql)){
                db.commit();
                return 0;
            }else{
                qDebug() << "更改临时表到新表 fail:" << query.lastError();
                return 2;
            }
        }else{
            qDebug() << "删除旧表 fail:" << query.lastError();
            return 2;
        }
    }else{
        qDebug() << "创建临时表存储不需要删除的数据 fail:" << query.lastError();
        return 2;
    }
}

//批量删除大量数据 delete 很慢，可以尝试以下方式
//-- 复制表结构并复制需要保留数据
//CREATE TABLE logE AS SELECT * FROM log where name = 'admin';
//-- 删除旧表
//DROP TABLE log;
//-- 重命名
//ALTER TABLE logE RENAME TO log

//判断QVariant 是否是整型
bool DataOper::isInteger(const QVariant &variant)
{
    switch (variant.userType())
    {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return true;
    }
    return false;
}
//判断QVariant 是否是字符串
bool DataOper::isString(const QVariant &variant)
{
    return variant.userType() == QMetaType::QString;
}


//获取用户列表
int DataOper::getUserNameList(QStringList &userNameList)
{
    QSqlQuery query(db);
    QString sql  = QString("select name from user");

    if(query.exec(sql)){
        while(query.next()){
            userNameList.append(query.value("name").toString());
        }
        return 0;
    }else{
        qDebug() << "getUserNameList fail: " + query.lastError().text();
        return 2;
    }
}
//查询所有用户
int DataOper::queryAllUser(QVector<User>& users)
{
    User  user;
    QSqlQuery q(db);
    QString sql = "select name,pwd,role,is_used from user ";

    if(q.exec(sql)){
        while(q.next()){
            user.name = q.value("name").toString();
            user.pwd = q.value("pwd").toString();
            user.role = q.value("role").toString();
            user.is_used = q.value("is_used").toString();
            users.push_back(user);
        }
        return 0;
    }else{
        qDebug() << "queryAllUser fail: " + q.lastError().text();
        return 2;
    }
}
//保存用户
int DataOper::saveUsers(QVector<User> users)
{
    //删除现有的全部用户
    deleteUser();
    QSqlQuery q(db);
    q.prepare("insert into user values (?, ?, ?, ?)");
    QVariantList names;
    QVariantList pwds;
    QVariantList roles;
    QVariantList is_useds;
    for(int i = 0; i < users.size(); i++){
        names << users[i].name;
        pwds << users[i].pwd;
        roles << users[i].role;
        is_useds << users[i].is_used;
    }
    q.addBindValue(names);
    q.addBindValue(pwds);
    q.addBindValue(roles);
    q.addBindValue(is_useds);
    if (q.execBatch()){
        return 0;
    }else{
        qDebug() << "saveUsers fail: " + q.lastError().text();
        return 2;
    }
}
//删除用户
int DataOper::deleteUser(QString name)
{
    QSqlQuery q(db);
    QString sql = nullptr;
    if(name == nullptr){
        sql = QString("delete from user");
    }else{
        sql = QString("delete from user where name = '%1'").arg(name);
    }
    if(q.exec(sql)){
        return 0;
    }else{
        qDebug() << "deleteUserByName fail: " + q.lastError().text();
        return 2;
    }
}
//根据名字获取用户信息
int DataOper::getUserByName(User &user, QString name)
{
    QSqlQuery query(db);
    QString sql  = QString("select name, pwd, role, is_used from user where name = '%1'").arg(name);

    if(query.exec(sql)){
        if(query.next()){
            user.name = query.value("name").toString();
            user.pwd = query.value("pwd").toString();
            user.role = query.value("role").toString();
            user.is_used = query.value("is_used").toString();
            return 0;
        }else{
            return 1;
        }
    }else{
        qDebug() << "getUserByName fail: " + query.lastError().text();
        return 2;
    }
}

int DataOper::saveProductionInfo(QVector<ProductionInfo> &paramtersInfo)
{

    QSqlQuery q(db);
    q.prepare("insert into productInfo(batchNum, typeNum, processNum, componentNum, dates) values (?,?,?,?,?)");
    QVariantList batchNums;
    QVariantList typeNums;
    QVariantList processNums;
    QVariantList componentNums;
    QVariantList date;

    for(int i=0;i<paramtersInfo.size();i++)
    {
        batchNums << paramtersInfo[i].batchNum;
        typeNums << paramtersInfo[i].typeNum;
        processNums << paramtersInfo[i].processNum;
        componentNums << paramtersInfo[i].componentNum;
        date << paramtersInfo[i].dates;
    }
    q.addBindValue(batchNums);
    q.addBindValue(typeNums);
    q.addBindValue(processNums);
    q.addBindValue(componentNums);
    q.addBindValue(date);
    if (q.execBatch()){
        db.commit();
        return 0;
    }else{
        qDebug() << "saveProductionInfo failed: " + q.lastError().text();
        return -1;
    }

}

int DataOper::saveDetectResult(QVector<ResultSet> &resultSet)
{
    QSqlQuery q(db);
    q.prepare("insert into resultTable(batchNum, typeNum, processNum, componentNum,resultValueOne,resultValueTwo,resultValueThree,resultValueFour,resultValueFive,resultValueSix,resultValueSeven,result, dateTime) "
              "values (?,?,?,?,?,?,?,?,?,?,?,?,?)");
    QVariantList batchNums;
    QVariantList typeNums;
    QVariantList processNums;
    QVariantList componentNums;
    QVariantList resultValueOnes;
    QVariantList resultValueTwos;
    QVariantList resultValueThrees;
    QVariantList resultValueFours;
    QVariantList resultValueFives;
    QVariantList resultValueSixs;
    QVariantList resultValueSevens;
    QVariantList results;
    QVariantList dateTimes;
    for(int i=0;i<resultSet.size();i++)
    {
        batchNums << resultSet[i].batchNum;
        typeNums << resultSet[i].typeNum;
        processNums << resultSet[i].processNum;
        componentNums << resultSet[i].componentNum;
        resultValueOnes << resultSet[i].valueOne;
        resultValueTwos << resultSet[i].valueTwo;
        resultValueThrees << resultSet[i].valueThree;
        resultValueFours << resultSet[i].valueFour;
        resultValueFives << resultSet[i].valueFive;
        resultValueSixs << resultSet[i].valueSix;
        resultValueSevens << resultSet[i].valueSeven;
        results << resultSet[i].result;
        dateTimes << resultSet[i].dateTime;
    }
    q.addBindValue(batchNums);
    q.addBindValue(typeNums);
    q.addBindValue(processNums);
    q.addBindValue(componentNums);
    q.addBindValue(resultValueOnes);
    q.addBindValue(resultValueTwos);
    q.addBindValue(resultValueThrees);
    q.addBindValue(resultValueFours);
    q.addBindValue(resultValueFives);
    q.addBindValue(resultValueSixs);
    q.addBindValue(resultValueSevens);
    q.addBindValue(results);
    q.addBindValue(dateTimes);
    if (q.execBatch()){
        db.commit();
        return 0;
    }else{
        qDebug() << "saveDetectResult failed: " + q.lastError().text();
        return -1;
    }


}

int DataOper::getBatchNumListByTime(QStringList &batchNumList, QString startTime, QString endTime)
{
    QSqlQuery q(db);
    QString sql = QString("select distinct batchNum from productInfo where dates >='%1' and dates <= '%2' order by batchNum asc ")
            .arg(startTime).arg(endTime);
    if(q.exec(sql))
    {
        while(q.next())
        {
            batchNumList.append(q.value("batchNum").toString());
        }
        return 0;
    }
    else
    {
        qDebug() << "getBatchNumList failed: " + q.lastError().text();
        return -1;
    }
}

int DataOper::getDetectDataByBatchNum(QVector<ResultSet> &resultSet, const QString typeNum, const QString &startTime, const QString &endTime)
{
    QSqlQuery q(db);
    QString sql = QString("SELECT * FROM resultTable WHERE batchNum = '%1' AND dateTime BETWEEN '%2' AND '%3'")
            .arg(typeNum).arg(startTime + "00:00:00").arg(endTime + "23:59:59");
    if(q.exec(sql))
    {
        while(q.next())
        {
            ResultSet results;
            results.batchNum = q.value("batchNum").toString();
            results.typeNum = q.value("typeNum").toString();
            results.processNum = q.value("processNum").toInt();
            results.componentNum = q.value("componentNum").toString();
            results.valueOne = q.value("resultValueOne").toDouble();
            results.valueTwo = q.value("resultValueTwo").toDouble();
            results.valueThree = q.value("resultValueThree").toDouble();
            results.valueFour = q.value("resultValueFour").toDouble();
            results.valueFive = q.value("resultValueFive").toDouble();
            results.valueSix = q.value("resultValueSix").toDouble();
            results.valueSeven = q.value("resultValueSeven").toDouble();
            results.result = q.value("result").toString();
            results.dateTime = q.value("dateTime").toString();
            resultSet.push_back(results);
        }
        return 0;
    }
    else
    {
        qDebug() << "getDetectDataByBatchNum fail:" + q.lastError().text();
        return -1;
    }
}

//int DataOper::getHardnessListByID(QVector<hardnessList> &hadnessDataList, const QString id)
//{
//    QSqlQuery query(db);
//    QString sql  = QString("select plateType, pointCode, xCoordinate, yCoordinate, hardness, diameter, result, time, operator from hardnessList where plateType='%1' ")
//            .arg(id);
//    if(query.exec(sql)){
//        while(query.next()){
//            hardnessList hardnesses;
//            hardnesses.plateType = query.value("plateType").toString();
//            hardnesses.id = query.value("pointCode").toString();
//            hardnesses.xCoordinate = query.value("xCoordinate").toInt();
//            hardnesses.yCoordinate = query.value("yCoordinate").toInt();
//            hardnesses.hardness = query.value("hardness").toInt();
//            hardnesses.diameter = query.value("diameter").toDouble();
//            hardnesses.time = query.value("time").toString();
//            hardnesses.oper = query.value("operator").toString();
//            hardnesses.result = query.value("result").toString();
//            hadnessDataList.push_back(hardnesses);
//        }
//        return 0;
//    }else{
//        qDebug() << "getHardnessListByID fail:" + query.lastError().text();
//        return 2;
//    }
//}








