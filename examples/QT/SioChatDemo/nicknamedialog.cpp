#include "nicknamedialog.h"
#include "ui_nicknamedialog.h"
#include <QTimer>
NicknameDialog::NicknameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NicknameDialog)
{
    ui->setupUi(this);
}

NicknameDialog::~NicknameDialog()
{
    delete ui;
}

const QString& NicknameDialog::getNickname() const
{
    return m_nickName;
}

void NicknameDialog::exitApp()
{
    QApplication::quit();
}

void NicknameDialog::closeEvent(QCloseEvent *event)
{
    QTimer::singleShot(0,this,SLOT(exitApp()));
}

void NicknameDialog::accept()
{
    if(this->findChild<QLineEdit*>("nicknameEdit")->text().length()>0)
    {
        m_nickName = this->findChild<QLineEdit*>("nicknameEdit")->text();
        done(QDialog::Accepted);
    }
}
