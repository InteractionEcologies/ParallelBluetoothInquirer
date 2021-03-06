/*-
 * Bluetooth inquisition - Copyright Michael John Wensley 2005 2006
 *
 * Changelog: 20060206 added request name support
 *
 * Featuring new aggressive, vicious scanning.
 *
 * https://www.bluetooth.org/spec/
 * BT Spec: http://www.bluetooth.org/foundry/adopters/document/Core_v2.0_EDR/en/1/Core_v2.0_EDR.zip
 * http://standards.ieee.org/getieee802/802.15.html
 *
 * Older spec: https://www.bluetooth.org/foundry/adopters/document/Bluetooth_Core_Specification_v1.2
 *
 * Unfortunatly we can't get closer to bluetooth than the HCI layer... so...
 *
 * do inquiry -notice that all multi-octet paramaters are in reverse octet order.
 * hcitool cmd 01 0001 33 8b 9e 30 00
 *                     ^  ^  ^  ^  ^---- maximum responses (0x00 = no limit) 0x01-0xFF means 1-255 maximum responses
 *                     |  |  |  +------- timeout in 1.28 second units (bluetooth hops) 0x01-0x30
 *                     +--+--+---------- LAP as 338b9e "GIAC" meaning I want all devices that can, to respond.
 * and 338b9e is mentioned here too: https://www.bluetooth.org/foundry/assignnumb/document/baseband
 * and subsequently moved to https://www.bluetooth.org/assigned-numbers/
 *
 * This tells the bluetooth dongle to do continuous inquiries until stopped with 0004 below
 *
 * perfect for the inquisition version 2 `_^ (see section 7.1.3 of the BT spec)
 * hcitool cmd 01 0003 0A 00 09 00 33 8b 9e 08 00
 *                     ^  ^  ^  ^  ^  ^  ^  ^  ^---- maximum responses like as above.
 *                     |  |  |  |  |  |  |  +------- length time in units to follow the inquiry channel hop sequence.
 *                     |  |  |  |  +--+--+---------- LAP (the magic number is "GIAC") meaning I want all devices.
 *                     |  |  +--+------------------- min_period between scans (must be bigger than length)
 *                     +--+------------------------- max_period between scans (must be bigger than min_period)
 * stop the inquisition... used on exit or to query for other things.
 * hcitool cmd 01 0004
 *
 * hcidump -X
 * xx [ mm mm mm mm mm mm 01 00 00 dd dd dd cc cc ]
 * ^^   ^^^^^^^^^^^^^^^^^ ^^ ^^ ^^ ^^^^^^^^ ^^^^^
 *  |                   |  |  |  |        |     +-- clock offset
 *  |                   |  |  |  |        +-------- device class
 *  |                   |  |  |  +----------------- Page Scan Mode
 *  |                   |  |  +-------------------- Page Scan Period Mode
 *  |                   |  +----------------------- Page Scan Repetition Mode
 *  |                   +-------------------------- IEEE MAC Address
 *  +---------------------------------------------- Number of responses
 *
 * return RSSI (signal stength) with inqury results would be nice too...
 * hcitool cmd 03 0045 01
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <libintl.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "mongo.h"
#include "string.h"

#include "inq.h"

#define maxi(a,b) ((a)>(b)?(a):(b))
#define mini(a,b) ((a)<(b)?(a):(b))

#define MAX_DONGLE 10

extern char **environ;
extern char *optarg;

bson b;
mongo conn;

static int p_inquiry = 0;
static int run = 0;
int debug = 0;

int completectrl = 0;

int num_dongle = 0;

static unsigned char buf[HCI_MAX_FRAME_SIZE];
static unsigned char controlbuf[100];
struct timeval inquiry_complete;

static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"device", 1, 0, 'i'},
	{"debug", 0, &debug, 1},
	{"length", 1, 0, 'l'},
	{"pause", 1, 0, 'p'},
	{0, 0, 0, 0}
};



/**
 * This decodes output from the dongle. Generally BlueZ would carbon-copy all the dongle's output
 * to all the programs that have a raw socket to the dongle open, thusly ensuring no-one
 * has to miss anything interesting :-)
 *
 * However, this means we must detect precicely the things we are interested in, as the
 * user might have other programs causing HCI events as well as our periodic inquiry,
 * such as open connections, sdp, etc. This also means you can usually leave inq running
 * when interrogating a discovered device with hcitool.
 *
 */
