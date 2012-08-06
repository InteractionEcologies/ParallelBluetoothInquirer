inq: main.o BluetoothController.o
	g++ -Wall -o inquiry.out main.o BluetoothController.o -lbluetooth \
		-I/usr/include/mysql -L/usr/lib/mysql -lmysqlclient -lcurl

main.o: main.cpp BluetoothController.h
	g++ -c -Wall -o main.o main.cpp -lbluetooth -I/usr/include/mysql -L/usr/lib/mysql -lmysqlclient -lcurl

BluetoothController.o: BluetoothController.cpp BluetoothController.h
	g++ -c -Wall BluetoothController.cpp -lbluetooth -L/usr/lib/mysql -lmysqlclient -I/usr/include/mysql -lcurl
