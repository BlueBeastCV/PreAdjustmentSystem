#ifndef LIMITFORM_H
#define LIMITFORM_H

#include <QWidget>
#include "myhelper.h"
#include <objectinfo.h>
#include <QRegExpValidator>
namespace Ui {
class LimitForm;
}

class LimitForm : public QWidget
{
        Q_OBJECT

    public:
        explicit LimitForm(QWidget *parent = nullptr);
        ~LimitForm();
        void setLineEditValidator(QLineEdit *edit);
        bool modifyPWD();
private slots:
        void on_pushButton_clicked();


signals:

        void hideCurrentWindow();

    private:
        Ui::LimitForm *ui;
};

#endif // LIMITFORM_H
