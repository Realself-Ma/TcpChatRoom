#ifndef SIGN_H
#define SIGN_H


#include <QDialog>
#include <QPushButton>
#include <QTcpSocket>
#include <QGridLayout>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHostAddress>
#include <QTimer>
#include <string.h>
#include "command.h"
#include "drawer.h"

class Sign : public QDialog
{
    Q_OBJECT

public:
    Sign(QWidget *parent = 0);
    ~Sign();

private:
    QGridLayout *mainLayout;
    QLabel *nameLabel;
    QLabel *passwordLabel;
    QPushButton *connectBtn;
    QLineEdit *nameLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *registerBtn;
    QPushButton *LogInBtn;
    QTimer *addfriendTimer;
    QString fromname;
    QString toname;
    int port;
    QHostAddress *serverIP;
    QTcpSocket *tcpSocket;

    QString username;
    Drawer *drawer;
    QTimer *PicChangeTimer;
public slots:
    void slotRegister();
    void slotLogIn();
    void slotConnect();
    void dataReceived();
    void connected();
    void Checkflag();
    void CheckPicChange();
};

#endif // SIGN_H
