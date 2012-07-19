inq: main.o BluetoothController.o
	g++ -Wall -o inquiry.out main.o BluetoothController.o -lbluetooth -lcurl 

main.o: main.cpp BluetoothController.h
	g++ -c -Wall -o main.o main.cpp -lbluetooth -lcurl

BluetoothController.o: BluetoothController.cpp BluetoothController.h
	g++ -c -Wall BluetoothController.cpp -lbluetooth -lcurl 
