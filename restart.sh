#! /bin/bash

while true 
do
	monitor=`ps -ef | grep main | grep -v grep | wc -l ` 
	if [ $monitor -eq 0 ] 
	then
		echo "TcpChatRoomServer program is not running, restart it"
		./main 2019 
	else
		echo "TcpChatRoomServer program is running"
	fi
	sleep 1
done																		
