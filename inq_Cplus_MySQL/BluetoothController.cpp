#include "BluetoothController.h"

using namespace std;

//For MySQL database connection
const char* BluetoothController::username = "multipleBT";
const char* BluetoothController::password = "multipleBT";
const char* BluetoothController::host = "localhost";

const char* BluetoothController::serverUpdateVisitors = "http://dhcp2-236.si.umich.edu:8000/bluetooth/";

//Need to define the constant at cpp, cannot define within the prototype in .h file
const char* BluetoothController::majors[] = {"Misc", "Computer", "Phone", "Net Access", "Audio/Video",
                        "Peripheral", "Imaging", "Wearable", "Toy"};

const char* BluetoothController::computers[] = {"Misc", "Desktop", "Server", "Laptop", "Handheld",\
                                "Palm", "Wearable"};

const char* BluetoothController::phones[] = {"Misc", "Cell", "Cordless", "Smart", "Wired",\
                        "ISDN", "Sim Card Reader For "};

const char* BluetoothController::av[] = {"Misc", "Headset", "Handsfree", "Reserved", "Microphone", "Loudspeaker",\
                "Headphones", "Portable Audio", "Car Audio", "Set-Top Box",\
                "HiFi Audio", "Video Tape Recorder", "Video Camera",\
                "Camcorder", "Video Display and Loudspeaker", "Video Conferencing", "Reserved",\
                "Game / Toy"};

const char* BluetoothController::peripheral[] = {"Misc", "Joystick", "Gamepad", "Remote control", "Sensing device",\
                "Digitiser Tablet", "Card Reader"};

const char* BluetoothController::wearable[] = {"Misc", "Wrist Watch", "Pager", "Jacket", "Helmet", "Glasses"};

const  char* BluetoothController::toys[] = {"Misc", "Robot", "Vehicle", "Character", "Controller", "Game"};

BluetoothController::BluetoothController(int numberOfAdaptors, int inquiryTimeLength)
{
	debug = 1;


	this->table = "btSignalRecords";
	this->numberOfAdaptors = numberOfAdaptors;
	this->inquiryTimeLength = inquiryTimeLength;

	//For Experiment
	this->experimentIndex = 0;
	this->experimentDistance = -1;
	this->experimentInquiryScanWindow = "0x0000";
	this->experimentNumOfNearbyDevices = -1;

	debug && cout << "numberOfAdaptors = " << this->numberOfAdaptors << endl; 	
	debug && cout << "inquiryTimeLength = " << this->inquiryTimeLength << endl;

	//Data base connection
	conn = mysql_init(NULL);
	if(conn == NULL){
		cout << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << endl;
        exit(1);
	}

	//mysql_real_connect( ..., database name, port number, unix socket, client flag)
	if (mysql_real_connect(conn, host, username, password, NULL, 0, NULL, 0) == NULL ){
        cout << "Error " << mysql_errno(conn) << ": " <<  mysql_error(conn) << endl;
        exit(1);
    }

	//Connect to database "BTSignalRecords
    if (mysql_query( conn, "USE BTSignalRecords")){
        cout << "Error " << mysql_errno(conn) << ": " <<  mysql_error(conn) << endl;
        exit(1);
    }
	
	
	//struct msghdr msgBuffer;
	//Open N ( N = numberOfAdaptors ) bluetooth adaptors from adaptor 0 to N -1 
	//This might cause problem if some adaptors are not usable.
	//hci_open_dev( int dev_id) will return the socket the bluetooth connect to. 
	//We need to store the socket for future connection
	btSocketArray = new int[this->numberOfAdaptors];
	for(int i=0; i < this->numberOfAdaptors; i++){
		btSocketArray[i] = hci_open_dev(i);
		debug && cout << "btSocketArray[" << i << "] = " << btSocketArray[i] << endl;
	}

	
}


