#include "MyReactor.h"

#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
auto my_logger = spdlog::stdout_color_mt("my_logger");//日志输出到屏幕

MyReactor::MyReactor()
{
}

MyReactor::~MyReactor()
{
}

struct ARG//定义结构体，用于传递this指针参数
{
    MyReactor* pThis;
};
//服务器功能初始化，完成数据库的连接，服务器的初始化，accept线程、广播线程、主工作线程的创建
bool MyReactor::init(const char* ip, short nport)
{
    //连接数据库
    int ret = connect();
    if(ret == -1)
    {
        my_logger->error("mysql connection failed");
    }


    if(!create_server_listener(ip, nport))//create_server_listener 初始化监听请求
    {
        my_logger->error("Unable to bind: {0}:{1}.", ip, nport);
        return false;
    }

    ARG *arg = new ARG();
    arg->pThis = this;

    pthread_create(&m_accept_threadid, NULL, accept_thread_proc, (void*)arg);//accept线程，接收监听请求并将请求挂到epoll监听树上

    pthread_create(&m_send_threadid, NULL, send_thread_proc, (void*)arg);//用于向所有连接到服务器的客户端广播消息的线程

    for(int i = 0; i < WORKER_THREAD_NUM; i++)
    {
        pthread_create(&m_threadid[i], NULL, worker_thread_proc, (void*)arg);
    }

    return true;
}

//关闭服务器
bool MyReactor::uninit()
{
    m_bStop = true;

    /* 将读端和写端都关闭 */
    shutdown(m_listenfd, SHUT_RDWR);
    close(m_listenfd);
    close(m_epollfd);

    mysql_close(mysql);

    return true;
}

//epoll监听结果线程
void* MyReactor::main_loop(void *p)
{
    my_logger->info("main thread id = {}", pthread_self());

    MyReactor* pReactor = static_cast<MyReactor*>(p);

    while(!pReactor->m_bStop)
    {
        struct epoll_event ev[1024];
        int n = epoll_wait(pReactor->m_epollfd, ev, 1024, 10);//10代表阻塞等待10ms，每次循环都等待10ms,超时后直接返回0
        if(n == 0)
            continue;
        else if(n < 0)
        {
            my_logger->error("epoll_wait error");
            continue;
        }

        int m = std::min(n, 1024);
        for(int i = 0; i < m; i++)
        {
            /* 有新连接 */
            if(ev[i].data.fd == pReactor->m_listenfd)
                pthread_cond_signal(&pReactor->m_accept_cond);//唤醒accept线程
            /* 有数据 */
            else
            {
                pthread_mutex_lock(&pReactor->m_client_mutex);
                pReactor->m_clientlist.push_back(ev[i].data.fd);
                pthread_mutex_unlock(&pReactor->m_client_mutex);
                pthread_cond_signal(&pReactor->m_client_cond);
            }
        }
    }

    my_logger->info("main loop exit ...");
    return NULL;
}

//关闭客户端封装函数（将客户端的描述符从epoll的监听树上摘下，再关闭客户端的描述符）
bool MyReactor::close_client(int clientfd)
{
    if(epoll_ctl(m_epollfd, EPOLL_CTL_DEL, clientfd, NULL) == -1)
    {
        my_logger->warn("release client socket failed as call epoll_ctl fail");
    }

    close(clientfd);
    return true;
}

//完成服务器的初始化，绑定，监听
bool MyReactor::create_server_listener(const char* ip, short port)
{
    m_listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(m_listenfd == -1)
    {
        my_logger->error("fail to create a socket");
        return false;
    }

	//设置服务器端口复用
    int on = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    if(bind(m_listenfd, (sockaddr *)&servaddr, sizeof(servaddr)) == -1)//绑定服务器
        return false;

    if(listen(m_listenfd, 50) == -1)//设置监听
        return false;

    m_epollfd = epoll_create(1);//创建epoll句柄，设置监听数量
    if(m_epollfd == -1)
        return false;

    struct epoll_event e;
    memset(&e, 0, sizeof(e));
    e.events = EPOLLIN | EPOLLRDHUP;//EPOLLIN: 表示对应的文件描述符可以读（包括对端SOCKET正常关闭）    EPOLLHUP：表示对应的文件描述符被挂断；
    e.data.fd = m_listenfd;
    if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &e) == -1)//向监听树中添加监听描述符
        return false;

    return true;
}