static int decode(int len, struct timeval tstamp) {
	char ieeeaddr[18];
	struct timeval resultime;
	char bufferForTime[20];
	time_t currentTime;
	struct timeval current_tv;
	struct timezone current_tz;
	struct tm *tm;
	gettimeofday(&current_tv, &current_tz);
	tm = localtime(&current_tv.tv_sec);

	int i;
	if (len < 1) {
		perror("Packet too small");
		return EXIT_SUCCESS;
	}
	switch (buf[0]) {
		case HCI_EVENT_PKT:
	        if (len < 3) {
        	        fprintf(stderr, gettext("Event so short I cannot read the OGF/OCF numbers\n"));
        	        return EXIT_FAILURE;
	        }
	        if (len != (buf[2] + 3)) {
	                fprintf(stderr, gettext("Event parameter size does not match buffer read.\n"));
	                return EXIT_FAILURE;
       		}
		/* now we don't need to refer to len again... */
		switch (buf[1]) {
			case EVT_INQUIRY_COMPLETE:
				/* this means the inquiry is complete, purge the database for the next */
				/* Only when every dongle all finish the inquiry, the database could swap */
				completectrl++;
				if(completectrl == num_dongle){
					completectrl = 0;
					inquiry_complete = tstamp;
					reporter_swap();
						if (buf[2] != 1) {
						fprintf(stderr, gettext("Inquiry Complete event length wrong.\n"));				
					}
					errno = bt_error(buf[3]);
					if (errno > 0) {
						perror("Inquiry Complete");
						return EXIT_FAILURE;
					}
					debug && printf("*Inquiry Complete*\n");
					fflush(stdout);
				}
				//debug && printf("\n");
				break;
			case EVT_INQUIRY_RESULT:
			/* we can infer the distance/interference from the time taken for a response */
			/* got a result but without any devices in it (!) */
			if (buf[2] == 0){
				printf("\n");
				break;
			}
			if (buf[2] != ((buf[3] * sizeof(inquiry_info)) + 1)) {
				fprintf(stderr, gettext("Inquiry Result event length wrong.\n"));
				return EXIT_FAILURE;
			}

			/* finding the time since the beginning of this inq
			 * gives us an unreliable hint about how far away it is
			 * RSSI is better for that, of course.
			 */
			timersub(&tstamp,&inquiry_complete,&resultime);

			for (i = 0; i < buf[3]; i++) {
				debug && printf("@ %8lu:%06lu ... ", resultime.tv_sec, resultime.tv_usec);
				reporter_add((inquiry_info*)&buf[4 + i * sizeof(inquiry_info)]);
			}
			/* flush so netcat6 or whatever else that is on standard out, gets the events */
			fflush(stdout);
			break;
			case EVT_REMOTE_NAME_REQ_COMPLETE:
			if (buf[2] != sizeof(evt_remote_name_req_complete)) {
				fprintf(stderr, gettext("Name Request Complete event length wrong.\n"));
				return EXIT_FAILURE;
			}
			errno = bt_error(((evt_remote_name_req_complete*)&buf[3])->status);
			debug && fprintf(stderr, gettext("Name complete Status %s\n"), strerror(errno));
			ba2str( &(((evt_remote_name_req_complete*)&buf[3])->bdaddr), ieeeaddr);
			if (((evt_remote_name_req_complete*)&buf[3])->status == 0) {
				printf("!name %s ", ieeeaddr);
				for (i = 0; i < (buf[2] - 7); i++) {
					if ( (((evt_remote_name_req_complete*)&buf[3])->name[i]) == 0)
						break;
					if ( (((evt_remote_name_req_complete*)&buf[3])->name[i]) < 32) {
						printf("\\x%02x", (((evt_remote_name_req_complete*)&buf[3])->name[i]));
					} else {
						putchar((((evt_remote_name_req_complete*)&buf[3])->name[i]));
					}
				}
			} else {
				printf("!namefail %s %d", ieeeaddr, ((evt_remote_name_req_complete*)&buf[3])->status);
			}
			printf("\n");
			break;
			case EVT_CMD_COMPLETE:
			/* it can be bigger, depending on the opcode */
			if (buf[2] < sizeof(evt_cmd_complete)) {
                                fprintf(stderr, gettext("Command Complete event length wrong."));
                                return EXIT_FAILURE;
                        }

			/* ogf is upper 6 bits, ocf lower 10 bits to make opcode */
			switch (((evt_cmd_complete*)&buf[3])->opcode) {
				case cmd_opcode_pack(OGF_LINK_CTL,OCF_PERIODIC_INQUIRY):
				errno = bt_error(buf[3 + sizeof(evt_cmd_complete)]);
				debug && fprintf(stderr, gettext("Periodic inquiry requested: %s\n"), strerror(errno));
				if (buf[2] != (sizeof(evt_cmd_complete) + 1)) {
					fprintf(stderr, gettext("Periodic Inquiry Command Complete event length wrong.\n"));
					return EXIT_FAILURE;
				}
				if (buf[3 + sizeof(evt_cmd_complete)] == HCI_COMMAND_DISALLOWED) {
					fprintf(stderr, gettext("\
The Dongle is in a state that refused setting periodic mode.\n\
It may, of course, already be in periodic mode (You -SIGKILL'd me earlier?)\n\
so I will just pretend that it is.\n\
"));
					p_inquiry = 1;
				} else {
					if (errno > 0) {
						perror(gettext("Can't Start Periodic mode."));
						return EXIT_FAILURE;
					}
					if (inquiry_complete.tv_sec == 0) inquiry_complete = tstamp;
					p_inquiry = 1;
				}
				break;
				case cmd_opcode_pack(OGF_LINK_CTL,OCF_EXIT_PERIODIC_INQUIRY):
				errno = bt_error(buf[3 + sizeof(evt_cmd_complete)]);
				debug && fprintf(stderr, gettext("Exit Periodic inquiry requested: %s\n"), strerror(errno));
                                if (buf[2] != (sizeof(evt_cmd_complete) + 1)) {
                                        fprintf(stderr, gettext("Exit Periodic Inquiry Command Complete event length wrong.\n"));
					return EXIT_FAILURE;
                                }
                                if (buf[3 + sizeof(evt_cmd_complete)] == HCI_COMMAND_DISALLOWED) {
					fprintf(stderr, gettext("\
The Dongle is in a state that refused clearing periodic mode.\n\
It may, of course, have been unset by another program.\n\
so I will just pretend that it is.\n\
"));
					p_inquiry = 0;
                                } else {
                                        errno = bt_error(buf[3 + sizeof(evt_cmd_complete)]);
                                        if (errno > 0) {
                                                perror(gettext("Can't Stop Periodic mode."));
                                                return EXIT_FAILURE;
                                        }
					p_inquiry = 0;
                                }
				break;
			}
				
			break;
			case EVT_CMD_STATUS:
			if (buf[2] != sizeof(evt_cmd_status)) {
                                perror(gettext("Command Status event length wrong.\n"));
                                return EXIT_FAILURE;
                        }
			errno = bt_error(((evt_cmd_status*)&buf[3])->status);
			switch (((evt_cmd_status*)&buf[3])->opcode) {
				case cmd_opcode_pack(OGF_LINK_CTL,OCF_CREATE_CONN):
				if (debug) {
					fprintf(stderr, gettext("Device Connect requested: %s\n"), strerror(errno));
				}
				break;
				case cmd_opcode_pack(OGF_LINK_CTL,OCF_DISCONNECT):
				if (debug) {
					fprintf(stderr, gettext("Device Disconnect Requested: %s\n"), strerror(errno));
				}
				break;
				case cmd_opcode_pack(OGF_LINK_CTL,OCF_REMOTE_NAME_REQ):
				printf("!namereq %d\n", ((evt_cmd_status*)&buf[3])->status);
				break;
				default:
				if (debug) {
					fprintf(stderr, gettext("HCI opcode %d requested: %s\n"), (((evt_cmd_status*)&buf[3])->opcode), strerror(errno));
				}
			}				
		
			break;
			/* my bluetooth dongle does not implement this (??) so it is untested
			 *
			 */
			case EVT_INQUIRY_RESULT_WITH_RSSI:
			if (buf[2] == 0)
				break;

			if (buf[2] != ((buf[3] * sizeof(inquiry_info_with_rssi)) + 1)) {
				perror(gettext("Inquiry Result with RSSI event length wrong."));
				return EXIT_FAILURE;
			}

			timersub(&tstamp,&inquiry_complete,&resultime);

			for (i = 0; i < buf[3]; i++) {
				debug && printf("@ %8lu:%06lu ... ", resultime.tv_sec, resultime.tv_usec);
				bson_init( &b );
				bson_append_new_oid( &b, "_id");
				bson_append_new_oid( &b, "user_id" );

				bson_append_int( &b, "usec", current_tv.tv_usec);
				
				time(&currentTime);
			
				bson_append_int( &b, "time", currentTime); 
				bson_append_string( &b, "unixTime", ctime(&currentTime)); 
				
				sprintf(bufferForTime, "%8lu", resultime.tv_sec);
				bson_append_string( &b, "inq_time_sec", bufferForTime);
				sprintf(bufferForTime, "%06u", resultime.tv_usec);
				bson_append_string( &b, "inq_time_usec", bufferForTime);
				
				reporter_add_with_rssi((inquiry_info_with_rssi*)&buf[4 + i * sizeof(inquiry_info_with_rssi)]);
			}
			fflush(stdout);
			break;
		}
		break;
	}
	return EXIT_SUCCESS;
}







