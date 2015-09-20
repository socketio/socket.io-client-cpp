#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTimer>
#include <sio_client.h>
#include "nicknamedialog.h"
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
    void OnMessageReturn();
protected:
    void showEvent(QShowEvent* event);

Q_SIGNALS:
    void RequestAddListItem(QListWidgetItem *item);
    void RequestRemoveListItem(QListWidgetItem *item);
    void RequestToggleInputs(bool loginOrNot);
private Q_SLOTS:
    void AddListItem(QListWidgetItem *item);
    void RemoveListItem(QListWidgetItem *item);
    void ToggleInputs(bool loginOrNot);
    void TypingStop();
    void NicknameAccept();
    void NicknameCancelled();
private:
    void OnNewMessage(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp);
    void OnUserJoined(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp);
    void OnUserLeft(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp);
    void OnTyping(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp);
    void OnStopTyping(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp);
    void OnLogin(std::string const& name,message::ptr const& data,bool hasAck,message::list &ack_resp);
    void OnConnected(std::string const& nsp);
    void OnClosed(client::close_reason const& reason);
    void OnFailed();
    void ShowLoginDialog();

    Ui::MainWindow *ui;

    std::unique_ptr<client> _io;

    std::unique_ptr<NicknameDialog> m_dialog;

    QString m_name;

    std::unique_ptr<QTimer> m_timer;

    QListWidgetItem *m_typingItem;
};

#endif // MAINWINDOW_H
