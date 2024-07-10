#ifndef RANGESETTING_H
#define RANGESETTING_H

#include <QWidget>
#include <objectinfo.h>
#include <myhelper.h>
namespace Ui {
class rangesetting;
}

class rangesetting : public QWidget
{
    Q_OBJECT

public:
    explicit rangesetting(User u,QWidget *parent = nullptr);
    ~rangesetting();
    void readParametersFromSettings();
    bool writeParametersInSettings();
private slots:
    void locked_Slot();

    void on_Reset_clicked();

private:
    Ui::rangesetting *ui;
    bool isLocked;

};

#endif // RANGESETTING_H
