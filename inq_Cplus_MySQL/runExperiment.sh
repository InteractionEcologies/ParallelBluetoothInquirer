#!/bin/bash

table="paramList"
username="multipleBT"
password="multipleBT"
host="127.0.0.1"
database="BTSignalRecords"

#Loop through a set of parameters, 1-20, 21-40, 41-60, 61-80 ...
# i in {1..20}
for i in 1
do
	j=0;
	for param in "distance" "inquiryScanWindow" "numOfNearbyDevices" "numOfInquirers" "inquiryTimeLength"
	do
		query="SELECT $param FROM $table WHERE i=$i"
		p[$j]=$(echo $query | mysql $database -u $username -p$password -h $host | tail -n+2)
	
		echo ${p[$j]}

	j=$(expr $j + 1)	
	done

	./inquiry.out $i ${p[1]} ${p[2]} ${p[3]} ${p[4]} ${p[5]}
	
	#Get distance
	#query="SELECT distance FROM $table WHERE i=$i"
	#distance=$(echo $query | mysql $database -u $username -p$password -h $host | tail -n+2)

	#Get inquiryScanWindow
	#query="SELECT inquiryScanWindow FROM $table WHERE i=$i"
    #distance=$(echo $query | mysql $database -u $username -p$password -h $host | tail -n+2)
	#i, distance, inquiryScanWindow, numOfNearbyDevices, numOfInquirers, inquiryTimeLength"

done

