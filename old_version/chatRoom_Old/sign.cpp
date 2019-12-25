#include "sign.h"
#include "drawer.h"
#include <QString>
#include <QMessageBox>
#include <QByteArray>
#include <iostream>

Sign::Sign(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("聊天室客户端"));
    this->setFixedSize(400,200);

    registerBtn = new QPushButton(tr("注册"));
    LogInBtn = new QPushButton(tr("登录"));
    connectBtn = new QPushButton(tr("连接服务器"));
    nameLabel = new QLabel(tr("Name(英文):"));
    passwordLabel = new QLabel(tr("Password:"));
//    ServerIPLabel=new QLabel(tr("ServerIP"));
//    ServerIPEdit=new QLineEdit();
    nameLineEdit = new QLineEdit();
    passwordLineEdit = new QLineEdit();
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    tcpSocket = new QTcpSocket(this);


    mainLayout = new QGridLayout(this);
    mainLayout->addWidget(nameLabel, 0, 0);
    mainLayout->addWidget(nameLineEdit, 0, 1, 1, 2);
    mainLayout->addWidget(passwordLabel, 1, 0);
    mainLayout->addWidget(passwordLineEdit, 1, 1, 1, 2);
//    mainLayout->addWidget(ServerIPLabel, 2, 0);
//    mainLayout->addWidget(ServerIPEdit, 2, 1, 1, 2);
    mainLayout->addWidget(registerBtn, 2, 0);
    mainLayout->addWidget(LogInBtn, 2, 1);
    mainLayout->addWidget(connectBtn, 2, 2);
    //mainLayout->setContentsMargins(0,0,0,0);//让布局紧贴主窗口，不留空白
    //mainLayout->setSpacing(0);

    addfriendTimer=new QTimer(this);
    connect(addfriendTimer,SIGNAL(timeout()),this,SLOT(Checkflag()));

    PicChangeTimer=new QTimer(this);
    connect(PicChangeTimer,SIGNAL(timeout()),this,SLOT(CheckPicChange()));

    registerBtn->setEnabled(false);
    LogInBtn->setEnabled(false);

    connect(connectBtn, SIGNAL(clicked()), this, SLOT(slotConnect()));
    connect(registerBtn, SIGNAL(clicked()), this, SLOT(slotRegister()));
    connect(LogInBtn, SIGNAL(clicked()), this, SLOT(slotLogIn()));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(connected()));
}

Sign::~Sign()
{
    tcpSocket->disconnectFromHost();
}
void Sign::slotConnect()
{
    if(nameLineEdit->text() == "" || passwordLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("error"), tr("Name or Password is empty!"));
        return;
    }

    //连接服务器
    port = 12345;
    serverIP = new QHostAddress();
    QString ip = SERVER_IP;
    serverIP->setAddress(ip);
    tcpSocket->connectToHost(*serverIP, port);
}

void Sign::connected()
{
    connectBtn->setEnabled(false);
    registerBtn->setEnabled(true);
    LogInBtn->setEnabled(true);
}


void Sign::slotRegister()
{
    QString msg = tr(REGISTER);
    username = nameLineEdit->text();
    QString password = passwordLineEdit->text();
    msg += username + '\t' + password;
    int length = tcpSocket->write(msg.toLatin1(), msg.toLatin1().length());
    if(length != msg.toUtf8().length())
    {
        return;
    }
}

void Sign::slotLogIn()
{
    QString msg = tr(LOGIN);
    username = nameLineEdit->text();
    QString password = passwordLineEdit->text();
    msg += username + '\t' + password;
    int length = tcpSocket->write(msg.toLatin1(), msg.toLatin1().length());
    if(length != msg.toUtf8().length())
    {
        return;
    }
}

