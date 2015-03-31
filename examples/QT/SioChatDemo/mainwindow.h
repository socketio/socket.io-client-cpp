#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <sio_client.h>
namespace Ui {
class MainWindow;
}

using namespace sio;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    void SendBtnClicked();
    void TypingChanged();
    void LoginClicked();
    void OnMessageReturn();
protected:
    void showEvent(QShowEvent* event);

Q_SIGNALS:
    void RequestAddListItem(QListWidgetItem *item);
    void RequestToggleInputs(bool loginOrNot);
private Q_SLOTS:
    void AddListItem(QListWidgetItem *item);
    void ToggleInputs(bool loginOrNot);
private:
    void OnNewMessage(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp);
    void OnUserJoined(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp);
    void OnUserLeft(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp);
    void OnTyping(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp);
    void OnStopTyping(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp);
    void OnLogin(std::string const& name,message::ptr const& data,bool hasAck,message::ptr &ack_resp);
    void OnConnected();
    void OnClosed(client::close_reason const& reason);
    void OnFailed();

    Ui::MainWindow *ui;

    std::unique_ptr<client> _io;

    QString m_name;
};

#endif // MAINWINDOW_H
