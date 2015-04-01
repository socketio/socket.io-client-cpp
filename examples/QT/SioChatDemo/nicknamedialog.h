#ifndef NICKNAMEDIALOG_H
#define NICKNAMEDIALOG_H

#include <QDialog>

namespace Ui {
class NicknameDialog;
}

class NicknameDialog : public QDialog
{
    Q_OBJECT

public Q_SLOTS:
    void accept();
    void exitApp();
public:
    explicit NicknameDialog(QWidget *parent = 0);
    ~NicknameDialog();

    const QString& getNickname() const;
    void closeEvent(QCloseEvent *);
private:
    Ui::NicknameDialog *ui;
    QString m_nickName;
};

#endif // NICKNAMEDIALOG_H
