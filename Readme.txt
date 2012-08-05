This program utilizes multiple bluetooth adaptors to do the device inquiry in parallel, which is used to speed up the device inquiry process. 

Install on Linux 
1. sudo apt-get install libbluetooth-dev 
2. sudo apt-get install mysql-server
3. sudo apt-get install mysql-client
4. sudo apt-get install libmysqlclient-dev

Database and Table Structures
1. Table: btSignalRecords
{
  i: int(11), NULL, 
  timeDate: datetime, NULL
  timeStamp: decimal(17,3), NULL
  timeUSec: int(11), NULL
  machineID: varchar(30), NULL
  rssi: int(11), NULL
  deviceClass:  varchar(30), NULL
  location: varchar(30), NULL
}

sql:CREATE TABLE btSignalRecords (i int(11), timeDate datetime, timeStamp decimal(17,3), timeUSec int(11), 
    machineID varchar(30), rssi int(11), deviceClass varchar(30), location varchar(30)); 