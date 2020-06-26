#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include "socket.h"
#include "message.h"

#define ASCENT 0.01  //Is used to increase the temperature as a percentage.
#define DESCENT 0.02 //Is used to decrease the temperature as a percentage.
#define START_VALUE 22  // Starting point for temperature

/* Simulates changes to the temperature sensor device using the 
 * FAN_STATUS message.
 */
void read_temperature(struct cignal *cig) {
	if (cig->hdr.type == FAN_STATUS) {
		cig->hdr.type = UPDATE;
		if (cig->cooler == ON) {
			cig->value -= DESCENT * cig->value;
		} else if (cig->cooler == OFF) {
			cig->value += ASCENT * cig->value;
		}
	} else {
		cig->hdr.type = UPDATE;
	}
}

int main(int argc, char **argv) {
	int port;
	char *hostname = "localhost";
	time_t rawtime;
	struct tm *now;
	struct cignal cig;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s port [hostname]\n", argv[0]);
		exit(1);
	}

	if (argc == 3) {
		hostname = "localhost";
	}

	port = strtol(argv[1], NULL, 0);
	printf("The Cignal Temperature Sensor is now reading...\n\n");

	// Set the initial state of this temperature sensor
	cig.hdr.device_type = TEMPERATURE; 
	cig.value = START_VALUE; //initial temperature value
	cig.cooler = OFF;
	cig.dehumid = OFF;

	char *cig_serialized;
	int msgno = 1;

	while (1) {
		int peerfd;	
		if ((peerfd = connect_to_server(port, hostname)) == -1)     {
			fprintf(stderr, "Error connecting to the gateway!\n");
			exit(1);
		}
		/*
		 * If this is the first message, then send a handshake message with
		 * a device id of -1.  If it is a subsequent message, then write
		 * the current state of the message, and read potentially new state
		 * from the server.
		 */

        if (msgno == 1){
            msgno = 0;
            cig.hdr.device_id = -1;
            cig.hdr.type = 1;
        }
        cig_serialized = serialize_cignal(cig);

        //Send to the server
        int numw = write(peerfd, cig_serialized, CIGLEN);
        if (numw <= 0){
            perror("write");
            exit(1);
        }

        //Read from the server
        int numr = read(peerfd, cig_serialized, CIGLEN);
        if (numr <= 0) {
            perror("read");
            exit(1);
        }

        close(peerfd);
        unpack_cignal(cig_serialized, &cig);

        if (cig.hdr.type != 1 && sleep(INTERVAL) >= 0) {
			rawtime = time(NULL);
			now = localtime(&rawtime);
			read_temperature(&cig);
			printf("[%02d:%02d:%02d] Temperature: %.4f\n", 
					now->tm_hour, 
					now->tm_min, 
					now->tm_sec, 
					cig.value);
		}
	}
}
