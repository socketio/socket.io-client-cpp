#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <functional>
#include <mutex>
#include <cstdlib>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _io(new client())
{
    ui->setupUi(this);
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    _io->bind_event("new message",std::bind(&MainWindow::OnNewMessage,this,_1,_2,_3,_4));
    _io->bind_event("user joined",std::bind(&MainWindow::OnUserJoined,this,_1,_2,_3,_4));
    _io->bind_event("user left",std::bind(&MainWindow::OnUserLeft,this,_1,_2,_3,_4));
    _io->bind_event("typing",std::bind(&MainWindow::OnTyping,this,_1,_2,_3,_4));
    _io->bind_event("stop typing",std::bind(&MainWindow::OnStopTyping,this,_1,_2,_3,_4));
    _io->bind_event("login",std::bind(&MainWindow::OnLogin,this,_1,_2,_3,_4));
    _io->set_connect_listener(std::bind(&MainWindow::OnConnected,this));
    _io->set_fail_listener(std::bind(&MainWindow::OnFailed,this));
    _io->set_close_listener(std::bind(&MainWindow::OnClosed,this,_1));

    connect(this,SIGNAL(RequestAddListItem(QListWidgetItem*)),this,SLOT(AddListItem(QListWidgetItem*)));
    connect(this,SIGNAL(RequestToggleInputs(bool)),this,SLOT(ToggleInputs(bool)));
}

MainWindow::~MainWindow()
{
    _io->clear_event_bindings();
    _io->clear_con_listeners();
    delete ui;
}

void MainWindow::SendBtnClicked()
{
    QLineEdit* messageEdit = this->findChild<QLineEdit*>("messageEdit");
    QString text = messageEdit->text();
    if(text.length()>0)
    {
        QByteArray bytes = text.toUtf8();
        std::string msg(bytes.data(),bytes.length());
        _io->emit("new message",msg);
        text.append(":You");
        QListWidgetItem *item = new QListWidgetItem(text);
        item->setTextAlignment(Qt::AlignRight);
        Q_EMIT RequestAddListItem(item);
        messageEdit->clear();
    }
}

void MainWindow::OnMessageReturn()
{
    this->SendBtnClicked();
}

void MainWindow::showEvent(QShowEvent *event)
{

}

void MainWindow::LoginClicked()
{
    QString str = this->findChild<QLineEdit*>("nickNameEdit")->text();
    if(str.length()>0)
    {
        _io->connect("ws://localhost:3000");
        m_name = str;
    }
}

void MainWindow::TypingStop()
{
    _timer.reset();
    _io->emit("stop typing","");
}

void MainWindow::TypingChanged()
{
    if(_timer&&_timer->isActive())
    {
        _timer->stop();
    }
    else
    {
        _io->emit("typing","");
    }
    _timer.reset(new QTimer(this));
    connect(_timer.get(),SIGNAL(timeout()),this,SLOT(TypingStop()));
    _timer->setSingleShot(true);
    _timer->start(1000);
}

void MainWindow::AddListItem(QListWidgetItem* item)
{
    this->findChild<QListWidget*>("listView")->addItem(item);
}

void MainWindow::OnNewMessage(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp)
{

    if(data->get_flag() == message::flag_object)
    {
        std::string msg = data->get_map()["message"]->get_string();
        std::string name = data->get_map()["username"]->get_string();
        QString label = QString::fromUtf8(name.data(),name.length());
        label.append(':');
        label.append(QString::fromUtf8(msg.data(),msg.length()));
        QListWidgetItem *item= new QListWidgetItem(label);
        Q_EMIT RequestAddListItem(item);
    }
}

void MainWindow::OnUserJoined(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp)
{
    if(data->get_flag() == message::flag_object)
    {
        std::string name = data->get_map()["username"]->get_string();
        int numUser = data->get_map()["numUsers"]->get_int();
        QString label = QString::fromUtf8(name.data(),name.length());
        bool plural = numUser != 1;
        label.append(" joined\n");
        label.append(plural?"there are ":"there's ");
        QString digits;
        while(numUser>=10)
        {
            digits.insert(0,QChar((numUser%10)+'0'));
            numUser/=10;
        }
        digits.insert(0,QChar(numUser+'0'));
        label.append(digits);
        label.append(plural?" participants":"participant");
        QListWidgetItem *item= new QListWidgetItem(label);
        item->setTextAlignment(Qt::AlignHCenter);
        QFont font;
        font.setPointSize(9);
        item->setFont(font);
        Q_EMIT RequestAddListItem(item);
    }

}

void MainWindow::OnUserLeft(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp)
{
    if(data->get_flag() == message::flag_object)
    {
        std::string name = data->get_map()["username"]->get_string();
        int numUser = data->get_map()["numUsers"]->get_int();
        QString label = QString::fromUtf8(name.data(),name.length());
        bool plural = numUser != 1;
        label.append(" left\n");
        label.append(plural?"there are ":"there's ");
        QString digits;
        while(numUser>=10)
        {
            digits.insert(0,QChar((numUser%10)+'0'));
            numUser/=10;
        }
        digits.insert(0,QChar(numUser+'0'));
        label.append(digits);
        label.append(plural?" participants":"participant");
        QListWidgetItem *item= new QListWidgetItem(label);
        item->setTextAlignment(Qt::AlignHCenter);
        QFont font;
        font.setPointSize(9);
        item->setFont(font);
        Q_EMIT RequestAddListItem(item);
    }
}

void MainWindow::OnTyping(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp)
{
//Not implemented
}

void MainWindow::OnStopTyping(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp)
{
//Not implemented
}

void MainWindow::OnLogin(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp)
{
    Q_EMIT RequestToggleInputs(true);
    int numUser = data->get_map()["numUsers"]->get_int();

    QString digits;
    bool plural = numUser !=1;
    while(numUser>=10)
    {
        digits.insert(0,QChar((numUser%10)+'0'));
        numUser/=10;
    }

    digits.insert(0,QChar(numUser+'0'));
    digits.insert(0,plural?"there are ":"there's ");
    digits.append(plural? " participants":" participant");
    QListWidgetItem *item = new QListWidgetItem(digits);
    item->setTextAlignment(Qt::AlignHCenter);
    QFont font;
    font.setPointSize(9);
    item->setFont(font);
    Q_EMIT RequestAddListItem(item);
}

void MainWindow::OnConnected()
{
    QByteArray bytes = m_name.toUtf8();
    std::string nickName(bytes.data(),bytes.length());
    _io->emit("add user", nickName);
}

void MainWindow::OnClosed(client::close_reason const& reason)
{
    Q_EMIT RequestToggleInputs(false);
}

void MainWindow::OnFailed()
{
    Q_EMIT RequestToggleInputs(false);
}

void MainWindow::ToggleInputs(bool loginOrNot)
{
    if(loginOrNot)//already login
    {
        this->findChild<QWidget*>("messageEdit")->setEnabled(true);
        this->findChild<QWidget*>("listView")->setEnabled(true);
        this->findChild<QWidget*>("sendBtn")->setEnabled(true);
        this->findChild<QWidget*>("nickNameEdit")->setEnabled(false);
        this->findChild<QWidget*>("loginBtn")->setEnabled(false);
    }
    else
    {
        this->findChild<QWidget*>("messageEdit")->setEnabled(false);
        this->findChild<QWidget*>("listView")->setEnabled(false);
        this->findChild<QWidget*>("sendBtn")->setEnabled(false);
        this->findChild<QWidget*>("nickNameEdit")->setEnabled(true);
        this->findChild<QWidget*>("loginBtn")->setEnabled(true);
    }
}
