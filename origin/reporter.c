/*
 * Bluetooth inquisition - Copyright Michael John Wensley 2005 2006
 */

#include <stdlib.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "inq.h"

static inquiry_info *store1 = NULL;
static int entries1 = 0;
static int capacity1 = 0;

static inquiry_info *store2 = NULL;
static int entries2 = 0;
static int capacity2 = 0;

extern int debug;
extern int num_dongle;

/* information: https://www.bluetooth.org/foundry/assignnumb/document/assigned_numbers
 * some device descriptions from https://www.bluetooth.org/foundry/assignnumb/document/baseband
 */
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
/* end of device descriptions */

/**
 * Decode device class
 */
static void classinfo(uint8_t dev_class[3]) {
	int flags = dev_class[2];
	int major = dev_class[1];
	int minor = dev_class[0] >> 2;

	printf("[ ");
	if (flags & 0x1) printf("Position ");
	if (flags & 0x2) printf("Net ");
	if (flags & 0x4) printf("Render ");
	if (flags & 0x8) printf("Capture ");
	if (flags & 0x10) printf("Obex ");
	if (flags & 0x20) printf("Audio ");
	if (flags & 0x40) printf("Phone ");
	if (flags & 0x80) printf("Info ");
	printf("]");

	if (major > ENT(majors)) {
		if (major == 63)
			printf(" Unclassified device\n");
		return;
	}
	switch (major) {
	case 1:
		if (minor <= ENT(computers)) printf(" %s", computers[minor]);
		break;
	case 2:
		if (minor <= ENT(phones)) printf(" %s", phones[minor]);
		break;
	case 3:
		printf(" Usage %d/56", minor);
		break;
	case 4:
		if (minor <= ENT(av)) printf(" %s", av[minor]);
		break;
	case 5:
		if ((minor & 0xF) <= ENT(peripheral)) printf(" %s", peripheral[(minor & 0xF)]);
		if (minor & 0x10) printf(" with keyboard");
		if (minor & 0x20) printf(" with pointing device");
		break;
	case 6:
		if (minor & 0x2) printf(" with display");
		if (minor & 0x4) printf(" with camera");
		if (minor & 0x8) printf(" with scanner");
		if (minor & 0x10) printf(" with printer");
		break;
	case 7:
		if (minor <= ENT(wearable)) printf(" %s", wearable[minor]);
		break;
	case 8:
		if (minor <= ENT(toys)) printf(" %s", toys[minor]);
		break;
	}
	printf(" %s", majors[major]);
}


/*
 * Add entry to the new inq. If it is not also
 * in the old inq, report adds.
 */
void reporter_add(inquiry_info* newthingy) {
	char ieeeaddr[18];
	int i;
	inquiry_info *rstore;

	/* find out if new object is already be found in this inquiry window */
	for (i = 0; i < entries1; i++) { //entries1 - 1 is because we already put newthingy into store1, if we use entries1 
					   // then bacmp will always find a bdaddr with the same bdaddr
		if (0 == bacmp(&((store1+i)->bdaddr),&(newthingy->bdaddr))) {
			if (debug) {
			        ba2str(&(newthingy->bdaddr), ieeeaddr);
			        printf("= %s %02x %02x ", ieeeaddr,
					newthingy->pscan_rep_mode,
					newthingy->pscan_mode);
			        classinfo(newthingy->dev_class);
				printf("\n");
			}
			return;
		}
	}

	/*only when we find a newthingy that is not in store1, we insert it into store1 */
		/* make storage bigger for bluetooth object if needed */	
	entries1++;
	if ((entries1 * INQUIRY_INFO_SIZE) > capacity1) {
		capacity1 = entries1 * INQUIRY_INFO_SIZE;
		rstore = realloc(store1, capacity1);
		if (rstore == NULL) {
			perror("I cannot allocate any more memory for new device, but continuing anyways... :-/");
			return;
		}
		store1 = rstore;
	}
	/* store in the array */
	*(store1+(entries1-1)) = *newthingy;

	/* find out if new object is in the old array... C cannot compare structures on its own... */
	for (i = 0; i < entries2; i++) {
		if (0 == bacmp(&((store2+i)->bdaddr),&(newthingy->bdaddr))) {
			if (debug) {
			        ba2str(&(newthingy->bdaddr), ieeeaddr);
			        printf("= %s %02x %02x ", ieeeaddr,
					newthingy->pscan_rep_mode,
					newthingy->pscan_mode);
			        classinfo(newthingy->dev_class);
				printf("\n");
			}
			return;
		}
	}

	/* it isn't ... report it to chatbot...
	 * we report the clock offset as it is used to quickly home in on the radio frequency (in the channel hopping sequence)
	 * that the target is operating on, when we want to send a command to a device.
	 *
	 * just for fun when you are running this program in debug mode, you may see the clock offset drift up and down
	 * slightly, maybe due to doppler effect, interference, etc.
	 *
	 *      Time ---> (clock offset)
	 *   +---+---+---+---+---+---+---+---+
	 * R |\  |   |/---\  |   |   /---\   |
	 * F | \ |   /   | \---\ |  /|   |\--|
	 *   |  \---/|   |   |  \--/ |   |   |
	 *   +---+---+---+---+---+---+---+---+
	 *
	 */
        ba2str(&(newthingy->bdaddr), ieeeaddr);
        printf(" %s %02x %02x ", ieeeaddr,
		newthingy->pscan_rep_mode,
		newthingy->pscan_mode);
        classinfo(newthingy->dev_class);
	printf("\n");
}

/* fix to support with rssi */
void reporter_add_with_rssi(inquiry_info_with_rssi* newthingy) {
	inquiry_info template;
	template.bdaddr = newthingy->bdaddr;
	template.pscan_rep_mode = newthingy->pscan_rep_mode; 
	template.pscan_period_mode = newthingy->pscan_period_mode;
	template.pscan_mode = 0;
	template.dev_class[0] = newthingy->dev_class[0];
	template.dev_class[1] = newthingy->dev_class[1];
	template.dev_class[2] = newthingy->dev_class[2];
	template.clock_offset = newthingy->clock_offset;
	debug && printf("rssi %d ... ", newthingy->rssi);
	reporter_add(&template);
}


/* The new inq has concluded so, see if there are any entries in
 * the old inq that are not in the new inq.... report removes.
 *
 * New inq becomes the old inq. Old inq is cleared and re'used for the next inq.
 */
void reporter_swap() {
	char ieeeaddr[18];
	int i, j;
	inquiry_info *rstore;
	int rcapacity;
	
	for (i = 0; i < entries2; i++) {
		for (j = 0; j < entries1; j++) {
			if (0 == bacmp( &((store2+i)->bdaddr), &((store1+j)->bdaddr))) break;
		}
		if (j == entries1) {
			ba2str(&((store2+i)->bdaddr), ieeeaddr);
			printf("- %s %02x %02x", ieeeaddr,
				(store2+i)->pscan_rep_mode,
				(store2+i)->pscan_mode);
		        classinfo((store2+i)->dev_class);
		        printf("\n");
		}
	}

	/* new becomes old, old is re-used for the next scan */
	rstore    = store2;
	rcapacity = capacity2;
	
	store2    = store1;
	capacity2 = capacity1;
	entries2  = entries1;

	store1    = rstore;
	capacity1 = rcapacity;
	entries1  = 0;
}
