#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <functional>
#include <mutex>
#include <cstdlib>

#define kURL "ws://localhost:3000"
#ifdef WIN32
#define BIND_EVENT(IO,EV,FN) \
    do{ \
        socket::event_listener_aux l = FN;\
        IO->on(EV,l);\
    } while(0)

#else
#define BIND_EVENT(IO,EV,FN) \
    IO->on(EV,FN)
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _io(new client()),
    m_typingItem(NULL),
    m_dialog()
{
    ui->setupUi(this);
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    socket::ptr sock = _io->socket();
    BIND_EVENT(sock,"new message",std::bind(&MainWindow::OnNewMessage,this,_1,_2,_3,_4));
    BIND_EVENT(sock,"user joined",std::bind(&MainWindow::OnUserJoined,this,_1,_2,_3,_4));
    BIND_EVENT(sock,"user left",std::bind(&MainWindow::OnUserLeft,this,_1,_2,_3,_4));
    BIND_EVENT(sock,"typing",std::bind(&MainWindow::OnTyping,this,_1,_2,_3,_4));
    BIND_EVENT(sock,"stop typing",std::bind(&MainWindow::OnStopTyping,this,_1,_2,_3,_4));
    BIND_EVENT(sock,"login",std::bind(&MainWindow::OnLogin,this,_1,_2,_3,_4));
    _io->set_socket_open_listener(std::bind(&MainWindow::OnConnected,this,std::placeholders::_1));
    _io->set_close_listener(std::bind(&MainWindow::OnClosed,this,_1));
    _io->set_fail_listener(std::bind(&MainWindow::OnFailed,this));

    connect(this,SIGNAL(RequestAddListItem(QListWidgetItem*)),this,SLOT(AddListItem(QListWidgetItem*)));
    connect(this,SIGNAL(RequestRemoveListItem(QListWidgetItem*)),this,SLOT(RemoveListItem(QListWidgetItem*)));
    connect(this,SIGNAL(RequestToggleInputs(bool)),this,SLOT(ToggleInputs(bool)));
}

MainWindow::~MainWindow()
{
    _io->socket()->off_all();
    _io->socket()->off_error();
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
        _io->socket()->emit("new message",msg);
        text.append(" : You");
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

void MainWindow::ShowLoginDialog()
{
    m_dialog.reset(new NicknameDialog(this));
    connect(m_dialog.get(),SIGNAL(accepted()),this,SLOT(NicknameAccept()));
    connect(m_dialog.get(),SIGNAL(rejected()),this,SLOT(NicknameCancelled()));
    m_dialog->exec();
}

void MainWindow::showEvent(QShowEvent *event)
{
    ShowLoginDialog();
}


void MainWindow::TypingStop()
{
    m_timer.reset();
    _io->socket()->emit("stop typing");
}

void MainWindow::TypingChanged()
{
    if(m_timer&&m_timer->isActive())
    {
        m_timer->stop();
    }
    else
    {
        _io->socket()->emit("typing");
    }
    m_timer.reset(new QTimer(this));
    connect(m_timer.get(),SIGNAL(timeout()),this,SLOT(TypingStop()));
    m_timer->setSingleShot(true);
    m_timer->start(1000);
}

void MainWindow::NicknameAccept()
{
    m_name = m_dialog->getNickname();
    if(m_name.length()>0)
    {
        _io->connect(kURL);
    }
}

void MainWindow::NicknameCancelled()
{
    QApplication::exit();
}

void MainWindow::AddListItem(QListWidgetItem* item)
{
    this->findChild<QListWidget*>("listView")->addItem(item);
}

void MainWindow::RemoveListItem(QListWidgetItem* item)
{
    QListWidget* list = this->findChild<QListWidget*>("listView");
    int row = list->row(item);
    delete list->takeItem(row);
}


void MainWindow::OnNewMessage(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp)
{

    if(data->get_flag() == message::flag_object)
    {
        std::string msg = data->get_map()["message"]->get_string();
        std::string username = data->get_map()["username"]->get_string();
        QString label = QString::fromUtf8(username.data(),username.length());
        label.append(" : ");
        label.append(QString::fromUtf8(msg.data(),msg.length()));
        QListWidgetItem *item= new QListWidgetItem(label);
        Q_EMIT RequestAddListItem(item);
    }
}

void MainWindow::OnUserJoined(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp)
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
        label.append(plural?" participants":" participant");
        QListWidgetItem *item= new QListWidgetItem(label);
        item->setTextAlignment(Qt::AlignHCenter);
        QFont font;
        font.setPointSize(9);
        item->setFont(font);
        Q_EMIT RequestAddListItem(item);
    }

}

void MainWindow::OnUserLeft(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp)
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
        label.append(plural?" participants":" participant");
        QListWidgetItem *item= new QListWidgetItem(label);
        item->setTextAlignment(Qt::AlignHCenter);
        QFont font;
        font.setPointSize(9);
        item->setFont(font);
        Q_EMIT RequestAddListItem(item);
    }
}

void MainWindow::OnTyping(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp)
{
    if(m_typingItem == NULL)
    {
        std::string name = data->get_map()["username"]->get_string();
        QString label = QString::fromUtf8(name.data(),name.length());
        label.append("  is typing...");
        QListWidgetItem *item = new QListWidgetItem(label);
        item->setTextColor(QColor(200,200,200,255));
        m_typingItem = item;
        Q_EMIT RequestAddListItem(item);
    }
}

void MainWindow::OnStopTyping(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp)
{
    if(m_typingItem != NULL)
    {
        Q_EMIT RequestRemoveListItem(m_typingItem);
        m_typingItem = NULL;
    }
}

void MainWindow::OnLogin(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp)
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

void MainWindow::OnConnected(std::string const& nsp)
{
    QByteArray bytes = m_name.toUtf8();
    std::string nickName(bytes.data(),bytes.length());
    _io->socket()->emit("add user", nickName);
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
//        this->findChild<QWidget*>("sendBtn")->setEnabled(true);
    }
    else
    {
        this->findChild<QWidget*>("messageEdit")->setEnabled(false);
        this->findChild<QWidget*>("listView")->setEnabled(false);
//        this->findChild<QWidget*>("sendBtn")->setEnabled(false);
        ShowLoginDialog();
    }
}
