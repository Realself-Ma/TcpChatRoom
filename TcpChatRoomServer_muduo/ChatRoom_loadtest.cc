#include "command.h"
#include "muduo/base/Atomic.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/TcpClient.h"

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int g_connections = 0;
AtomicInt32 g_aliveConnections;
AtomicInt32 g_messagesReceived;
bool firstsend=false;
Timestamp g_startTime;
std::vector<Timestamp> g_receiveTime;
EventLoop* g_loop;
std::function<void()> g_statistic;

class ChatRoomClient : noncopyable
{
 public:
  ChatRoomClient(EventLoop* loop, const InetAddress& serverAddr,int command)
    : loop_(loop),
      client_(loop, serverAddr, "LoadTestClient"),command_(command)
  {
    client_.setConnectionCallback(
        std::bind(&ChatRoomClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&ChatRoomClient::onMessage, this, _1, _2, _3));
    //client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

  void disconnect()
  {
    // client_.disconnect();
  }

  Timestamp receiveTime() const { return receiveTime_; }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
      connection_ = conn;
      if (g_aliveConnections.incrementAndGet() == g_connections)
      {
        LOG_INFO << "all connected";
      }
	  send();
    }
    else
    {
      connection_.reset();
    }
  }
  void onMessage(const TcpConnectionPtr& ,Buffer *buf,Timestamp)
  {
	muduo::string message(buf->retrieveAllAsString());
	printf("<<< %s\n", message.c_str());
	receiveTime_ = loop_->pollReturnTime();
    int received = g_messagesReceived.incrementAndGet();
    if (received == g_connections)
    {
      Timestamp endTime = Timestamp::now();
      LOG_INFO << "all received " << g_connections << " in "
               << timeDifference(endTime, g_startTime);
      g_loop->queueInLoop(g_statistic);
    }
    else if (received % 1000 == 0)
    {
      LOG_DEBUG << received;
    }
  }
  
  void send()
  {
    muduo::string msg;
    char name[20];
	snprintf(name,sizeof(name),"loadtest%d",g_aliveConnections.get());
    muduo::string username(name);
	if(!firstsend)
	{
    	g_startTime = Timestamp::now();
		firstsend=true;
	}
	switch (command_)
	{
		case 1:
			msg=REGISTER+username+"\t"+"7777777";
			connection_->send(msg);
			break;
		case 2:
			msg=LOGIN+username+"\t"+"7777777";
			connection_->send(msg);
			break;
		case 3:
			msg+=ONLINESTATE;
			connection_->send(msg);
			break;
		case 4:
			msg="loadtest";
			connection_->send(msg);
			break;
		default:
			break;
	}
    LOG_DEBUG << "sent";
  }

  EventLoop* loop_;
  TcpClient client_;
  TcpConnectionPtr connection_;
  Timestamp receiveTime_;
  int command_;
};

void statistic(const std::vector<std::unique_ptr<ChatRoomClient>>& clients)
{
  LOG_INFO << "statistic " << clients.size();
  std::vector<double> seconds(clients.size());
  for (size_t i = 0; i < clients.size(); ++i)
  {
    seconds[i] = timeDifference(clients[i]->receiveTime(), g_startTime);
  }

  std::sort(seconds.begin(), seconds.end());
  for (size_t i = 0; i < clients.size(); i += std::max(static_cast<size_t>(1), clients.size()/20))
  {
    printf("%6zd%% %.6f\n", i*100/clients.size(), seconds[i]);
  }
  if (clients.size() >= 100)
  {
    printf("%6d%% %.6f\n", 99, seconds[clients.size() - clients.size()/100]);
  }
  printf("%6d%% %.6f\n", 100, seconds.back());
}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 4)
  {
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(argv[1], port);
    g_connections = atoi(argv[3]);
    int threads = 0;
    if (argc > 5)
    {
      threads = atoi(argv[5]);
    }

    EventLoop loop;
    g_loop = &loop;
    EventLoopThreadPool loopPool(&loop, "ChatRoom-loadtest");
    loopPool.setThreadNum(threads);
    loopPool.start();

    g_receiveTime.reserve(g_connections);
    std::vector<std::unique_ptr<ChatRoomClient>> clients(g_connections);
    g_statistic = std::bind(statistic, std::ref(clients));

    for (int i = 0; i < g_connections; ++i)
    {
      clients[i].reset(new ChatRoomClient(loopPool.getNextLoop(), serverAddr,atoi(argv[4])));
      clients[i]->connect();
      usleep(200);
    }

    loop.loop();
    // client.disconnect();
  }
  else
  {
    printf("Usage: %s host_ip port connections command [threads] \ncommand:\n\t1 REGISTER\n\t2 LOGIN\n\t3 ONLINESTATE\n\t4 MESSAGE\n",argv[0]);
  }
}
