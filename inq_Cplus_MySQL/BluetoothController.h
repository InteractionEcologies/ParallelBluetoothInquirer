#include <iostream>
#include <bluetooth/bluetooth.h> //BlueZ only supplies C language
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/* Communicate with a RESTful server using HTTP request provides by curl */
#include <curl/curl.h>

#include <cerrno>
#include <cstdlib>
#include <time.h>
#include <string>

#include <sys/socket.h>

using namespace std;

#define ENT(e) (int)(sizeof(e)/sizeof(char*))

//using namespace std;

class BluetoothController
{
public:
	BluetoothController(int numberOfAdaptors, int inquiryTimeLength);
	BluetoothController(int argc, char* argv[]);
	~BluetoothController();
	void run();
	

private:
	int debug;

	unsigned char msgBuffer[HCI_MAX_FRAME_SIZE];
	unsigned char controlBuffer[100];
	
	CURL *curl;
	CURLcode res;	
	const static char * serverUpdateVisitors;
	const static char * roomName;

	int * btSocketArray;

	int experimentIndex;
	int experimentDistance;
	string  experimentInquiryScanWindow;
	int experimentNumOfNearbyDevices; 
	int numberOfAdaptors; // = number of inquirers
	int inquiryTimeLength;
		


	const static char * majors[];// = {"Misc", "Computer"};
	const static char * computers[];
	const static char * phones[];
	const static char * av[];
	const static char * peripheral[];
	const static char * wearable[];
	const static char * toys[];

	int decodeBTEvent(int msgLengthFromBTSocket, struct timeval timeStamp);
	
	//Overloading
	void decodeBTMsg(inquiry_info * btMsg, struct timeval timeStamp);
	void decodeBTMsg( inquiry_info_with_rssi * btMsg, struct timeval timeStamp);
	void getClassInfo(uint8_t dev_class[3], char * classInfo);
};
