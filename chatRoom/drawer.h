#ifndef DRAWER_H
#define DRAWER_H

#include <QToolButton>
#include <QTableWidget>
#include <QWidget>
#include <QPushButton>
#include <QToolBox>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QQueue>
#include <QTimer>
#include <QPoint>
#include <QApplication>
#include "tcpclient.h"
#include "addfriends.h"
#include "friendslist.h"
#include "onlinestate.h"
#include "command.h"
class Drawer : public QToolBox
{
    Q_OBJECT
public:
    Drawer(QWidget *parent = 0, Qt::WindowFlags f = 0);

    QString username;
    QString fromname;
    QString toname;
    QString onlinename;
    QTabWidget *tabWidget;
    QToolButton *toolBtn1;
    QToolButton *toolBtn2;
    QPushButton *addFriendsBtn;
    QComboBox *SelectMapStytle;
    QWidget *myFriends;
    QVBoxLayout *friLayout;
    //QLabel *picSelect;
    bool addflag;
    bool refuseflag;
    bool changefunc;
    bool PicChangeflag;
    QQueue<QString> friendList;
    QTimer *havedoneTimer;
    QToolButton **friendsListButtons;
    int friendsNum;
    AddFriends *addFriends;
private slots:
    void showChatWidget1();
    void slotAddFriends();
    void showaddfriendWidget();
    void MapStytleSet();
    void getFDL();
    void CheckFlag();
    void CheckonlineState();
    void Quit();
private:
    QPoint windowPos;//窗口位置
    QPoint mousePos;//鼠标位置
    QPoint dPos;//鼠标移动后的位置
    void mousePressEvent(QMouseEvent *event);//鼠标按下事件（用于移动主界面）
    void mouseMoveEvent(QMouseEvent *event);//鼠标移动事件
    void initialMapstylestring();
    void initialfriendsList();
    QString MapStytle[11];
    TcpClient *chat1;
    friendsList *getFriendsList;
    onlineState *checkOnlineState;
    QTimer *getfdlTimer;
    QTimer *onlineStateTimer;
    struct userInfo *users;//有嫌疑
};

#endif // DRAWER_H
