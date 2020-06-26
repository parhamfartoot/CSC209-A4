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
#include "controller.h"

#define MAXFD(x,y) ((x) >= (y)) ? (x) : (y)
int readh(int fd, int* device_record);

int main(int argc, char *argv[]){
	int port;
	//struct cignal cig;

	// A buffer to store a serialized message

	//char *cig_serialized = malloc(sizeof(char)*CIGLEN);

	// An array to registered sensor devices
	int device_record[MAXDEV] = {0};
	
	if(argc == 2){
		port = strtol(argv[1], NULL, 0);
	} else{
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(1);
	}

	int gatewayfd = set_up_server_socket(port);
	printf("\nThe Cignal Gateway is now started on port: %d\n\n", port);
	int peerfd;
	
	/*
	 *
	 * Use select so that the server process never blocks on any call except
	 * select. If no sensors connect and/or send messsages in a timespan of
	 * 5 seconds then select will return and print the message "Waiting for
	 * Sensors update..." and go back to waiting for an event.
	 * 
	 * The server will handle connections from devices, will read a message from
	 * a sensor, process the message (using process_message), write back
	 * a response message to the sensor client, and close the connection.
	 * After reading a message, your program must print the "RAW MESSAGE"
	 * message below, which shows the serialized message received from the *
	 * client.
	 * 
	 *  Print statements you must use:
     * 	printf("Waiting for Sensors update...\n");
	 * 	printf("RAW MESSAGE: %s\n", YOUR_VARIABLE);
	 */

    fd_set all;
    FD_ZERO(&all);
    FD_SET(gatewayfd, &all);
    int max = gatewayfd;
    struct timeval tv;
    tv.tv_usec = 0;

    while(1) {
        tv.tv_sec = 5;

        fd_set listen = all;
        int nready = select(max + 1, &listen, NULL, NULL, &tv);

        if (nready == 0){
            printf("Waiting for Sensors update...\n");
            continue;
        }
        if (nready == -1){
            perror("select");
            exit(1);
        }
        if (FD_ISSET(gatewayfd, &listen)){
            peerfd = accept_connection(gatewayfd);
            if (peerfd > max){
                max = peerfd;
            }
            FD_SET(peerfd, &all);
        }
        for (int i = 0; i <= max; i++){
            if ((i != gatewayfd) && FD_ISSET(i, &listen) && readh(i, device_record)!= 0){
                close(i);
                FD_CLR(readh(i, device_record), &all);
            }
        }
    }
}

//======================Helper======================
int readh(int fd, int* device_record){
    //Return fd if it has been closed

    char serialized_cignal[CIGLEN];
    struct cignal cignal;

    if (read(fd, serialized_cignal, CIGLEN) < CIGLEN)
        return fd;

    printf("RAW MESSAGE: %s\n", serialized_cignal);
    unpack_cignal(serialized_cignal, &cignal);

    if (process_message(&cignal, device_record) == -1) {
        return fd;
    }
    if ((write(fd, serialize_cignal(cignal), CIGLEN)) < CIGLEN) {
        return fd;
    }

    return 0;
}