BluetoothController::BluetoothController(int argc, char* argv[])
{
	/*
    * argv[1] = i (index)
    * argv[2] = distance
    * argv[3] = inquiryScanWindow
    * argv[4] = numOfNearbyDevices
    * argv[5] = numOfInquirers
    * argv[6] = inquiryTimeLength
    */  
	
	debug = 1;

	this->table = "btSignalRecordsForExperiment";

	debug && cout << "Table = " << this->table << endl;
	//For Experiment
	this->experimentIndex = atoi(argv[1]);
	this->experimentDistance = atoi(argv[2]);
	this->experimentInquiryScanWindow = argv[3];
	this->experimentNumOfNearbyDevices = atoi(argv[4]);
	this->numberOfAdaptors = atoi(argv[5]);
	this->inquiryTimeLength = atoi(argv[6]);

	debug && cout << "Parameters for experiment" << endl;
	debug && cout << "Experiment Index = " << this->experimentIndex << endl;
	debug && cout << "Distance = " << this->experimentDistance << endl;
	debug && cout << "Inquiry Scan Window = " << this->experimentInquiryScanWindow << endl;
	debug && cout << "Number of Nearby Devices = " << this->experimentNumOfNearbyDevices << endl;
	debug && cout << "Number of Inquirers = " << this->numberOfAdaptors << endl; 	
	debug && cout << "inquiryTimeLength = " << this->inquiryTimeLength << endl;

	//Data base connection
	conn = mysql_init(NULL);
	if(conn == NULL){
		cout << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << endl;
        exit(1);
	}

	//mysql_real_connect( ..., database name, port number, unix socket, client flag)
	if (mysql_real_connect(conn, host, username, password, NULL, 0, NULL, 0) == NULL ){
        cout << "Error " << mysql_errno(conn) << ": " <<  mysql_error(conn) << endl;
        exit(1);
    }

	//Connect to database "BTSignalRecords
    if (mysql_query( conn, "USE BTSignalRecords")){
        cout << "Error " << mysql_errno(conn) << ": " <<  mysql_error(conn) << endl;
        exit(1);
    }
	
	
	//struct msghdr msgBuffer;
	//Open N ( N = numberOfAdaptors ) bluetooth adaptors from adaptor 0 to N -1 
	//This might cause problem if some adaptors are not usable.
	//hci_open_dev( int dev_id) will return the socket the bluetooth connect to. 
	//We need to store the socket for future connection
	btSocketArray = new int[this->numberOfAdaptors];
	for(int i=0; i < this->numberOfAdaptors; i++){
		btSocketArray[i] = hci_open_dev(i);
		debug && cout << "btSocketArray[" << i << "] = " << btSocketArray[i] << endl;
	}

}
BluetoothController::~BluetoothController()
{
	mysql_close(conn);
}