//等待epoll监听结果，若有新的连接请求，将其设置为非阻塞模式，并添加到epoll的监听树上
void* MyReactor::accept_thread_proc(void* args)
{
    ARG *arg = (ARG*)args;
    MyReactor* pReactor = arg->pThis;//获取this指针

    while(!pReactor->m_bStop)
    {
        pthread_mutex_lock(&pReactor->m_accept_mutex);//加锁
        pthread_cond_wait(&pReactor->m_accept_cond, &pReactor->m_accept_mutex);
		//阻塞等待条件变量m_accept_cond，同时释放互斥锁m_accept_mutex
		//当条件变量满足，线程被唤醒，同时重新加锁
		
        struct sockaddr_in clientaddr;
        socklen_t addrlen;
        int newfd = accept(pReactor->m_listenfd, (struct sockaddr *)&clientaddr, &addrlen);
        pthread_mutex_unlock(&pReactor->m_accept_mutex);
        if(newfd == -1)
            continue;


        pthread_mutex_lock(&pReactor->m_cli_mutex);
        pReactor->m_fds.insert(newfd);
        pthread_mutex_unlock(&pReactor->m_cli_mutex);

        my_logger->info("new client connected:");
		std::cout<<newfd<<std::endl;

        /* 将新socket设置为non-blocking */
        int oldflag = fcntl(newfd, F_GETFL, 0);
        int newflag = oldflag | O_NONBLOCK;
        if(fcntl(newfd, F_SETFL, newflag) == -1)
        {
            my_logger->error("fcntl error, oldflag = {0}, newflag = {1}", oldflag, newflag);
            continue;
        }

        struct epoll_event e;
        memset(&e, 0, sizeof(e));
        e.events = EPOLLIN | EPOLLRDHUP | EPOLLET;//EPOLLET epoll以水平触发模式工作，配合O_NONBLOCK使用，高效
        e.data.fd = newfd;
        /* 添加进epoll的兴趣列表 */
        if(epoll_ctl(pReactor->m_epollfd, EPOLL_CTL_ADD, newfd, &e) == -1)
        {
            my_logger->error("epoll_ctl error, fd = {}", newfd);
        }
    }

    return NULL;
}

