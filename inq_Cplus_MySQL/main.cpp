#include <iostream>
#include "BluetoothController.h"

int main()
{
	BluetoothController * btController = new BluetoothController(6, 10);
	btController->run();

	delete btController;
	return 0;
}