void BluetoothController::run()
{
	struct timeval timeStamp;

	//Set HCI_DATA_DIR, HCI_TIME_STAMP, and Event Filter For the assigned N Adaptors
	for(int i=0; i < this->numberOfAdaptors; i++){
		int optionValue = 1;
		setsockopt( btSocketArray[i], SOL_HCI, HCI_DATA_DIR, &optionValue, sizeof(optionValue));

		optionValue = 1;	
		setsockopt( btSocketArray[i], SOL_HCI, HCI_TIME_STAMP, &optionValue, sizeof(optionValue));
		
		struct hci_filter eventFilter;	
		hci_filter_clear(&eventFilter);
		
    	//Set the filter to only receive HCI_EVENT_PKT
    	//This specifies what events we want on our socket.
    	hci_filter_set_ptype(HCI_EVENT_PKT, &eventFilter);

    	//We want all the events related to HCI 
    	hci_filter_all_events(&eventFilter);

    	//Set HCI_FILTER
    	setsockopt(btSocketArray[i], SOL_HCI, HCI_FILTER, &eventFilter, sizeof(eventFilter));	
	}

	periodic_inquiry_cp hciCmdParamInfo;
	periodic_inquiry_cp * hciCmdParam = &hciCmdParamInfo;
	//Set Periodic Inquiry Parameters
	hciCmdParam->num_rsp = 0x00;
	
	hciCmdParam->lap[0] = 0x33;     //Bluetooth's magic number
    hciCmdParam->lap[1] = 0x8b;
    hciCmdParam->lap[2] = 0x9e;

    hciCmdParam->length = this->inquiryTimeLength;
    hciCmdParam->min_period = hciCmdParam->length + 1;
    hciCmdParam->max_period = hciCmdParam->min_period + 1;

	//Begin Periodic Inquiry
	for(int i = 0; i < this->numberOfAdaptors; i++){
		//Stop the periodic inquiry first so to make sure our periodic inquiry work!
		if( hci_send_cmd( btSocketArray[i], OGF_LINK_CTL, OCF_EXIT_PERIODIC_INQUIRY, 0, NULL)  < 0 ){
			cerr << "Cannot stop the periodic inquiry for adaptor " << i 
				<< " before our periodic inquiry" << endl;
			exit(1); 
		}

		if( hci_send_cmd( btSocketArray[i], OGF_LINK_CTL, 
			OCF_PERIODIC_INQUIRY, PERIODIC_INQUIRY_CP_SIZE, hciCmdParam) < 0 ){
			cerr << "Cannot request periodic inquiry." << endl;
			exit(1);
		}
	}

	//File Descriptor
	fd_set readSet;
	//fd_set writeSet;
	//fd_set exceptSet;

	while(1){
		//Listen to the responses from the HCI Socket
		FD_ZERO( &readSet);
		
		for(int i = 0; i < this->numberOfAdaptors; i++){
			FD_SET(btSocketArray[i], &readSet);
			//cerr << "FD_SET, any error here? : " << strerror(errno) << endl;		
		}
	
		//int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
		// see which sockets get responses, 
		// nfds = the larget socket + 1
		debug && cout << "btSocketArray[N] = " << btSocketArray[this->numberOfAdaptors-1] << endl;
		select( (btSocketArray[(this->numberOfAdaptors)-1]) + 1, &readSet, NULL, NULL, NULL);
		cerr << "select, any error here? : " << strerror(errno) << endl;		
			
		//Check all Sockets for bluetooth adaptors to see whether they receive any message 
		for(int i= 0; i < this->numberOfAdaptors; i++){
			if(FD_ISSET(btSocketArray[i], &readSet))
        	{
						
				cerr << "FD_ISSET, any error here? : " << strerror(errno) << endl;		
            	debug && cout << "btSocket[" << i << "] " <<  "receives a message." << endl;

				//Variables used to store message from Bluetooth Socket
				struct msghdr msgFromBTSocket;
				struct iovec iov;
				
				memset( &msgFromBTSocket, 0, sizeof(msgFromBTSocket));
				//initialize msgFromBTSocket

    			//assign msgBuffer to be the space for btSocket to send and receive message
    			iov.iov_base = &(this->msgBuffer);
    			iov.iov_len = sizeof(this->msgBuffer);

    			msgFromBTSocket.msg_iov = &iov;     //scatter-gatther array 
            			                            //struct iovec 
 
   				msgFromBTSocket.msg_iovlen = 1;     // # of elements in msg_iov
    			msgFromBTSocket.msg_control = &(this->controlBuffer);
    			msgFromBTSocket.msg_controllen = sizeof(this->controlBuffer);

            	int msgLengthFromBTSocket = recvmsg(btSocketArray[i], &msgFromBTSocket, 0 ); // 0 = no flags

				debug && cout << "msgLengthFromBTSocket = "<<  msgLengthFromBTSocket << endl;

    	        if(msgLengthFromBTSocket < 0){//put how to exit the program here
					debug && cout << "msgLengthFromBTSocket < 0, exit program." << endl;
            		debug && cout << "Error: " << strerror(errno) << endl;


					exit(1);
				} else {
					debug && cout << "msgLengthFromBTSocket >= 0." << endl;
             

					
                    //Check the received message' control header to see what  
					struct cmsghdr * cmsgFromBTSocket;
					cmsgFromBTSocket = CMSG_FIRSTHDR( &msgFromBTSocket ); //cmsg = control messages


					// We only need to use HCI_CMSG_TSTAMP here
					while (cmsgFromBTSocket) {
                        switch (cmsgFromBTSocket->cmsg_type){
                        case HCI_CMSG_DIR:
                            //Do something ... we don't need to use this now
                            debug && printf("Receive HCI_CMSG_DIR\n");
                            break;
                        case HCI_CMSG_TSTAMP:
                            debug && printf("Receove HCI_CMSG_TSTAMP\n");

							//Store the timestamp of the bluetooth message from BT adaptor.
                            //CMSG_DATA returns a pointer to the data portion of the cmsg
                            //Here, it returns a pointer to the the time stamp data
                            //So we point to this time stamp data, and dereference it.  
                            timeStamp = *( (struct timeval *) CMSG_DATA(cmsgFromBTSocket) );
                            debug && cout << "timeStamp.tv_sec = " << timeStamp.tv_sec << endl;
							break;
                        }

                        //Look at next cmsghdr
                        cmsgFromBTSocket = CMSG_NXTHDR( &msgFromBTSocket, cmsgFromBTSocket);
					}//end while(cmsgFromBTSocket)
					
					//begin decode the packets
					debug && cout << "Begin to decode which event we get from bluetooth adaptor" << endl;
					decodeBTEvent( msgLengthFromBTSocket, timeStamp);	

				} //end if-and-else
			}//end FD_ISSET				
		}//end For-loop for N bluetooth Adaptors
	}//end while(1)
}

