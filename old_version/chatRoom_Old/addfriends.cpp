#include "addfriends.h"
#include "command.h"

#include <QString>
#include <QMessageBox>
#include <QByteArray>
#include <iostream>

AddFriends::AddFriends(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("添加好友"));
    this->setFixedSize(300,200);

    nameLabel = new QLabel(tr("Name:"));
    nameLineEdit = new QLineEdit();
    enterBtn = new QPushButton("Enter");

    tcpSocket = new QTcpSocket(this);

    mainLayout = new QGridLayout(this);
    mainLayout->addWidget(nameLabel, 0, 0);
    mainLayout->addWidget(nameLineEdit, 0, 1);
    mainLayout->addWidget(enterBtn, 1, 1);

    //连接服务器
    port = 12345;
    serverIP = new QHostAddress();
    QString ip = SERVER_IP;
    serverIP->setAddress(ip);

    connect(enterBtn, SIGNAL(clicked()), this, SLOT(slotEnter()));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
}


AddFriends::~AddFriends()
{
    tcpSocket->disconnectFromHost();
}
//在关闭窗口之前，断开与服务器的连接
void AddFriends::closeEvent(QCloseEvent *event)
{
    tcpSocket->disconnectFromHost();
    tcpSocket->waitForDisconnected();
    event->accept();
}
void AddFriends::slotEnter()
{
    tcpSocket->connectToHost(*serverIP, port);
    if(nameLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("Name is empty"), tr("Name is empty"));
        return;
    }
    QString msg = tr(ADDFRIENDS);
    toname=nameLineEdit->text();
    msg += username+"+"+toname;
    int length=tcpSocket->write(msg.toLatin1(), msg.toLatin1().length());
    if(length != msg.length())
    {
        return;
    }
    this->hide();
}

void AddFriends::dataReceived()
{
    QByteArray datagram;
    datagram.resize(tcpSocket->bytesAvailable());
    tcpSocket->read(datagram.data(), datagram.size());
    QString msg = datagram.data();

    if(msg == "The name is wrong")
    {
        QMessageBox::warning(this, tr("the message from server..."), tr("The name is wrong"));
        this->show();
    }
    else if(msg == "message has been sended")
    {
        QMessageBox::information(this, tr("the message from server..."), tr("message has been sended"));
        tcpSocket->disconnectFromHost();
    }
    else if(msg=="you are have been friends!")
    {
        QMessageBox::information(this, tr("the message from server..."), tr("you are have been friends!"));
        this->show();
    }
}