static void exquisition(int sig) {
}



/**
 * The Inquisition... sets up dongle to scan
 * continuously, and continuously reads its output
 */
static int inquisition(int* dd, int length) {
        fd_set readSet;
        fd_set writeSet;
        fd_set exceptSet;
	char inbuf[100];
	char ieeeaddr[18];
	
	int i;
	int counter;
	unsigned int j, k, l;
	uint8_t dir;
	int len;
	periodic_inquiry_cp infoarea;
	periodic_inquiry_cp *info = &infoarea;
	remote_name_req_cp getnamearea;
	remote_name_req_cp *getname = &getnamearea;
	
	struct hci_filter flt;
	/* hci_event_hdr *hdr; */
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct timeval tstamp;
	
	struct sigaction sa;



	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = exquisition;

	/* sent control-c */
	sigaction(SIGINT, &sa, NULL);
	/* killall inq */
	sigaction(SIGTERM, &sa, NULL);
	/* bash -c 'inq | something &'
	 * and the something e.g. netcat6 decides to quit...
	 */
	/* sigaction(SIGPIPE, &sa, NULL); */
	
	run = 1;

	/* connect to the dongle length between 1 and 30
	 *
	 */
	info->num_rsp = 0x00;
	info->lap[0] = 0x33;
	info->lap[1] = 0x8b;
	info->lap[2] = 0x9e;

	info->length = length;
	info->min_period = info->length + 1;
	info->max_period = info->min_period + 1;

	/* Set HCI_DATA_DIR */
	for(counter=0; counter<num_dongle; counter++){
		i = 1;
		if (setsockopt(dd[counter], SOL_HCI, HCI_DATA_DIR, &i, sizeof(i)) < 0) {
			perror(gettext("Can't request data direction\n"));
			return EXIT_FAILURE;
		}
	}

	/* Set HCI_TIME_STAMP */
	for(counter=0; counter<num_dongle; counter++){
		i = 1;
		if (setsockopt(dd[counter], SOL_HCI, HCI_TIME_STAMP, &i, sizeof(i)) < 0) {
			perror(gettext("Can't request data timestamps\n"));
			return EXIT_FAILURE;
		}
	}

	/* filtering tells BlueZ which events we want on our socket... the dongle's
	 * own event mask isnt necessarily just this, it includes what other programs want too..?
	 * I.e. we want command complete, and all inquisition messages.
	 */
	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	/* hci_filter_all_ptypes(&flt); */
	hci_filter_all_events(&flt);
	
	/* Set HCI_FILEER */
	for(counter=0; counter<num_dongle; counter++){
		if (setsockopt(dd[counter], SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
			perror(gettext("Cannot set filter"));
			return EXIT_FAILURE;
		}
	}

	/* Begin Periodic Inquiry */
	for(counter=0; counter<num_dongle; counter++){
		if (hci_send_cmd(dd[counter], OGF_LINK_CTL, OCF_PERIODIC_INQUIRY, PERIODIC_INQUIRY_CP_SIZE, info) < 0) {
			perror(gettext("Cannot request periodic inquiry!\n"));
			return EXIT_FAILURE;
		}
	}


	memset(&msg, 0, sizeof(msg));

	iov.iov_base = &buf;
	iov.iov_len = sizeof(buf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &controlbuf;
	msg.msg_controllen = sizeof(controlbuf);
	
	inquiry_complete.tv_sec = 0;
	inquiry_complete.tv_usec = 0;
	
	/* select <-> recvmsg (like read) */
	while (run || p_inquiry) {
	        FD_ZERO(&readSet);
	        FD_ZERO(&writeSet);
	        FD_ZERO(&exceptSet);

		for(counter=0; counter< num_dongle; counter++){
			FD_SET(dd[counter], &readSet);
		}
		//debug && fprintf(stderr, "FD_ISSET:dd = %d, dd1 = %d\n", FD_ISSET(dd, &readSet), FD_ISSET(dd1, &readSet));

		FD_SET(fileno(stdin), &readSet);
		select(maxi(dd[num_dongle-1], fileno(stdin))+1, &readSet, &writeSet, &exceptSet, NULL);

		for(counter=0; counter<num_dongle; counter++){
			if (FD_ISSET(dd[counter], &readSet)) {
				len = recvmsg(dd[counter], &msg, 0);
				
				
				if (len < 0) {
					/* this is the preferred method of exiting this program ... SIGINT */
					if (errno == EINTR) {
						if (run == 0) {
							perror("Second SIGINT received, giving up waiting for exit periodic!");
							return EXIT_FAILURE;
						}
						run = 0;
						if (p_inquiry) {
						        if (hci_send_cmd(dd[counter], OGF_LINK_CTL, OCF_EXIT_PERIODIC_INQUIRY, 0, NULL) < 0) {
						                fprintf(stderr, gettext("I cannot stop the inquisition!\n"));
					        	        return EXIT_FAILURE;
							}
					        }
						perror(gettext("Exiting"));
					} else {
						perror(gettext("Could not receive\n"));
						return EXIT_FAILURE;
					}
				} else {	
	
					if( len >= 0){
						cmsg = CMSG_FIRSTHDR(&msg);
						while (cmsg) {
							switch (cmsg->cmsg_type) {
							case HCI_CMSG_DIR:
								dir = *((uint8_t *) CMSG_DATA(cmsg));
								break;
							case HCI_CMSG_TSTAMP:
								tstamp = *((struct timeval *) CMSG_DATA(cmsg));
								break;
							}
							cmsg = CMSG_NXTHDR(&msg, cmsg);
						}
				
						/* decode packets */
						/* printf("dump %02X %02X %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]); */
						if ((i=decode(len, tstamp)) != EXIT_SUCCESS) {
							return i;
						}
					}
				}
			}
		}	
		if (FD_ISSET(fileno(stdin), &readSet)) {
			len = read(0, inbuf, 50);
			if (len < 1) {
				/* this is the preferred method of exiting this program ... SIGINT */
				if (errno == EINTR) {
					if (run == 0) {
						perror("Second SIGINT received, giving up waiting for exit periodic!");
						return EXIT_FAILURE;
					}
					run = 0;
					if (p_inquiry) {
					        if (hci_send_cmd(dd[0], OGF_LINK_CTL, OCF_EXIT_PERIODIC_INQUIRY, 0, NULL) < 0) {
					                fprintf(stderr, gettext("I cannot stop the inquisition!\n"));
				        	        return EXIT_FAILURE;
						}
				        }
					perror(gettext("Exiting"));
				} else {
					perror(gettext("Could not receive\n"));
					return EXIT_FAILURE;
				}
			}
			inbuf[len] = 0;
			j = 0; k = 0; l = 0;
			i = sscanf(inbuf, "getname %17s %02x %02x %04x", ieeeaddr, &j, &k, &l);

			getname->pscan_rep_mode = j;
			getname->pscan_mode = k;
			getname->clock_offset = l;

			ieeeaddr[17] = 0;
			if (i > 0) {
				str2ba(ieeeaddr, &getname->bdaddr);
				ba2str(&getname->bdaddr, ieeeaddr);
				getname->clock_offset = 0x8000 | getname->clock_offset;
				if (i < 4) { 
					getname->clock_offset = 0;
				} 
					
				debug && printf("getname request received %d [ %s %02x %02x %04x ]\n", i, ieeeaddr,
					getname->pscan_rep_mode,
					getname->pscan_mode,
					getname->clock_offset);
				
				if (hci_send_cmd(dd[0], OGF_LINK_CTL, OCF_REMOTE_NAME_REQ, REMOTE_NAME_REQ_CP_SIZE, getname) < 0) {
					perror(gettext("Cannot request remote name!\n"));
					return EXIT_FAILURE;
				}
			}
		}
	}

	return EXIT_SUCCESS;
}




/* have the operating system unlock access to
 * exclusivly superuser net fun and games (raw sockets)

 * void unlock_netadmin() {
 *	char startup[100];
 *	sprintf(startup, "sudo setpcaps cap_net_raw+eip %d > /dev/null 2>&1", getpid());
 *	system(startup);
 * }
 */

int main(argc, argv, envp)
int argc;
char *argv[];
char *envp[];
{
	int opt,iq,length = 0x08, fd = -1;
	char *str = "OK\r\n";
	int dongle_n[MAX_DONGLE];
	int i;

	//Create a mongo database connection
	if( mongo_connect( &conn, "127.0.0.1", 27017) != MONGO_OK) {
		switch( conn.err){
		case MONGO_CONN_NO_SOCKET:
			printf("FAIL: Could not create a socket!\n");
			break;
		case MONGO_CONN_FAIL:
			printf("FAIL: Could not connect to mongod. Make sure it's listening at 127.0.0.1:27017.\n");
			break;
		}
		
		exit(1);
	}



	//initialize all the dongles/adaptors
	for(i=0; i< MAX_DONGLE; i++){
		dongle_n[i] = -1;
	}
	
	while ((opt=getopt_long(argc, argv, "+n:hdl:p", long_options, NULL)) != -1) {
		switch(opt) {
		case 'n':
			num_dongle = atoi(optarg);
			//dongle = hci_devid(optarg);
			
			for(i=0; i < num_dongle; i++){
				dongle_n[i] = hci_open_dev(i); 
				if (dongle_n[i] < 0) {
					fprintf(stderr, "Dongle hci%d does not exist!\n", i);
					return EXIT_FAILURE;
				}
			
			}
			break ;
		case 'p':
			fd = open (optarg, O_WRONLY);
			if (fd < 0) {
				fprintf(stderr, "Cannot open pipe to write!\n");
				return EXIT_FAILURE;
			}
		      write (fd, str, strlen (str));
		      close (fd);
			fd = open (optarg, O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "Cannot open pipe to read!\n");
				return EXIT_FAILURE;
			}
		      read (fd, str, strlen (str));
		      close (fd);

			break;;
		case 'h':
			printf(gettext("\
inq: Copyright 2005 Michael John Wensley\n\
This program puts a hci device into continuous scanning mode\n\
\n\
	-h, --help\n\
		for this text\n\
	-i, --device=\"hci0\"\n\
		specify a dongle by name.\n\
	-d, --debug\n\
		for extra output\n\
	-l, --length=8\n\
		length of inquiry in 1.28 second units between 1 and 48\n\
	-p, --pause=\"pipe\"\n\
		pause startup until read something from given FIFO\n\
		usually root-only facilities with\n\
		e.g. sudo setpcaps cap_net_raw+eip `pidof inq`\n\
"));
			return EXIT_SUCCESS;
		case 'd':
			fprintf(stderr, gettext("Debug on\n"));
			debug = 1;
			break;;
		case 'l':
			length = atoi(optarg);
			fprintf(stderr, "length = %d\n", length); 
			if ((length < 0x01) || (length > 0x30)) {
				fprintf(stderr,"Inquiry length has to be between 1 and 48\n");
				return EXIT_FAILURE;
			}
			break;;
		}
	}
	
	//If user doesn't specify the number of dongles, only use 1 dongle (dongle_n[0])
	if ( num_dongle == 0) {
		num_dongle = 1;
		dongle_n[0] = hci_open_dev(0);
		debug && fprintf(stderr, "dongle_n[0] = %d\n", dongle_n[0]);
	}
	if (dongle_n[0] < 0) {
		fprintf(stderr, gettext("You don't seem to have a bluetooth dongle! Or I cannot connect to your dongle!\n"));
		return EXIT_FAILURE;
	}
	debug && fprintf(stderr, gettext("Welcome to the inquisition on device %d\n"), dongle_n[0]);

	iq=inquisition(dongle_n, length);

	for(i=0; i<num_dongle; i++){
		hci_close_dev(dongle_n[i]);
	}

	mongo_destroy( &conn );
	return iq;
}
