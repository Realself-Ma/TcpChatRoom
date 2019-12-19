#include "friendslist.h"

friendsList::friendsList(QWidget *parent) : QWidget(parent)
{
    //连接服务器
    port = 2019;
    serverIP = new QHostAddress();
    QString ip = SERVER_IP;
    serverIP->setAddress(ip);
    tcpSocket=new QTcpSocket(this);
    tcpSocket->connectToHost(*serverIP, port);
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
    havedoneFlag=false;
}
friendsList::~friendsList()
{
    tcpSocket->disconnectFromHost();
}
void friendsList::sendRQ()
{
    QString msg = tr(FRIENDSLIST);
    msg+=username;
    int length=tcpSocket->write(msg.toUtf8(), msg.toUtf8().length());
    if(length != msg.toUtf8().length())
    {
        return;
    }
}
void friendsList::dataReceived()
{
    QByteArray datagram;
    datagram.resize(tcpSocket->bytesAvailable());
    tcpSocket->read(datagram.data(), datagram.size());
    QString msg = datagram.data();
    //qDebug()<<"friendslist msg:"<<msg;
    if(msg.contains(":",Qt::CaseSensitive))
    {
    int count=msg.mid(msg.size()-1,1).toInt();
    //qDebug()<<"friendslist count:"<<count;
    int i;
    QString name;
    for(i=0;i<count;i++)
    {
        name=msg.section(':',i,i);
        friendList.push_back(name);
    }
    if(count!=0)
    {
        havedoneFlag=true;
    }
    }
    tcpSocket->disconnectFromHost();
}

