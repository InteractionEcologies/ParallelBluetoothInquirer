#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <time.h> //Used to convert time from timestamp to human readable time

#include <mysql.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <string.h>

int debug = 1;

static unsigned char msgBuffer[HCI_MAX_FRAME_SIZE];
static unsigned char controlBuffer[100];


static int decode(int msgLengthFromBTSocket, struct timeval timeStamp);
void decodeBTMsg(inquiry_info * btMsg, struct timeval timeStamp);
void decodeBTMsgWithRSSI(inquiry_info_with_rssi * btMsg, struct timeval timeStamp);
void getClassInfo(uint8_t dev_class[3], char * classInfo);

MYSQL *conn;


#define ENT(e) (sizeof(e)/sizeof(char*))
static char *majors[] = {"Misc", "Computer", "Phone", "Net Access", "Audio/Video",\
                        "Peripheral", "Imaging", "Wearable", "Toy"};

static char* computers[] = {"Misc", "Desktop", "Server", "Laptop", "Handheld",\
                                "Palm", "Wearable"};

static char* phones[] = {"Misc", "Cell", "Cordless", "Smart", "Wired",\
                        "ISDN", "Sim Card Reader For "};

static char* av[] = {"Misc", "Headset", "Handsfree", "Reserved", "Microphone", "Loudspeaker",\
                "Headphones", "Portable Audio", "Car Audio", "Set-Top Box",\
                "HiFi Audio", "Video Tape Recorder", "Video Camera",\
                "Camcorder", "Video Display and Loudspeaker", "Video Conferencing", "Reserved",\
                "Game / Toy"};

static char* peripheral[] = {"Misc", "Joystick", "Gamepad", "Remote control", "Sensing device",\
                "Digitiser Tablet", "Card Reader"};

static char* wearable[] = {"Misc", "Wrist Watch", "Pager", "Jacket", "Helmet", "Glasses"};

static char* toys[] = {"Misc", "Robot", "Vehicle", "Character", "Controller", "Game"};








