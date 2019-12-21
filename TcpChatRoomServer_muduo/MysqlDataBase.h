#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H
#include "mysql/mysql.h"
#include "muduo/net/TcpConnection.h"
#include <string.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <map>
#include "command.h"
using muduo::string;
using namespace muduo::net;
typedef std::map<TcpConnectionPtr,std::string> ConnectionMap;
class MysqlDataBase
{
public:
	MysqlDataBase():mysql(nullptr),res_ptr(nullptr),sqlrow(0){}
	int connect();
	int sqlQuery(const char *query);
	string parseMessageAndOperation(const TcpConnectionPtr& conn,const string& msg);
	void doOffline(const TcpConnectionPtr& conn);
private:
	ConnectionMap nameMap_;
	MYSQL *mysql;
    MYSQL_RES *res_ptr;
    MYSQL_ROW sqlrow;
};
#endif	//MYSQLDATABASE_H
