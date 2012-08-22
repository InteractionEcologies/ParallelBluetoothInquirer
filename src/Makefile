inq: main.o BluetoothController.o
	g++ -Wall -o inquiry.out main.o BluetoothController.o \
	-lbluetooth -lcurl -I/usr/local/include/hiredis/ -I/usr/local/include/curlpp/ \
	-L/usr/local/lib -lhiredis -lcurlpp

main.o: main.cpp BluetoothController.h
	g++ -c -Wall -o main.o main.cpp -lbluetooth -lcurl -I/usr/local/include/hiredis/ \
	-I/usr/local/include/curlpp -L/usr/local/lib -lhiredis -lcurlpp

BluetoothController.o: BluetoothController.cpp BluetoothController.h
	g++ -c -Wall BluetoothController.cpp -lbluetooth -lcurl -I/usr/local/include/hiredis/ \
	-I/usr/local/include/curlpp -L/usr/local/lib/ -lhiredis -lcurlpp 
