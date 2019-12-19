#ifndef FRIENDSLIST_H
#define FRIENDSLIST_H

#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QQueue>
#include "command.h"
class friendsList : public QWidget
{
    Q_OBJECT
public:
    explicit friendsList(QWidget *parent = 0);
    ~friendsList();
    void sendRQ();
    QString username;
    bool havedoneFlag;
    QQueue<QString> friendList;
    QTcpSocket *tcpSocket;
private:
    int port;
    QHostAddress *serverIP;
private slots:
    void dataReceived();
};

#endif // FRIENDSLIST_H
