#! /bin/bash

while true 
do
	monitor=`ps -ef | grep main | grep -v grep | wc -l ` 
	if [ $monitor -eq 0 ] 
	then
		echo "TcpChatRoomServer program is not running, restart it"
		#.~/others/TcpChatRoomServer/main 2019 #这样就不行？？?原因未知
		./main 2019 
	else
		echo "TcpChatRoomServer program is running"
	fi
	sleep 1
done																		