int BluetoothController::decodeBTEvent(int msgLengthFromBTSocket, struct timeval timeStamp)
{
	if( msgLengthFromBTSocket < 1){
        //Packet too small
        perror("Packet too small\n"); //print error messages to std error 
        return EXIT_SUCCESS;
    }
	
	if( this->msgBuffer[0] == HCI_EVENT_PKT){
		debug && cout << "Message received == HCI_EVENT_PKT" << endl;

		switch( this->msgBuffer[1] ){
            case EVT_INQUIRY_COMPLETE:
                debug && cout << "Message received == EVT_INQUIRY_COMPLETE" << endl;
                //Don't need to do anything here, the adaptors should keep inquiry
            break;
            case EVT_INQUIRY_RESULT:
                debug && cout << "Message received == EVT_INQUIRY_RESULT" << endl;

                if( this->msgBuffer[2] == 0){
                    break;
                } else if ( this->msgBuffer[2] != (this->msgBuffer[3] * sizeof(inquiry_info) + 1)){
                    perror("Inquiry Result event length wrong. \n");
                    return EXIT_FAILURE;
                } else {
                    for(int i = 0; i < this->msgBuffer[3]; i++){
                        //Decode and add into database
                        decodeBTMsg( (inquiry_info*)&(this->msgBuffer[4 + i * sizeof(inquiry_info)]), timeStamp );
               		 }
				}

            break;
            case EVT_REMOTE_NAME_REQ_COMPLETE:
                //I don't think we need to use this event   
            break;
            case EVT_CMD_STATUS:
				//This is use for paging, so we don't need to care about this here 
			break;
			case EVT_CMD_COMPLETE:
                debug && cout << "Message received == EVT_CMD_COMPLETE" << endl;
			
				//msgBuffer[2] := the length of the packet
				if( this->msgBuffer[2] < sizeof(evt_cmd_complete) ){
					cerr << "Command Complete event length wrong." << endl;
					return EXIT_FAILURE;
				}//end if

				switch(  ((evt_cmd_complete*) &this->msgBuffer[3])->opcode){
					// Two cases:
					//	(1) OCF_PERIODIC_INQUIRY
					//  (2) OCF_EXIT_PERIOIDIC_INQUIRY
					case cmd_opcode_pack(OGF_LINK_CTL, OCF_PERIODIC_INQUIRY):
						errno = bt_error( this->msgBuffer[3 + sizeof(evt_cmd_complete)] );	
						cerr << "Periodic inquiry requested: " << strerror(errno) << endl;	
	
						//Three cases:
						//	(1) event length wrong
						//  (2) The bluetooth adaptor refused setting periodic mode. 
						//		It may already be in periodic mode.
						//  (3) errno > 0: Can't start periodic mode.
						
						//	(1)
						if( this->msgBuffer[2] != sizeof(evt_cmd_complete) + 1	){//(1)
							cerr << "Periodic Inquiry Command Complete event length wrong" << endl;
							return EXIT_FAILURE;	
						} 
						
						//	(2)
						if( this->msgBuffer[3 + sizeof(evt_cmd_complete)] == HCI_COMMAND_DISALLOWED){
							cerr << "The adaptor refuses to set in periodic mode. " 
								 << "It may already be in periodic mode, "
								 <<	"but the inquiry window (inquiryTimeLength) might be different."
								 << endl;
	
							//cerr << "Try to stop periodic mode, and restart the adaptor again" << endl;
		 
						} else {
							if(errno > 0){
								cerr << "Can't Start Periodic mode." << endl;
								return EXIT_FAILURE;
							} else {
								debug && cout << "Successfully activate Periodic Inquiry Mode for one adaptor!" 
											 << endl;
							}
						}
						//debug && cout << "END OF EVT_CMD_COMPLETE handle" << endl;
					break;
					case cmd_opcode_pack(OGF_LINK_CTL, OCF_EXIT_PERIODIC_INQUIRY):
						errno = bt_error( this->msgBuffer[3 + sizeof(evt_cmd_complete)] );
						cerr << "Exit Periodic inquiry requested. " << strerror(errno) << endl;

						//Two cases:
						// (1) Exit Periodic Inquiry Command Complete event length wrong
						// (2) HCI_COMMAND_DISALLOWED
						
						//	(1)
						if( this->msgBuffer[2] != sizeof(evt_cmd_complete) + 1){
							cerr << "Exit periodic Inquiry Command Complete event length wrong." << endl;
						}

						if( this->msgBuffer[ 3 + sizeof(evt_cmd_complete)] == HCI_COMMAND_DISALLOWED) {					
							cerr << "The adaptor refuses to set in periodic mode. " 
								 << "It may already be in periodic mode, "
								 <<	"but the inquiry window (inquiryTimeLength) might be different." 
								 << endl;
						} else {
							if(errno > 0){
                                cerr << "Can't Start Periodic mode." << endl;
                                return EXIT_FAILURE;
                            }
						}
			
					break;
				}

            break;
            case EVT_INQUIRY_RESULT_WITH_RSSI:
                debug && cout << "Message received == EVT_INQUIRY_RESULT_WITH_RSSI" << endl;

                if (this->msgBuffer[2] == 0){
                    //Message length = 0
                    break;
                } else if (this->msgBuffer[2] != ( this->msgBuffer[3] * sizeof(inquiry_info_with_rssi) ) + 1 ){
                    debug && cout << "Inquiry Result With RSSI length wrong" << endl;
                    return EXIT_FAILURE;
                } else {
                
					//Success detect inquiry result events
					for(int i = 0; i < this->msgBuffer[3]; i++){
               		
					//Decode and add into database
						decodeBTMsg( 
							(inquiry_info_with_rssi*) &(this->msgBuffer[4 + i * sizeof(inquiry_info_with_rssi)]), 
							timeStamp );
                    }
                } 
			break;
			} //switch-case break		
	}// 

	return EXIT_SUCCESS;	
}

