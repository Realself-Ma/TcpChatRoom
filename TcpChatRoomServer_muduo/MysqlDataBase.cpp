#include "MysqlDataBase.h"
#include "muduo/base/Logging.h"
int MysqlDataBase::connect()
{
    mysql = mysql_init(NULL);//数据库初始化
    if(!mysql)
    {
        LOG_ERROR<<"mysql init falied";
        return -1;
    }

    mysql = mysql_real_connect(mysql,IP, "root", "140226", "ChatRoom", 0, NULL, 0);//连接到数据库

    if(mysql)
    {
        LOG_INFO<<"MySQL connection success";
    }
    else
    {
        LOG_WARN<<" MySQL connection failed";
        return -1;
    }
    return 0;
}
int MysqlDataBase::sqlQuery(const char *query)
{
	int res=mysql_query(mysql, "set names utf8");
	if(res!=0)
	{
		LOG_INFO<<"mysql_query set utf8 error";
		return -1;
	}
    res=mysql_query(mysql,query);
    if(res)
    {
        return -1;
    }
    return 0;
}
void MysqlDataBase::doOffline(const TcpConnectionPtr& conn,const ConnectionMap& connMap)
{
	auto it=connMap.find(conn);
	int connid=it->second;
	char query[100];
	snprintf(query,sizeof(query),"select online from UserInfo where connid=%d",connid);
	int ret=sqlQuery(query);
	if(ret==-1)
	{
		LOG_ERROR<<"doOffline select error:"<<mysql_error(mysql);
	}
	res_ptr=mysql_store_result(mysql);
	sqlrow=mysql_fetch_row(res_ptr);
	if(sqlrow&&(atoi(sqlrow[0])==1))
	{
		memset(query,0,sizeof(query));
		snprintf(query,sizeof(query),"update UserInfo set online=%d where connid=%d",0,connid);
		ret=sqlQuery(query);
		if(ret==-1)
		{
			LOG_ERROR<<"doOffline update error:"<<mysql_error(mysql);
		}
	}
	mysql_free_result(res_ptr);
}
string MysqlDataBase::parseMessageAndOperation(const ConnectionMap& connMap,const muduo::net::TcpConnectionPtr& conn,const string& buff)
{
    std::string  strclientmsg;
    if(buff[0] == REGISTER)
    {
        std::string rec(buff);
        if(rec.size()==0)
        {
            std::cout<<"Register substr error"<<std::endl;
        }
        auto it = rec.find('\t');


        char name[20];
        strcpy(name,rec.substr(1,it-1).c_str());

        char password[30];
        strcpy(password,rec.substr(it+1, rec.size()-1).c_str());

        char query[100];
        sprintf(query, "select username from UserInfo where username = '%s'", name);

        int ret;

        ret = sqlQuery(query);

        res_ptr = mysql_store_result(mysql); //即使不需要返回值也要这样，否则会出错
        sqlrow = mysql_fetch_row(res_ptr);
        if(sqlrow)
        {

            strclientmsg += "The name has been registered";
        }
        else
        {
            memset(query, 0, sizeof(query));
            auto it=connMap.find(conn);
            if(it==connMap.end())
            {
                LOG_ERROR<<"Map find error";
            }
            sprintf(query, "insert into UserInfo values('%s', '%s',%d,%d,%d)", name, password,0,0,it->second);

            ret = sqlQuery(query);
            if(ret == -1)
            {
                LOG_ERROR<<"insert error :"<<mysql_error(mysql);
            }
            else
            {
                //注册成功
                //新建一个用户表，用于保存用户信息
                memset(query, 0, sizeof(query));
                sprintf(query, "create table %s (friends char(30));",name);

                ret=sqlQuery(query);
                if(ret==-1)
                {
                    LOG_ERROR<<"create table error :"<<mysql_error(mysql);
                }
                //把自己加入到好友表中
                memset(query, 0, sizeof(query));
                sprintf(query, "insert into %s values('%s')", name, name);
                ret=sqlQuery(query);
                if(ret==-1)
                {
                    LOG_ERROR<<"insert error :"<<mysql_error(mysql);
                }
                strclientmsg += "REGISTER SUCCESS";

                if(mysql_errno(mysql))
                {
                    LOG_ERROR<<"retrive error :"<<mysql_error(mysql);
                }
            }
        }
        mysql_free_result(res_ptr);
    }
    //登录
    else if(buff[0] == LOGIN)
    {
        std::string rec(buff);
        if(rec.size()==0)
        {
            std::cout<<"LOGIN substr error"<<std::endl;
        }
        auto it = rec.find('\t');
        char name[20];
        strcpy(name,rec.substr(1,it-1).c_str());
        std::string str(name);
        char password[30];
        strcpy(password,rec.substr(it+1, rec.size()-1).c_str());

        char query[100];
        sprintf(query, "select password from UserInfo where username = '%s'",name);
        sqlQuery(query);

        res_ptr = mysql_store_result(mysql);
        sqlrow = mysql_fetch_row(res_ptr);
        if(!sqlrow)
        {
            strclientmsg += "You have to register first";
        }
        else
        {
            
            if(strcmp(sqlrow[0], password) == 0)
            {

                strclientmsg += "LOG IN SUCCESS";

                std::cout<<conn->name();
                //登录成功
                mysql_free_result(res_ptr);
                memset(query, 0, sizeof(query));
                auto it=connMap.find(conn);
                int connid=it->second;
                sprintf(query,"select connid from UserInfo where username='%s'",name);
				int ret=sqlQuery(query);
                if(ret == -1)
                {
                    LOG_ERROR<<"select error :"<<mysql_error(mysql);
                }
                res_ptr=mysql_store_result(mysql);
                sqlrow=mysql_fetch_row(res_ptr);
                if(connid!=atoi(sqlrow[0]))
                {
                    memset(query, 0, sizeof(query));
                    sprintf(query,"update UserInfo set connid=%d where username='%s'",connid,name);
                    ret=sqlQuery(query);
                    if(ret == -1)
                    {
                        LOG_ERROR<<"update error :"<<mysql_error(mysql);
                    }
                }
				memset(query,0,sizeof(query));
                sprintf(query,"update UserInfo set online=%d where username='%s'",1,name);
                sqlQuery(query);
            }
            else
            {
                strclientmsg += "password is wrong";
            }
            if(mysql_errno(mysql))
            {
                LOG_ERROR<<"Retrive error :"<<mysql_error(mysql);
            }

        }
        mysql_free_result(res_ptr);//一次store对应一次free，多次free会出现double free or corruption (!prev) core dumped!
    }
    else if(buff[0]==FRIENDSLIST)
    {
        char query[100];
        char name[20];
        std::string rec(buff);
        if(rec.size()==0)
        {
            std::cout<<"friendlist substr error"<<std::endl;
        }
        //std::cout<<"FriendList ::rec "<<rec<<std::endl;
        strcpy(name,rec.substr(1,rec.size()-1).c_str());
        //std::cout<<"name "<<name<<std::endl;
        memset(query,0,sizeof(query));
        sprintf(query,"select *from %s",name);
        int ret=sqlQuery(query);
        if(ret==-1)
        {
            LOG_ERROR<<"select friends error :"<<mysql_error(mysql);
        }
        res_ptr=mysql_store_result(mysql);
        int rows=(int)mysql_num_fields(res_ptr);

        std::string friendsmsg;
        int count=0;
        std::string tempstr;
        while((sqlrow=mysql_fetch_row(res_ptr)))
        {
            int i;
            for(i=0;i<rows;i++)
            {
                friendsmsg+=sqlrow[i];
                friendsmsg+=":";

                tempstr+=friendsmsg;

                friendsmsg.clear();
            }
            count++;
        }
        char str[5];
        sprintf(str,"%d",count);
        tempstr+=str;
        //std::cout<<"Friendlist tempstr: "<<tempstr<<std::endl;
        conn->send(tempstr);
        mysql_free_result(res_ptr);

    }
    //ONLINESTATE
    else if(buff[0]==ONLINESTATE)
    {

        char query[100];
        sprintf(query,"select username,online,pic from UserInfo");
        sqlQuery(query);
        res_ptr=mysql_store_result(mysql);
        int count=0;
        std::string namemsg;
        std::string onlinemsg;
        std::string picmsg;
        std::string allmsg;
        std::string tempstr;
        while((sqlrow=mysql_fetch_row(res_ptr)))
        {
            namemsg+=sqlrow[0];
            onlinemsg+=sqlrow[1];
            picmsg+=sqlrow[2];
            allmsg+=namemsg+","+onlinemsg+","+picmsg+",:";
            tempstr+=allmsg;

            namemsg.clear();
            onlinemsg.clear();
            picmsg.clear();
            allmsg.clear();
            count++;
        }
        char str[5];
        sprintf(str,"#%d#",count);
        tempstr+=str;
		//std::cout<<tempstr<<std::endl;
        conn->send(tempstr);
        mysql_free_result(res_ptr);
    }
    else if(buff[0]==PICCHANGE)
    {
        std::cout<<"PICHANGE buff: "<<buff<<std::endl;
        std::string rec(buff);
        if(rec.size()==0)
        {
            std::cout<<"PICCHANGE substr error"<<std::endl;
        }
        auto it=rec.find(':');
        char name[20];
        int pic,ret;
        char query[100];
        std::cout<<"PICCHANGE rec: "<<rec<<std::endl;
        strcpy(name,rec.substr(1,it-1).c_str());
        std::cout<<"name "<<name<<std::endl;
        pic=atoi(rec.substr(it+1,rec.size()-it-1).c_str());
        std::cout<<"pic "<<pic<<std::endl;
        sprintf(query,"update UserInfo set pic=%d where username='%s'",pic,name);
        ret=sqlQuery(query);
        if(ret==-1)
        {
            LOG_ERROR<<"update pic error :"<<mysql_error(mysql);
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
        sqlQuery(query);
        res_ptr = mysql_store_result(mysql);
        sqlrow = mysql_fetch_row(res_ptr);
        mysql_free_result(res_ptr);
        if(!sqlrow)
        {
            strclientmsg+="The name is wrong";
        }
        else
        {
            memset(query,0,sizeof(query));
            sprintf(query,"select friends from %s where friends='%s'",fromname,toname);
            sqlQuery(query);
            res_ptr=mysql_store_result(mysql);
            sqlrow=mysql_fetch_row(res_ptr);
            mysql_free_result(res_ptr);
            if(sqlrow)
            {
                strclientmsg+="you are have been friends!";
            }
            else
            {
                memset(query,0,sizeof(query));
                sprintf(query,"select connid from UserInfo where username='%s'",toname);
                sqlQuery(query);
                res_ptr=mysql_store_result(mysql);
                sqlrow=mysql_fetch_row(res_ptr);
                int connid=atoi(sqlrow[0]);
                muduo::net::TcpConnectionPtr Connection;
                for(auto it=connMap.begin();it!=connMap.end();++it)
                {
                    if(it->second==connid)
                    {
                        Connection=it->first;
                    }
                }
                std::string msg;
                msg+="*AdDR. ";

                char tempstr[20];
                sprintf(tempstr,"%s+%s",fromname,toname);
                msg+=tempstr;
                //std::cout<<"msg: "<<msg<<std::endl;
                Connection->send(msg);
                strclientmsg+="message has been sended";
                mysql_free_result(res_ptr);
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
        sprintf(query,"select connid from UserInfo where username='%s'",fromname);
        ret=sqlQuery(query1);
        ret=sqlQuery(query2);
        sqlQuery(query);
        res_ptr=mysql_store_result(mysql);
        sqlrow=mysql_fetch_row(res_ptr);
        int connid=atoi(sqlrow[0]);
        muduo::net::TcpConnectionPtr Connection;
        for(auto it=connMap.begin();it!=connMap.end();++it)
        {
            if(it->second==connid)
            {
                Connection=it->first;
            }
        }
        if(ret == -1)
        {
            LOG_ERROR<<"insert error :"<<mysql_error(mysql);
        }
        std::string str="*ACCEPT.";

        Connection->send(str);
        strclientmsg+=buff;
        mysql_free_result(res_ptr);
    }
    else if(buff[0]==REFUSE)
    {

        std::string rec(buff);
        char fromname[20];
        char query[100];
        int ret;
        strcpy(fromname,rec.substr(1,rec.size()-1).c_str());
        sprintf(query,"select connid from UserInfo where username='%s'",fromname);
        ret=sqlQuery(query);
        if(ret==-1)
        {
            LOG_ERROR<<"select error :"<<mysql_error(mysql);
        }
        res_ptr=mysql_store_result(mysql);
        sqlrow=mysql_fetch_row(res_ptr);
        if(!sqlrow)
        {
            LOG_INFO<<"server msg:"<<"there is no user called "<<fromname<<" in database UserInfo";
        }
        int connid=atoi(sqlrow[0]);
        muduo::net::TcpConnectionPtr Connection;
        for(auto it=connMap.begin();it!=connMap.end();++it)
        {
            if(it->second==connid)
            {
                Connection=it->first;
            }
        }
        std::string msg="your requet had been refused...";
        Connection->send(msg);
        mysql_free_result(res_ptr);
    }
    else
    {
        strclientmsg+=buff;
    }

    return string(strclientmsg);

}
