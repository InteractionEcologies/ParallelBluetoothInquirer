#include <iostream>
#include "BluetoothController.h"

using namespace std;


int main(int argc, char* argv[])
{
	/*
	* argv[1] = i (index)
	* argv[2] = distance
	* argv[3] = inquiryScanWindow
	* argv[4] = numOfNearbyDevices
	* argv[5] = numOfInquirers
	* argv[6] = inquiryTimeLength
	*/	
	
	if(argc > 1){
		if(argc != 7){
			cout << "Argument Format Wrong" << endl;
			cout << "The right sequence is:" << endl;
			cout << "i, distance, inquiryScanWindow, numOfNearbyDevices, numOfInquirers, inquiryTimeLength" << endl;
			return 1;
		} else {
			BluetoothController * btController = new BluetoothController(argc, argv);
			btController->run();
			
			delete btController;
		}
	} else {
		//default setting
		//BluetoothController(numOfNearbyDevices, inquiryTimeLength
		BluetoothController * btController = new BluetoothController(6, 10);
		btController->run();

		delete btController;
	}
	
	return 0;
}