int main()
{
	int btSocket;
	int optionValue;
	struct hci_filter eventFilter;

	struct iovec iov; //I/O Vector, a scatter-gather array for sockets
	struct msghdr msgFromBTSocket;
	struct cmsghdr * cmsgFromBTSocket; //A pointer to the control messages
	struct timeval timeStamp;
	int msgLengthFromBTSocket;

	periodic_inquiry_cp hciCmdParamInfo;
	periodic_inquiry_cp * hciCmdParam = &hciCmdParamInfo;

	int inquiryLength = 21;

	//File Descriptor
	fd_set readSet; 
	fd_set writeSet;
	fd_set exceptSet;

	//Program begin

	//Database connection
	conn = mysql_init(NULL);
	if (conn == NULL){
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	//mysql_real_connect( ..., database name, port number, unix socket, client flag)
	if (mysql_real_connect(conn, "localhost", "multipleBT", "multipleBT", NULL, 0, NULL, 0) == NULL ){
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	if (mysql_query( conn, "USE BTSignalRecords")){
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	//initialize msgFromBTSocket
	//assign msgBuffer to be the space for btSocket to send and receive message
	iov.iov_base = &msgBuffer;
	iov.iov_len = sizeof(msgBuffer);
	msgFromBTSocket.msg_iov = &iov; 	//scatter-gatther array 
										//struct iovec	
	msgFromBTSocket.msg_iovlen = 1;		// # elements in msg_iov
	msgFromBTSocket.msg_control = &controlBuffer;
	msgFromBTSocket.msg_controllen = sizeof(controlBuffer);

	//Open a bluetooth adaptor and connect it to a socket
	//int hci_open_dev(int dev_id);
	btSocket = hci_open_dev(0);
	

	
	/*Set HCI_DATA_DIR to true, the msg return will include data direction*/
	//int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
	
	optionValue = 1;
	setsockopt(btSocket, SOL_HCI, HCI_DATA_DIR, &optionValue, sizeof(optionValue));

	/*Set HCI_TIME_STAMP*/
	optionValue = 1;
	setsockopt(btSocket, SOL_HCI, HCI_TIME_STAMP, &optionValue, sizeof(optionValue));
	
	hci_filter_clear(&eventFilter);
	//Set the filter to only receive HCI_EVENT_PKT
	//This specifies what events we want on our socket.
	hci_filter_set_ptype(HCI_EVENT_PKT, &eventFilter);

	//We want all the events related to HCI 
	hci_filter_all_events(&eventFilter);

	//Set HCI_FILTER
	setsockopt(btSocket, SOL_HCI, HCI_FILTER, &eventFilter, sizeof(eventFilter));


	//Set Periodic Inquiry Parameters
	hciCmdParam->num_rsp = 0x00;	//Unlimited number of responses
	
	hciCmdParam->lap[0] = 0x33;		//Bluetooth's magic number
	hciCmdParam->lap[1] = 0x8b;
	hciCmdParam->lap[2] = 0x9e;

	hciCmdParam->length = inquiryLength;
	hciCmdParam->min_period = hciCmdParam->length + 1;
	hciCmdParam->max_period = hciCmdParam->min_period + 1;

	//Begin Periodic Inquiry
	hci_send_cmd(btSocket, OGF_LINK_CTL, OCF_PERIODIC_INQUIRY, PERIODIC_INQUIRY_CP_SIZE, hciCmdParam);


	while (1){
		//Listen the responses from the HCI socket
		
		FD_ZERO(&readSet);

		//Set btSocket to the file descriptor set we read
		FD_SET(btSocket, &readSet);

		//int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
		select(btSocket+1, &readSet, NULL, NULL, NULL);	

		if(FD_ISSET(btSocket, &readSet))
		{
			debug && printf("btSocket receives a message.\n");
			msgLengthFromBTSocket = recvmsg(btSocket, &msgFromBTSocket, 0 ); // 0 = no flags

			debug && printf("msgLengthFromBTSocket = %d\n", msgLengthFromBTSocket);		

			if(msgLengthFromBTSocket < 0){
				//put how to exit the program here
				debug && printf("msgLengthFromBTSocket < 0, exit program\n");
			} 
		
			else {
				debug && printf("msgLengthFromBTSocket >= 0:\n");
				if( msgLengthFromBTSocket >= 0){
					
					cmsgFromBTSocket = CMSG_FIRSTHDR( &msgFromBTSocket ); //cmsg = control messages
					while (cmsgFromBTSocket) {
						switch (cmsgFromBTSocket->cmsg_type){
						case HCI_CMSG_DIR:
							//Do something ...
							debug && printf("Receive HCI_CMSG_DIR\n");
							break;
						case HCI_CMSG_TSTAMP:
							debug && printf("Receove HCI_CMSG_TSTAMP\n");

							//CMSG_DATA returns a pointer to the data portion of the cmsg
							//Here, it returns a pointer to the the time stamp data
							//So we point to this time stamp data, and dereference it.  
							timeStamp = *( (struct timeval *) CMSG_DATA(cmsgFromBTSocket) );
							break;
						}

						//Look at next cmsghdr
						cmsgFromBTSocket = CMSG_NXTHDR( &msgFromBTSocket, cmsgFromBTSocket);
					}
				
				//Begin decode the packets (messages)
				debug && printf("Get into decode()\n");
				decode( msgLengthFromBTSocket, timeStamp);

				}
			}
		}
	}//end while()
	
	mysql_close(conn);
	return 0;
}



static int decode(int msgLengthFromBTSocket, struct timeval timeStamp)
{
	int i;	

	if( msgLengthFromBTSocket < 1){
		//Packet too small
		perror("Packet too small\n"); //print error messages to std error 
		return EXIT_SUCCESS;
	}

	if( msgBuffer[0] == HCI_EVENT_PKT ){
		debug && printf("Message received == HCI_EVENT_PKT\n");
		
		switch( msgBuffer[1] ){
			case EVT_INQUIRY_COMPLETE:
				debug && printf("Message received == EVT_INQUIRY_COMPLETE\n");
				//Don't need to do anything here, the adaptors should keep inquiry
			break;
			case EVT_INQUIRY_RESULT:
				debug && printf("Message received == EVT_INQUIRY_RESULT\n");
				
				if(msgBuffer[2] == 0{
					break;
				} else if (msgBuffer[2] * sizeof(inquiry_info) + 1){
					perror("Inquiry Result event length wrong. \n");
					return EXIT_FAILURE;
				} else {
					for(i = 0; i < msgBuffer[3]; i++){
						//Decode and add into database
						decodeBTMsg( (inquiry_info*)&msgBuffer[4 + i * sizeof(inquiry_info)], timeStamp );	
				}
						
			break;
			case EVT_REMOTE_NAME_REQ_COMPLETE:
				//I don't think we need to use this event	
			break;
			case EVT_CMD_STATUS:
								

			break;
			case EVT_CMD_COMPLETE:
				debug && printf("Message received == EVT_CMD_COMPLETE\n");
				
			break;
			case EVT_INQUIRY_RESULT_WITH_RSSI:
				debug && printf("Message received == EVT_INQUIRY_RESULT_WITH_RSSI\n");		

				if (msgBuffer[2] == 0){
					//Message length = 0
					break;
				} else if (msgBuffer[2] != ( msgBuffer[3] * sizeof(inquiry_info_with_rssi) ) + 1 ){
					debug && printf("Inquiry Result With RSSI length wrong\n");
					return EXIT_FAILURE;
				} else {

					debug && printf("Success detect inquiry result events with RSSI, msgBuffer[3] = %d\n", msgBuffer[3]);
					//Success detect inquiry result events
					for(i = 0; i < msgBuffer[3]; i++){
						//Decode and add into database
						decodeBTMsgWithRSSI( (inquiry_info_with_rssi*)&msgBuffer[4 + i * sizeof(inquiry_info_with_rssi)], timeStamp );	
				
					}

				}

			break;
		}
	}	
	return 0;
}

void decodeBTMsg(inquiry_info * btMsg, struct timeval timeStamp)
{
	
	struct tm timeInDateTimeFormat;
	char timeStampStringContainsMicroSecond[80];
	char timeStringInDateTimeFormat[80];	
	char btMachineID[18];
	char queryStatement[256];
	inquiry_info template;

	template.bdaddr = btMsg->bdaddr;
	template.dev_class[0] = btMsg->dev_class[0];
	template.dev_class[1] = btMsg->dev_class[1];
	template.dev_class[2] = btMsg->dev_class[2];
	
	ba2str(&(btMsg->bdaddr), btMachineID);

	timeInDateTimeFormat = * localtime( &(timeStamp.tv_sec) ); 
	//convert the time to a human redable format
	strftime(timeStringInDateTimeFormat, sizeof(timeStringInDateTimeFormat), 
		"%Y-%m-%d %H:%M:%S", &(timeInDateTimeFormat)); 
				
	sprintf(timeStampStringContainsMicroSecond, "%lu.%03lu\n", timeStamp.tv_sec, timeStamp.tv_usec);
	debug && printf("timeStampStringContainsMicroSeccond = %s", timeStampStringContainsMicroSecond);
	debug && printf("@ %s : %06lu ... \n", timeStringInDateTimeFormat, timeStamp.tv_usec);						
	debug && printf("timeStamp.tv_sec = %8lu\n", timeStamp.tv_sec);
	
	//timeDate, timeStamp, timeUSec, machineID, rssi, deviceClass

	snprintf(queryStatement, 256, "INSERT INTO btSignalRecords VALUES ('%s', '%s', '%lu', '%s', '0', 'deviceClass'  )", timeStringInDateTimeFormat, timeStampStringContainsMicroSecond , timeStamp.tv_usec, btMachineID);

	mysql_query(conn, queryStatement);	

}

void decodeBTMsgWithRSSI(inquiry_info_with_rssi* btMsg, struct timeval timeStamp)
{
	struct tm timeInDateTimeFormat;
	char timeStampStringContainsMicroSecond[80];
	char timeStringInDateTimeFormat[80];	
	char btMachineID[18];
	char queryStatement[256];
	char classInfo[30];	
	inquiry_info_with_rssi template;

	

	template.bdaddr = btMsg->bdaddr;
	template.dev_class[0] = btMsg->dev_class[0];
	template.dev_class[1] = btMsg->dev_class[1];
	template.dev_class[2] = btMsg->dev_class[2];
	template.rssi = btMsg->rssi;
	
	ba2str(&(btMsg->bdaddr), btMachineID);
	debug && printf("Find a nearby bluetooth device:\n");
	debug && printf("	%s, rssi = %d\n", btMachineID, btMsg->rssi);

	//add to database here
	//...
	//mysql_query(conn, "");

	
	//mysql_query(conn, "INSERT INTO btSignalRecords VALUES(timeDate, timeStamp, timeUSec, machineID, rssi, deviceClass")

	timeInDateTimeFormat = * localtime( &(timeStamp.tv_sec) ); 
	//convert the time to a human redable format
	strftime(timeStringInDateTimeFormat, sizeof(timeStringInDateTimeFormat), 
		"%Y-%m-%d %H:%M:%S", &(timeInDateTimeFormat)); 
				
	sprintf(timeStampStringContainsMicroSecond, "%lu.%03lu\n", timeStamp.tv_sec, timeStamp.tv_usec);
	debug && printf("timeStampStringContainsMicroSeccond = %s", timeStampStringContainsMicroSecond);
	debug && printf("@ %s : %06lu ... \n", timeStringInDateTimeFormat, timeStamp.tv_usec);						
	debug && printf("timeStamp.tv_sec = %8lu\n", timeStamp.tv_sec);
	
	//timeDate, timeStamp, timeUSec, machineID, rssi, deviceClass

	classInfo[0] = '\0';
	getClassInfo(btMsg->dev_class, classInfo);

	debug && printf("classInfo = %s\n", classInfo);
	snprintf(queryStatement, 256, "INSERT INTO btSignalRecords VALUES ('%s', '%s', '%lu', '%s', '%d', '%s'  )", timeStringInDateTimeFormat, timeStampStringContainsMicroSecond , timeStamp.tv_usec, btMachineID, btMsg->rssi, classInfo);

	mysql_query(conn, queryStatement);	

}


void getClassInfo(uint8_t dev_class[3], char * classInfo)
{
	int flags = dev_class[2];
	int major = dev_class[1];
	int minor = dev_class[0] >> 2;
	char buffer[30];

	if (major > ENT(majors)) {
        if (major == 63)
           sprintf(buffer, " Unclassified device\n");
        	strcat( classInfo, buffer);
		return;
    }
    switch (major) {
    case 1:
        if (minor <= ENT(computers)) strcat( classInfo , computers[minor]);
        break;
    case 2:
        if (minor <= ENT(phones)) strcat( classInfo, phones[minor]);
        break;
    case 3:
        sprintf(buffer, " Usage %d/56", minor);
        strcat( classInfo, buffer);
		break;
    case 4:
        if (minor <= ENT(av)) sprintf(buffer," %s", av[minor]);
        strcat( classInfo, buffer);
		break;
    case 5:
        if ((minor & 0xF) <= ENT(peripheral)) sprintf(buffer," %s", peripheral[(minor & 0xF)]);
        if (minor & 0x10) sprintf(buffer, " with keyboard");
        if (minor & 0x20) sprintf(buffer, " with pointing device");
        strcat( classInfo, buffer);
		break;
    case 6:
        if (minor & 0x2) sprintf(buffer," with display");
        if (minor & 0x4) sprintf(buffer, " with camera");
        if (minor & 0x8) sprintf(buffer, " with scanner");
        if (minor & 0x10) sprintf(buffer, " with printer");
    	strcat( classInfo, buffer);
	break;
    case 7:
        if (minor <= ENT(wearable)) sprintf(buffer, " %s", wearable[minor]);
        strcat( classInfo, buffer);
		break;
    case 8:
        if (minor <= ENT(toys)) sprintf(buffer," %s", toys[minor]);
        strcat( classInfo, buffer);
		break;
    }
    sprintf(buffer, " %s", majors[major]);
	strcat( classInfo, buffer);
}
