#ifndef ONLINESTATE_H
#define ONLINESTATE_H

#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>
#include "command.h"
struct userInfo
{
  std::string name;
   int online;
   int pic;
};
class onlineState : public QWidget
{
    Q_OBJECT
public:
    explicit onlineState(QWidget *parent = 0);
    void sendRQ();
    //QString username;
    QTimer *SendRqTimer;
    struct userInfo *users;
    int count;
    bool StateUpdateDone;
private:
    int port;
    QHostAddress *serverIP;
    QTcpSocket *tcpSocket;
private slots:
    void dataReceived();
    void senRq();
};

#endif // ONLINESTATE_H