void BluetoothController::decodeBTMsg( inquiry_info * btMsg, struct timeval timeStamp)
{
	struct tm timeInDateTimeFormat;
    char timeStampStringContainsMicroSecond[80];
    char timeStringInDateTimeFormat[80];
    char btMachineID[18];
    char queryStatement[256];

	char httpputUrl[256];


    ba2str(&(btMsg->bdaddr), btMachineID);
	debug && cout << "find a nearby bluetooth devices: " << btMachineID << endl;

	/** Store BT Signal to MySQL Database **/	
    timeInDateTimeFormat = * localtime( &(timeStamp.tv_sec) );
    //convert the time to a human redable format
    strftime(timeStringInDateTimeFormat, sizeof(timeStringInDateTimeFormat),
        "%Y-%m-%d %H:%M:%S", &(timeInDateTimeFormat));

    sprintf(timeStampStringContainsMicroSecond, "%lu.%03lu\n", timeStamp.tv_sec, timeStamp.tv_usec);
    debug && printf("timeStampStringContainsMicroSeccond = %s", timeStampStringContainsMicroSecond);
    debug && printf("@ %s : %06lu ... \n", timeStringInDateTimeFormat, timeStamp.tv_usec);
    debug && printf("timeStamp.tv_sec = %8lu\n", timeStamp.tv_sec);
	debug && printf("timeStamp.tv_usec = %8lu\n", timeStamp.tv_usec);

	//Get nearby device's device class information	
	char classInfo[30];
	classInfo[0] = '\0';
	getClassInfo(btMsg->dev_class, classInfo);
	
	//btSignalRecords table:
    //	timeDate, timeStamp, timeUSec, machineID, rssi, deviceClass
    snprintf(queryStatement, 256, 
		"INSERT INTO %s VALUES ('%d', '%s', '%s', '%lu', '%s', '0', '%s', 'StudentLounge'  )",
		 table.c_str(), this->experimentIndex , timeStringInDateTimeFormat, timeStampStringContainsMicroSecond , 
		 timeStamp.tv_usec, btMachineID, classInfo);

    mysql_query(conn, queryStatement);


	/** Send BT Signal To ProD 2.0 server **/
	snprintf(httpputUrl, 256, "%s%s/", serverUpdateVisitors, btMachineID);

	curl = curl_easy_init();
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, httpputUrl);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	
		curl_easy_cleanup(curl);
	}
}


