#include "ChatRoomServer.h"

ChatRoomServer::ChatRoomServer(EventLoop* loop,const InetAddress& listenAddr):connid(0),_server(loop,listenAddr,"ChatRoomServer")
{
	_server.setConnectionCallback(std::bind(&ChatRoomServer::onConnection,this,_1));
	_server.setMessageCallback(std::bind(&ChatRoomServer::onMessage,this,_1,_2,_3));
}
void ChatRoomServer::broadcast(const std::string& msg)
{
	for(std::map<TcpConnectionPtr,int>::iterator it=_connections.begin();it!=_connections.end();++it)
	{
		(it->first)->send(msg);
	}
}
void ChatRoomServer::send(const TcpConnectionPtr& conn,string& sendmsg,Timestamp time)
{
	if(sendmsg[0] == MESSAGE||sendmsg[0] == ACCEPT)
	{
		sendmsg.erase(0,1);
		string timestr="[UTC "+time.toFormattedString()+"]  ";
		sendmsg.insert(0,timestr);
		broadcast(sendmsg);
	}
	else
	{
		conn->send(sendmsg);
	}

}

void ChatRoomServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<(conn->connected()?" Up":" Down");
	
	if(conn->connected())
	{
		_connections.insert(make_pair(conn,++connid));
	}
	else
	{
		--connid;
		_Mysql.doOffline(conn,_connections);
		_connections.erase(conn);
	}
}
void ChatRoomServer::onMessage(const TcpConnectionPtr& conn,Buffer *buf,Timestamp time)
{
	string msg(buf->retrieveAllAsString());
	
	string sendmsg=_Mysql.parseMessageAndOperation(_connections,conn,msg);
	send(conn,sendmsg,time);
	if(sendmsg!="")
	{
		LOG_INFO<<"client msg:"<<sendmsg.substr(1,sendmsg.size()-1)<<" "<<time.toFormattedString();
	}
}
void ChatRoomServer::start()
{
	_server.start();
	int ret=_Mysql.connect();
	if(ret==-1)
	{
		exit(1);
	}
}
