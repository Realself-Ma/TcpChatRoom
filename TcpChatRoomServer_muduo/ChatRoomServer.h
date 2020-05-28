#ifndef CHATROOMSERVER_H
#define CHATROOMSERVER_H
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/Logging.h"
#include "MysqlDataBase.h"
#include "command.h"
#include <stdlib.h>
#include <set>
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using namespace muduo;
using namespace muduo::net;
using muduo::string;
class ChatRoomServer
{
public:
	ChatRoomServer(EventLoop *loop,const InetAddress& listenAddr);
	void onConnection(const TcpConnectionPtr& conn);
	void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time);
	void start();
private:

	void send(const TcpConnectionPtr& conn,string& sendmsg,Timestamp time);
	void broadcast(const string& msg);
	typedef std::set<TcpConnectionPtr> ConnectionSet;
	MysqlDataBase _Mysql;
	TcpServer _server;
	ConnectionSet _connections;
};
#endif	//CHATROOMSERVER_H