void Sign::dataReceived()
{
    QByteArray datagram;
    datagram.resize(tcpSocket->bytesAvailable());
    tcpSocket->read(datagram.data(), datagram.size());
    QString msg = datagram.data();

  //qDebug()<<"msg [0]:"<<msg[0];
    //如果登录成功就可以进入主界面
    if(msg=="LOG IN SUCCESS")
    {
        QMessageBox::information(this, tr("log in success"), tr("log in success"));
        this->hide();

        drawer = new Drawer();
        drawer->toolBtn1->setText(username);
        drawer->username=username;
        //drawer->resize(250, 500);
        drawer->setFixedSize(250,500);
        drawer->show();
        PicChangeTimer->start(2000);
    }
    else if(msg == "REGISTER SUCCESS")
    {
        QMessageBox::information(this, tr("register success, now you can log in"), tr("register success, now you can log in"));
    }
    else if(msg.contains("*AdDR.",Qt::CaseSensitive))
     {
        std::string rec=msg.toStdString();
        auto it = rec.find('+');
        //qDebug()<<"it "<<it;
        fromname=QString::fromStdString(rec.substr(7,it-7));
        //qDebug()<<fromname;
        toname=QString::fromStdString(rec.substr(it+1, rec.size()-1));
        //qDebug()<<toname;
        drawer->fromname=fromname;
        drawer->toname=toname;
        drawer->toolBtn2->show();
        addfriendTimer->start(10);
    }
    else if(msg=="your requet had been refused...")
    {
        drawer->toolBtn2->show();
        drawer->changefunc=true;
    }
    else if(msg.contains("*ACCEPT.",Qt::CaseSensitive))
    {
        int currNum=drawer->friendsNum;
        qDebug()<<"toname:"<<drawer->addFriends->toname;
        drawer->friendsListButtons[currNum]->setText(drawer->addFriends->toname);
        drawer->friendsListButtons[currNum]->setIcon(QPixmap(":/images/qq.png"));
        drawer->friendsListButtons[currNum]->setIconSize(QPixmap(":/images/qq.png").size());
        drawer->friendsListButtons[currNum]->setAutoRaise(true);
        drawer-> friendsListButtons[currNum]->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        drawer->friLayout->addWidget(drawer->friendsListButtons[currNum]);
        drawer->friendsNum++;
    }
    else if(msg=="The name has been registered")
    {
        QMessageBox::information(this, tr("The name has been registered"), tr("The name has been registered"));
    }
    else if(msg=="You have to register first")
    {
        QMessageBox::information(this, tr("You have to register first"), tr("You have to register first"));
    }
    else if(msg=="password is wrong")
    {
        QMessageBox::information(this, tr("password is wrong"), tr("password is wrong"));
    }
//    else if(msg.contains("*ONLINE.",Qt::CaseSensitive))
//    {
//        qDebug()<<"msg   :"<<msg;
//        std::string rec=msg.toStdString();
//        auto it1 = rec.find('$');
//        auto it2=rec.find('*');
//        QString onlineName=QString::fromStdString(rec.substr(it1+1,it2-it1-1));
//        qDebug()<<"onlineName:"<<onlineName;
//        drawer->onlinename=onlineName;
//        drawer->onlineTimer->start(500);
//    }
}
void Sign::Checkflag()
{
    if(drawer->addflag)
    {
        QString msg = tr(ACCEPT);
        msg+=fromname+"\t"+toname+" have been friends";
        int length=0;
        if((length = tcpSocket->write(msg.toLatin1(),msg.toLatin1(). length())) != msg.toLatin1().length())
        {
            return;
        }
        addfriendTimer->stop();
        fromname.clear();
        //toname.clear();
        drawer->addflag=false;
    }
    if(drawer->refuseflag)
     {
        QString msg = tr(REFUSE);
         msg+=fromname;
         int length=0;
         if((length = tcpSocket->write(msg.toLatin1(),msg.toLatin1(). length())) != msg.toLatin1().length())
         {
             return;
         }
         addfriendTimer->stop();
         fromname.clear();
         toname.clear();
         drawer->refuseflag=false;
    }
}
void Sign::CheckPicChange()
{
    if(drawer->PicChangeflag)
    {
        QString msg = tr(PICCHANGE);
        QString pic=QString::number(drawer->SelectMapStytle->currentIndex(),10);
        msg+=drawer->username+":"+pic;
        tcpSocket->write(msg.toLatin1(),msg.toLatin1(). length());
        drawer->PicChangeflag=false;
    }

}
