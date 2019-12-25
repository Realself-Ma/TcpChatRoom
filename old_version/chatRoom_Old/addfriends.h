#ifndef ADDFRIENDS_H
#define ADDFRIENDS_H

#include <QDialog>
#include <QPushButton>
#include <QTcpSocket>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QHostAddress>
#include <QCloseEvent>
#include "command.h"

class AddFriends : public QDialog
{
    Q_OBJECT
public:
    AddFriends(QWidget *parent = 0);
    ~AddFriends();
public:
    QString username;
    QString toname;
private:
    QGridLayout *mainLayout;
    QLabel *nameLabel;
    QLineEdit *nameLineEdit;
    QPushButton *enterBtn;
    void closeEvent(QCloseEvent *event);//关闭事件
    int port;
    QHostAddress *serverIP;
    QTcpSocket *tcpSocket;

public slots:
    void slotEnter();
    void dataReceived();
};

#endif // ADDFRIENDS_H