void BluetoothController::decodeBTMsg( inquiry_info_with_rssi * btMsg, struct timeval timeStamp)
{
	struct tm timeInDateTimeFormat;
    char timeStampStringContainsMicroSecond[80];
    char timeStringInDateTimeFormat[80];
    char btMachineID[18];
    char queryStatement[256];
	char httpputUrl[256];

    ba2str(&(btMsg->bdaddr), btMachineID);
	debug && cout << "find a nearby bluetooth devices: " << btMachineID << endl;
	
    timeInDateTimeFormat = * localtime( &(timeStamp.tv_sec) );
    //convert the time to a human redable format
    strftime(timeStringInDateTimeFormat, sizeof(timeStringInDateTimeFormat),
        "%Y-%m-%d %H:%M:%S", &(timeInDateTimeFormat));

    sprintf(timeStampStringContainsMicroSecond, "%lu.%03lu\n", timeStamp.tv_sec, timeStamp.tv_usec);
    debug && printf("timeStampStringContainsMicroSeccond = %s", timeStampStringContainsMicroSecond);
    debug && printf("@ %s : %06lu ... \n", timeStringInDateTimeFormat, timeStamp.tv_usec);
    debug && printf("timeStamp.tv_sec = %8lu\n", timeStamp.tv_sec);
	debug && printf("timeStamp.tv_usec = %8lu\n", timeStamp.tv_usec);

	char classInfo[30];
	classInfo[0] = '\0';
	getClassInfo(btMsg->dev_class, classInfo);

	//btSignalRecords table:
    //	timeDate, timeStamp, timeUSec, machineID, rssi, deviceClass
    snprintf(queryStatement, 256, 
		"INSERT INTO %s VALUES ('%d', '%s', '%s', '%lu', '%s', '%d', '%s', 'StudentLounge'  )", 
		table.c_str(), this->experimentIndex,  timeStringInDateTimeFormat, timeStampStringContainsMicroSecond , 
		timeStamp.tv_usec, btMachineID, btMsg->rssi, classInfo);
			//c_str() convert the C++ string to char *

    mysql_query(conn, queryStatement);
	
	/** Send BT Signal To ProD 2.0 server **/
	snprintf(httpputUrl, 256, "%s%s/", serverUpdateVisitors, btMachineID);

	curl = curl_easy_init();
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, httpputUrl);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	
		curl_easy_cleanup(curl);
	}
	

}

void BluetoothController::getClassInfo(uint8_t dev_class[3], char * classInfo)
{
	//int flags = dev_class[2];
    int major = dev_class[1];
    int minor = dev_class[0] >> 2;
    char buffer[30];



    if (major > ENT(majors) ){
        if (major == 63)
           sprintf(buffer, " Unclassified device");
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
	default:
    	sprintf(buffer, " Unclassified device");
    	strcat( classInfo, buffer);
    	return;
	}
    sprintf(buffer, " %s", majors[major]);
    strcat( classInfo, buffer);
}
