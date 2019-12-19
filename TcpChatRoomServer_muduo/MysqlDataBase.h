#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H
#include "mysql/mysql.h"
#include "muduo/net/TcpConnection.h"
#include <string.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include "command.h"
using muduo::string;
using namespace muduo::net;
typedef std::map<muduo::net::TcpConnectionPtr,int> ConnectionMap;
class MysqlDataBase
{
public:
	MysqlDataBase():mysql(nullptr),res_ptr(nullptr),sqlrow(0){}
	int connect();
	int sqlQuery(const char *query);
	string parseMessageAndOperation(const ConnectionMap& connMap,const muduo::net::TcpConnectionPtr& conn,const string& msg);
	void doOffline(const TcpConnectionPtr& conn,const ConnectionMap& connMap);
private:
	MYSQL *mysql;
    MYSQL_RES *res_ptr;
    MYSQL_ROW sqlrow;
};
#endif	//MYSQLDATABASE_H
