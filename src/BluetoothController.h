#include <iostream>
#include <bluetooth/bluetooth.h> //BlueZ only supplies C language
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/* Communicate with a RESTful server using HTTP request provides by curl */
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Easy.hpp>
#define MY_PORT 8000 

#include <cerrno>
#include <cstdlib>
#include <time.h>
#include <string>
#include <sstream>
#include <sys/socket.h>

#include <map>
#include <utility>

//Redis
#include "hiredis.h" 

//Json Spirit 
#include "json_spirit.h"
#include "json_spirit_stream_reader.h"

//Lexical cast from boost
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace json_spirit;
using namespace boost;
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

	// cURLpp: HttpRequest to get a username from the server 	
	const static string getUsernameFromServerUrl;
	const static int serverPortNumber;
	
	// Redis, bluetooth inquirer needs to put the user and a timestamp to a presence_bluetooths_HS { key="user", value="timestamp" }
	redisContext *rc;
	redisContext *rcLocal;
	redisReply *reply;
	const static string redisServerName;
	const static string redisLocalServerName;

	const static int redisPortNumber;
	const static string redisNearbyBTUsersHS;
	const static string redisNearbyBTUsersCH;	

	const static string redisLocalMacToUsernameHS;
	// A table cached all the user and mac_add mappings 
	map<string, string> UsernameCache; // <mac_addr, username>	

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


	/** HttpRequest a username from the server using a bluetooth mac_addr
	*   Also add it to the UsernameCache 
	*	mac_addr: a mac_addr
	*   	@return username if the mac_addr is registed in the server
	*		Null	otherwise
	*/
	string getUsernameFromServer(string mac_addr);
	
	/** Get a username from UsernameCache using a bluetooth mac_addr
	*	mac_addr: a mac_addr
	*	@return username if a username is existed in the UsernameCache,
	*		Null	 otherwise
	*/ 
	string getUsernameFromCache(string mac_addr);


	string getUsernameFromLocalRedis(string mac_addr);

	/** Get a username from mac_addr
	*	First check the UsernameCache, 
	*	If it is not there, request the server to get the username
	*	@return username if a username is register in the server
	*		Null	otherwise
	*/
	string getUsername(string mac_addr);


	/** Update current_user_HS (redis)
	*	no return value, the inquirer doesn't handle if the httprequest is faile, 	
	*	it will just keep update the timestamp of the current_users_HS
	*	current_users_HS = "{ 'username" : "timestamp" }"
	*/
	void updateNearbyBTUsersInRedis(string username, struct timeval timestamp);

	/** HttpResponse JSON Parser
	*	jsonParser
	*
	*/
	string jsonParser(stringstream &json);
};