//接收客户端消息，并完成用户数据与数据库的连接，对用户信息直接发送给客户端（不广播），对用户消息封装后交给广播线程
void* MyReactor::worker_thread_proc(void* args)
{
    ARG *arg = (ARG*)args;
    MyReactor* pReactor = arg->pThis;

    while(!pReactor->m_bStop)
    {
        int clientfd;
        pthread_mutex_lock(&pReactor->m_client_mutex);
        /* 注意！要用while循环等待 */
        while(pReactor->m_clientlist.empty())
            pthread_cond_wait(&pReactor->m_client_cond, &pReactor->m_client_mutex);

        /* 取出客户套接字 */
        clientfd = pReactor->m_clientlist.front();
        pReactor->m_clientlist.pop_front();
        pthread_mutex_unlock(&pReactor->m_client_mutex);

		
       //std::cout <<"run1..."<<clientfd<< std::endl;

		//获取当前时间
        time_t now = time(NULL);
        struct tm* nowstr = localtime(&now);
        std::ostringstream ostimestr;

        std::string strclientmsg;
        char buff[256];
        bool bError = false;
        //std::cout <<"run1..."<<clientfd<< std::endl;
        while(1)
        {
            memset(buff, 0, sizeof(buff));
            int nRecv = recv(clientfd, buff, 256, 0);//从客户端接收消息
			//char username[20];
            if(nRecv == -1)
            {
                if(errno == EWOULDBLOCK)
                    break;
                else
                {
					//出错后，关闭客户端，退出循环
                    my_logger->error("recv error, client disconnected, fd = {}", clientfd);
                    pReactor->close_client(clientfd);
                    bError = true;
                    break;
                }
            }
            /* 对端关闭了socket，这端i也关闭 */
            else if(nRecv == 0)
            {
                /* 将该客户从accept后的客户列表中删除（从epoll的监听树上删除） */
                pthread_mutex_lock(&pReactor->m_cli_mutex);
                pReactor->m_fds.erase(clientfd);
                pthread_mutex_unlock(&pReactor->m_cli_mutex);
				char query[100];
				int ret;
				pthread_mutex_lock(&pReactor->m_clie_name);
				std::cout<<"this clientfd:"<<clientfd<<std::endl;
				sprintf(query,"select username from UserInfo where clientfd=%d",clientfd);
				ret=pReactor->sqlQuery(query);
				if(ret==-1)
				{
					my_logger->error("select clientfd error:{}",mysql_error(pReactor->mysql));
				}
				pReactor->res_ptr=mysql_store_result(pReactor->mysql);
				char name[20];
				std::string tempstr;
				if(!pReactor->m_clientname.empty())
				{  
					while((pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr)))
					{
						//std::cout<<"search name:"<<pReactor->sqlrow[0]<<std::endl;
						std::list<std::string>::iterator it;
						for(it=pReactor->m_clientname.begin();
								it!=pReactor->m_clientname.end();it++)
						{
							//std::cout<<"*it:"<<*it<<std::endl;
							if(*it==pReactor->sqlrow[0])
							{
								tempstr=*it;
								strcpy(name,(*it).c_str());
								std::cout<<"username:"<<name<<"clientfd:"<<clientfd<<std::endl;	
								memset(query,0,sizeof(query));
								mysql_free_result(pReactor->res_ptr);
								sprintf(query,"select online from UserInfo where username='%s'",name);
								ret=pReactor->sqlQuery(query);
								if(ret==-1)
								{
									my_logger->error("select online error:{}",mysql_error(pReactor->mysql));
								}
								pReactor->res_ptr=mysql_store_result(pReactor->mysql);
								pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr);
								if(atoi(pReactor->sqlrow[0])==1)
								{
									memset(query,0,sizeof(query));
									sprintf(query,"update UserInfo set online=%d where username='%s'",0,name);
									int ret=pReactor->sqlQuery(query);
									if(ret==-1)
									{
										my_logger->error("update online error:{}",mysql_error(pReactor->mysql));
									}
									pReactor->m_clientname.remove(tempstr);
									tempstr.clear();
									break;
								}		
							}
						}
					}
				}
				mysql_free_result(pReactor->res_ptr);
                my_logger->info("peer closed, client disconnected, fd = {}", clientfd);
                pReactor->close_client(clientfd);
				//std::cout<<"close over"<<std::endl;
                bError = true;
				pthread_mutex_unlock(&pReactor->m_clie_name);
                break;
            }

            //注册
            if(buff[0] == REGISTER)
            {
				//std::cout<<"buff:"<<buff<<std::endl;
                std::string rec(buff);
				//std::cout<<"rec: "<<rec<<std::endl;
                auto it = rec.find('\t');
				//std::cout<<"it: "<<it<<std::endl;
                //const char *name = rec.substr(1, it-1).c_str();

				char name[20];
				strcpy(name,rec.substr(1,it-1).c_str());
				//std::cout<<"name :"<<name<<std::endl;
                //const char *password = rec.substr(it+1, rec.size()-1).c_str();
				char password[30];
				strcpy(password,rec.substr(it+1, rec.size()-1).c_str());
				//std::cout<<"password: "<<password<<std::endl;
                //std::cout <<"name: "<< name << '\t' <<"password: "<< password << std::endl;
                char query[100];
                sprintf(query, "select username from UserInfo where username = '%s'", name);
				//std::cout<<"query1: "<<query<<std::endl;
				int ret;
				//std::cout<<"before query,ret= "<<ret<<std::endl;
                ret = pReactor->sqlQuery(query);
				//std::cout<<"after query,ret= "<<ret<<std::endl;
				//std::cout<<"ret= "<<ret<<std::endl;
                pReactor->res_ptr = mysql_store_result(pReactor->mysql); //即使不需要返回值也要这样，否则会出错
				pReactor->sqlrow = mysql_fetch_row(pReactor->res_ptr);
                if(pReactor->sqlrow)
                {
					//my_logger->error("select error {}", mysql_error(pReactor->mysql));
					strclientmsg += "The name has been registered";
                }
                else
                {
                    memset(query, 0, sizeof(query));
					//std::cout<<"clientfd: "<<clientfd<<std::endl;
                    sprintf(query, "insert into UserInfo values('%s', '%s',%d,%d,%d)", name, password,clientfd,0,0);
					//std::cout<<"query2: "<<query<<std::endl;
                    ret = pReactor->sqlQuery(query);
                    if(ret == -1)
                    {
                        my_logger->error("insert error {}", mysql_error(pReactor->mysql));
                    }
                    else
                    {
                        //注册成功
                        //新建一个用户表，用于保存用户信息
                        memset(query, 0, sizeof(query));
                        sprintf(query, "create table %s (friends char(30));",name);
						//std::cout<<"query3: "<<query<<std::endl;
                        ret=pReactor->sqlQuery(query);
						if(ret==-1)
						{
							my_logger->error("create table error:{}",mysql_error(pReactor->mysql));
						}
                        //把自己加入到好友表中
                        memset(query, 0, sizeof(query));
                        sprintf(query, "insert into %s values('%s')", name, name);
                        ret=pReactor->sqlQuery(query);
						if(ret==-1)
						{
							my_logger->error("inserr error:{}",mysql_error(pReactor->mysql));
						}
                        strclientmsg += "REGISTER SUCCESS";
                       // continue;
                        if(mysql_errno(pReactor->mysql))
                        {
                            my_logger->error("retrive error {}", mysql_error(pReactor->mysql));
                        }
                    }
                }
                mysql_free_result(pReactor->res_ptr);
            }
            //登录
            else if(buff[0] == LOGIN)
            {
                std::string rec(buff);
                auto it = rec.find('\t');
				char name[20];
				strcpy(name,rec.substr(1,it-1).c_str());
				std::string str(name);
				char password[30];
				strcpy(password,rec.substr(it+1, rec.size()-1).c_str());

                char query[100];
                sprintf(query, "select password from UserInfo where username = '%s'",name);
                pReactor->sqlQuery(query);
                /* std::cout << "ret = " << ret << std::endl; */
				pReactor->res_ptr = mysql_store_result(pReactor->mysql); 
                pReactor->sqlrow = mysql_fetch_row(pReactor->res_ptr);
                if(!pReactor->sqlrow)
                {
					strclientmsg += "You have to register first";
                    //my_logger->error("select error {}", mysql_error(pReactor->mysql));
                }
                else
                {
					  int ret;
                      if(strcmp(pReactor->sqlrow[0], password) == 0)
                      {
						pReactor->m_clientname.push_back(str);
                        strclientmsg += "LOG IN SUCCESS";
						std::cout<<name<<":clientfd="<<clientfd<<std::endl;
                         //登录成功
                         //continue;
						mysql_free_result(pReactor->res_ptr);
						memset(query, 0, sizeof(query));
						sprintf(query,"select clientfd from UserInfo where username='%s'",name);
						ret=pReactor->sqlQuery(query);
						if(ret == -1)
                    	{
                        	my_logger->error("select error {}", mysql_error(pReactor->mysql));
                    	}
						pReactor->res_ptr=mysql_store_result(pReactor->mysql);
						pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr);
						if(clientfd!=atoi(pReactor->sqlrow[0]))
						{   
							memset(query, 0, sizeof(query));
							sprintf(query,"update UserInfo set clientfd=%d where username='%s'",clientfd,name);
							ret=pReactor->sqlQuery(query);
							if(ret == -1)
                    		{
                        		my_logger->error("update error {}", mysql_error(pReactor->mysql));
                    		}
							my_logger->info("server msg:{} have changed client to LOGIN",name);
						}
						memset(query,0,sizeof(query));
						sprintf(query,"update UserInfo set online=%d where username='%s'",1,name);
						pReactor->sqlQuery(query);
                      }
                      else
                      {
                         strclientmsg += "password is wrong";
                      }
				      //continue;
                      if(mysql_errno(pReactor->mysql))
                      {
                         my_logger->error("Retrive error {}", mysql_error(pReactor->mysql));
                      }

                 }
              mysql_free_result(pReactor->res_ptr);
            }
			else if(buff[0]==FRIENDSLIST)
			{			
						char query[100];
						char name[20];
						std::string rec(buff);
						strcpy(name,rec.substr(1,rec.size()-1).c_str());
						//std::cout<<"FRIENDLIST name:"<<name<<std::endl;
						memset(query,0,sizeof(query));
						sprintf(query,"select *from %s",name);
						int	ret=pReactor->sqlQuery(query);
						if(ret==-1)
						{
								my_logger->error("select friends error:{}",mysql_error(pReactor->mysql));
						}
						pReactor->res_ptr=mysql_store_result(pReactor->mysql);
						int rows=(int)mysql_num_fields(pReactor->res_ptr);
						//std::cout<<"clientfd:"<<clientfd<<std::endl;
						//std::deque<std::string> friendsmsg;
						std::string friendsmsg;
						int count=0;
						std::string tempstr;
						while((pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr)))
						{
							int i;
							for(i=0;i<rows;i++)
							{
								friendsmsg+=pReactor->sqlrow[i];
								friendsmsg+=":";
								//std::cout<<"friendsmsg:"<<friendsmsg<<std::endl;
								tempstr+=friendsmsg;
								//send(clientfd,friendsmsg.c_str(),friendsmsg.size(),0);
								friendsmsg.clear();
							}
							count++;
						}
						char str[5];
						sprintf(str,"%d",count);
						tempstr+=str;
						//std::cout<<"FRIENDSLIST tempstr--"<<tempstr<<std::endl;
						send(clientfd,tempstr.c_str(),tempstr.size(),0);
						mysql_free_result(pReactor->res_ptr);

			}
			//ONLINESTATE
			else if(buff[0]==ONLINESTATE)
			{
				//pReactor->onlineState_fd=clientfd;
				char query[100];
				sprintf(query,"select username,online,pic from UserInfo");
				pReactor->sqlQuery(query);
				pReactor->res_ptr=mysql_store_result(pReactor->mysql);
				int count=0;
				std::string namemsg;
				std::string onlinemsg;
				std::string	picmsg;
				std::string allmsg;
				std::string tempstr;
				while((pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr)))
				{
					namemsg+=pReactor->sqlrow[0];
					onlinemsg+=pReactor->sqlrow[1];
					picmsg+=pReactor->sqlrow[2];
					allmsg+=namemsg+","+onlinemsg+","+picmsg+",:";
					tempstr+=allmsg;
					//send(clientfd,allmsg.c_str(),allmsg.size(),0);
					namemsg.clear();
					onlinemsg.clear();
					picmsg.clear();
					allmsg.clear();
					count++;
				}	
				char str[5];
				sprintf(str,"%d",count);
				tempstr+=str;
				//std::cout<<"ONLINESTATE tempstr--"<<tempstr<<std::endl;
				send(clientfd,tempstr.c_str(),tempstr.size(),0);
				mysql_free_result(pReactor->res_ptr);
			}
			else if(buff[0]==PICCHANGE)
			{
				std::string rec(buff);
				auto it=rec.find(':');
				char name[20];
				int pic,ret;
				char query[100];
				strcpy(name,rec.substr(1,it-1).c_str());
				std::cout<<"name "<<name<<std::endl;
				pic=atoi(rec.substr(it+1,rec.size()-it-1).c_str());
				std::cout<<"pic "<<pic<<std::endl;
				sprintf(query,"update UserInfo set pic=%d where username='%s'",pic,name);
				ret=pReactor->sqlQuery(query);
				if(ret==-1)
				{
					my_logger->error("update pic error:{}",mysql_error(pReactor->mysql));
				}
			}
            //聊天信息
            else if(buff[0] == MESSAGE)
            {   
                strclientmsg += buff;
            }
            // TODO:添加好友消息
            else if(buff[0] == ADDFRIEND)
            {
				std::string rec(buff);
				char query[100];
				char fromname[20],toname[20];
				auto it=rec.find('+');
				strcpy(fromname,rec.substr(1,it-1).c_str());
				strcpy(toname,rec.substr(it+1,rec.size()-1).c_str());
				sprintf(query,"select username from UserInfo where username='%s'",toname);
				pReactor->sqlQuery(query);
				pReactor->res_ptr = mysql_store_result(pReactor->mysql);
				pReactor->sqlrow = mysql_fetch_row(pReactor->res_ptr);
				mysql_free_result(pReactor->res_ptr);
				if(!pReactor->sqlrow)
				{
					strclientmsg+="The name is wrong";
				}
				else
				{
					memset(query,0,sizeof(query));
					sprintf(query,"select friends from %s where friends='%s'",fromname,toname);
					pReactor->sqlQuery(query);
					pReactor->res_ptr=mysql_store_result(pReactor->mysql);
					pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr);
					mysql_free_result(pReactor->res_ptr);
					if(pReactor->sqlrow)
					{
						strclientmsg+="you are have been friends!";
					}
					else
					{
					memset(query,0,sizeof(query));
					sprintf(query,"select clientfd from UserInfo where username='%s'",toname);
					pReactor->sqlQuery(query);
					pReactor->res_ptr=mysql_store_result(pReactor->mysql);
					pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr);
					int tg_clientfd=atoi(pReactor->sqlrow[0]);
					std::cout<<"tg_clientfd: "<<tg_clientfd<<std::endl;
					std::string msg;
					msg+="*AdDR. ";
					//std::cout<<"msg[0] :"<<msg[0]<<std::endl;
					char tempstr[20];
					sprintf(tempstr,"%s+%s",fromname,toname);
					msg+=tempstr;
					std::cout<<"msg: "<<msg<<std::endl;
					send(tg_clientfd,msg.c_str(),msg.length(),0);
					strclientmsg+="message has been sended";
					}
				}
            }
			else if(buff[0]==ACCEPT)
			{
				std::string rec(buff);
				auto it=rec.find('\t');
				char fromname[20];
				char toname[20];
				char query1[100];
				char query2[100];
				char query[100];
				int ret;
				strcpy(fromname,rec.substr(1,it-1).c_str());
				strcpy(toname,rec.substr(it+1,rec.size()-(it+19)).c_str());
				std::cout<<"fromname "<<fromname<<"s"<<std::endl;
				std::cout<<"toname "<<toname<<"s"<<std::endl;
				memset(query1,0,sizeof(query1));
				memset(query2,0,sizeof(query2));
				sprintf(query1,"insert into %s values('%s')",fromname,toname);
				sprintf(query2,"insert into %s values('%s')",toname,fromname);
				sprintf(query,"select clientfd from UserInfo where username='%s'",fromname);
				ret=pReactor->sqlQuery(query1);
				ret=pReactor->sqlQuery(query2);
				pReactor->sqlQuery(query);
				pReactor->res_ptr=mysql_store_result(pReactor->mysql);
				pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr);
				int tg_clientfd=atoi(pReactor->sqlrow[0]);
				if(ret == -1)
                {
                     my_logger->error("insert error {}", mysql_error(pReactor->mysql));
                }
				std::string str="*ACCEPT.";
				send(tg_clientfd,str.c_str(),str.size(),0);
				strclientmsg+=buff;				
			}
			else if(buff[0]==REFUSE)
			{
				std::string rec(buff);
				char fromname[20];
				char query[100];
				int ret;
				strcpy(fromname,rec.substr(1,rec.size()-1).c_str());
				sprintf(query,"select clientfd from UserInfo where username='%s'",fromname);
				ret=pReactor->sqlQuery(query);
				if(ret==-1)
				{
					my_logger->error("select error {}",mysql_error(pReactor->mysql));
				}
				pReactor->res_ptr=mysql_store_result(pReactor->mysql);
				pReactor->sqlrow=mysql_fetch_row(pReactor->res_ptr);
				if(!pReactor->sqlrow)
				{
				my_logger->info("server msg:there is no user called {} in database UserInfo",fromname);
				}
				int tg_clientfd=atoi(pReactor->sqlrow[0]);
				std::string msg="your requet had been refused...";
				send(tg_clientfd,msg.c_str(),msg.size(),0);
			}
			else
			{	
				strclientmsg+=buff;
			}
        }
		//std::cout<<"run3..."<<clientfd<<std::endl;
        /* 如果出错了就不必往下执行了 */
        if(bError)
        {
            continue;
        }
		if(strclientmsg[0]==REGISTER||strclientmsg[0]==LOGIN||strclientmsg[0]==MESSAGE||strclientmsg[0]==ADDFRIEND||strclientmsg[0]==ACCEPT)
		{
        	my_logger->info("client msg: {}", strclientmsg.substr(1,strclientmsg.size()-1));
		}
		else 
		{
			 if(strclientmsg!="")
			{
       		 my_logger->info("client msg: {}", strclientmsg);
			}
		}

        if(strclientmsg[0] == MESSAGE||strclientmsg[0] == ACCEPT)
        {
            strclientmsg.erase(0, 1); //将命令标识符去掉
            /* 将消息加上时间戳 */
            ostimestr << "[" << nowstr->tm_year + 1900 << "-"
                << std::setw(2) << std::setfill('0') << nowstr->tm_mon + 1 << "-"
                << std::setw(2) << std::setfill('0') << nowstr->tm_mday << " "
                << std::setw(2) << std::setfill('0') << nowstr->tm_hour << ":"
                << std::setw(2) << std::setfill('0') << nowstr->tm_min << ":"
                << std::setw(2) << std::setfill('0') << nowstr->tm_sec << " ] ";

            strclientmsg.insert(0, ostimestr.str());//将时间戳插入到消息字符串的前面
        }
        else//不是交流的消息（注册、登录信息），就直接单独发送给客户端，不用广播
        {
			if(strclientmsg!="")
			{
            	send(clientfd, strclientmsg.c_str(), strclientmsg.length(), 0);
			}
            continue;//阶数本次循环，不会再广播
        }

        /* 将消息交给发送广播消息的线程 */
        pReactor->m_msgs.push_back(strclientmsg);
        pthread_cond_signal(&pReactor->m_send_cond);
		//std::cout<<"run4..."<<clientfd<<std::endl;
    }
    return NULL;
}

