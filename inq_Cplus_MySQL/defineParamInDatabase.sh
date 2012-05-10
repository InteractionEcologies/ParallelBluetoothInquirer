#!/bin/bash

table="paramList"
username="multipleBT"
password="multipleBT"
host="127.0.0.1"
database="BTSignalRecords"

#echo "SELECT machineID FROM $table" | mysql $database -u $username -p$password -h $host 

echo "CREATE TABLE $table (i INT, distance INT, inquiryScanWindow VARCHAR(6), numOfNearbyDevices INT, numOfInquirers INT, inquiryTimeLength INT, UNIQUE (i));" | mysql $database -u $username -p$password -h $host
	#UNIQUE constraint set the i column to be unique


i=1
for distance in 2 5
do
	for inquiryScanWindow in "0x0012" "0x0800"
	do
		for numOfNearbyDevices in 10 5 1
		do
			for numOfInquirers in 6 4 3 2 1 
			do
				for inquiryTimeLength in 20 10 6 2
				do
				query="INSERT INTO $table VALUE('$i', '$distance', '$inquiryScanWindow', '$numOfNearbyDevices', '$numOfInquirers', '$inquiryTimeLength')"
				echo $query | mysql $database -u $username -p$password -h $host
				i=$(expr $i + 1)
				#Or, use i=((i++))
				done
			done
		done
	done
done

#echo "INSERT INTO $table VALUE('$i', '$distance', '$inquiryScanWindow', '$numOfNearbyDevices', '$numOfInquirers', '$inquiryTimeLength');" | mysql $database -u $username -p$password -h $host



#----- Retrieve data from database -----
#query="SELECT distance FROM $table WHERE i=1"

#distance=$(echo $query | mysql $database -u $username -p$password -h $host | tail -n+2)
  #The "tail" call ignores the first output line, which is the column name. Results begin on the second line, thus +2.

#echo $distance
