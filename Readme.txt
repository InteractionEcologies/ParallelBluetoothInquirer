This program utilizes multiple bluetooth adaptors to do the device inquiry in parallel, which is used to speed up the device inquiry process. 

Install LAMP (Linux, Apache, MySQL and PHP)
reference: https://help.ubuntu.com/community/ApacheMySQLPHP

Install Dependencies on Linux 
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

MySQL User Manual :
1. Login MySQL database
  $ mysql -u <username> -p -h 127.0.0.1

2. Use a database, ex: BTSignalRecords
  -under mysql command line:  
  mysql>> USE BTSignalRecords;

3. Get all the data from a table, ex: The table name is btSignalRecords here. 
  mysql>> SELECT * FROM btSignalRecords

Bluetooth User Manual:
1. Go to Develop/multipleBT-Git/inq_Cplus_MySQL
  $cd multipltBT-Git/inq_Cplus_MySQL

2. Turn on Bluetooth Detecter
  sudo ./inquiry.out

3. Turn off Bluetooth Detecter
  3.1 First, close the program 
    ctrl + C
  3.2 Kill the process
    $ ps -a (find the PID for inquiry.out
    $ sudo kill -9 <PID>