//广播用户消息
void* MyReactor::send_thread_proc(void *args)
{
    ARG *arg = (ARG*)args;
    MyReactor* pReactor = arg->pThis;

    while(!pReactor->m_bStop)
    {
        std::string strclientmsg;

        pthread_mutex_lock(&pReactor->m_send_mutex);
        /* 注意！要用while循环等待 */
        while(pReactor->m_msgs.empty())
            pthread_cond_wait(&pReactor->m_send_cond, &pReactor->m_send_mutex);//线程阻塞在条件变量上等待

		//线程被唤醒，取出列首数据，并将列首弹出
        strclientmsg = pReactor->m_msgs.front();
        pReactor->m_msgs.pop_front();
        pthread_mutex_unlock(&pReactor->m_send_mutex);

        std::cout << std::endl;


        while(1)
        {
            int nSend;
            int clientfd;
            //广播消息（向所有与服务器建立连接的客户端发送消息）
            for(auto it = pReactor->m_fds.begin(); it != pReactor->m_fds.end(); it++)
            {
                clientfd = *it;
				//std::cout<<"------onlineState_fd-----"<<pReactor->onlineState_fd<<std::endl;
				//if(clientfd==pReactor->onlineState_fd)
				//	continue;
                nSend = send(clientfd, strclientmsg.c_str(), strclientmsg.length(), 0);
                if(nSend == -1)
                {
                    if(errno == EWOULDBLOCK)
                    {
                        sleep(10);
                        continue;
                    }
                    else
                    {
                        my_logger->error("send error, fd = {}", clientfd);
                        pReactor->close_client(clientfd);
                        break;
                    }
                }
            }

            my_logger->info("send: {}", strclientmsg);
            /* 发送完把缓冲区清干净 */
            strclientmsg.clear();

			//代表整个while循环只执行一次
            if(strclientmsg.empty())
                break;
        }
    }

    return NULL;
}

//数据库连接
int MyReactor::connect()
{
    mysql = mysql_init(NULL);//数据库初始化
    if(!mysql)
    {
        my_logger->error("mysql init falied");
        return -1;
    }

    /* root为用户名，mysql123为密码，ChatRoom为要连接的database */
    mysql = mysql_real_connect(mysql, IP, "root", "mysql123", "ChatRoom", 0, NULL, 0);//连接到数据库

    if(mysql)
    {
        my_logger->info("MySQL connection success");
    }
    else
    {
        my_logger->warn(" MySQL connection failed");
    }
    return 0;
}

//数据库操作
int MyReactor::sqlQuery(const char* query)
{
    int res = mysql_query(mysql, query);
	//std::cout<<"query res= "<<res<<std::endl;
	//if(mysql_errno(mysql))
	//{
	//	const char *errorstr=mysql_error(mysql);
	//	std::cout<<"a error occured: "<<errorstr<<std::endl;
	//}
    if(res)
    {
        return -1;
    }
    return 0;
}
