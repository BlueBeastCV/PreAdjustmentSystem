#include "limitform.h"
#include "ui_limitform.h"

LimitForm::LimitForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LimitForm)
{
    ui->setupUi(this);

    myHelper::FormInCenter(this);//窗体居中显示
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    ui->titleWidget->set_lab_Title("密码输入");
    ui->titleWidget->setShowMaxBtn(false);
    ui->titleWidget->setShowMinBtn(false);
    setLineEditValidator(ui->lineEdit_3);
    connect(ui->modifyPwd,&QPushButton::clicked,this,[=](){
        if(modifyPWD() == true)
        {
            myHelper::ShowMessageBoxInfo("修改成功！");
            ui->lineEdit_3->clear();
        }
        else
        {
            myHelper::ShowMessageBoxError("修改失败！");
        }
    });
}

LimitForm::~LimitForm()
{
    delete ui;
}
//设置QLineEdit只能输入数字和字母
void LimitForm::setLineEditValidator(QLineEdit *edit)
{
    // 定义一个正则表达式，只允许字母和数字
    QRegExp regex("[a-zA-Z0-9]*");
    // 创建一个QRegExpValidator对象，并应用正则表达式
    QRegExpValidator *validator = new QRegExpValidator(regex, edit);
    edit->setValidator(validator);
}

bool LimitForm::modifyPWD()
{
    if(ui->lineEdit_3->text().isEmpty())
    {
        myHelper::ShowMessageBoxError("密码不能为空,请重新输入!");
        return false;
    }
    else
    {
        myHelper::writeSettings("parameter/pwd",ui->lineEdit_3->text());
        return true;
    }
}

void LimitForm::on_pushButton_clicked()
{
    QString pwd = ui->lineEdit->text();
    QString p = myHelper::readSettings("parameter/pwd").toString();
    if(pwd == p){
        emit hideCurrentWindow();
        this->close();
    }else{
        myHelper::ShowMessageBoxError("密码错误");
    }
}

