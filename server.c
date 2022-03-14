//	C echo socket server, uses threading

#include <stdio.h>	 // printf
#include <string.h>	 // strlen
#include <stdlib.h>	 // strlen, atoi
#include <sys/socket.h>  // socket
#include <arpa/inet.h>	 // inet_addr
#include <unistd.h>	 // write
#include <pthread.h>     // for threading, link with -lpthread or -pthread

#define BACK_LOG 3 

const char* possibleFlags = "-ip [val], -host [num], -bufferSize [num]";
int bufferSize = 1024;

// the thread function connection

void *connection_handler(void *socket_desc) {
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char *message; char client_message[bufferSize];
	
	// send buffer size to the client
	int length = snprintf(NULL, 0, "%d", bufferSize) + 1; 
	char *bufferSizeStr = malloc(length);
	snprintf(bufferSizeStr, length, "%d", bufferSize);	
	write(sock, bufferSizeStr, length);
	free(bufferSizeStr);

	//Receive a message from client
	while ((read_size = recv(sock, client_message, bufferSize, 0)) > 0) {	
		fputs("recv: ", stdout);
		puts(client_message);
		// send the message to client
		if (write(sock, client_message, strlen(client_message)) < 0) {
			perror("ERROR: while writing to socket\n");
			break;
		} 
		// Clean buffer
    		memset(client_message, '\0', sizeof(client_message));
	}
	
	if (read_size == 0) {
		puts("INFO, Client disconnected."); // put to buffer
		fflush(stdout); // print to console
	} else if (read_size == -1) {
		perror("ERROR: receive failed.");
	}

	//Free the socket pointer
	free(socket_desc);
}

int main(int argc , char *argv[])
{	
	char* serverIp = "127.0.0.1";
	short serverHost = 8080; 
	
	// try to get command line params
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-ip") == 0 && i+1 < argc) {
			serverIp = argv[++i];
		} else if (strcmp(argv[i], "-host") == 0 && i+1 < argc) {
			serverHost = (short)atoi(argv[++i]);
		} else if (strcmp(argv[i], "-bufferSize") == 0 && i+1 < argc) {
			bufferSize = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-help") == 0) {
			printf("Creating TCP multiclient server. The possible flags are: %s.", possibleFlags);
			return 0;
		} else {
			printf("ERROR! Unknown flag: %s . The possible flags are: %s.", argv[i], possibleFlags);
			return -1;
		}
	}
	bufferSize = bufferSize > 1 ? bufferSize : 2; // limit buffer size
	int socket_desc, client_sock, c, *new_sock;
	struct sockaddr_in server , client;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("ERROR: Could not create socket.");
		return -1;
	}
	puts("INFO, socket created.");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(serverIp);
	server.sin_port = htons(serverHost);
	
	//Bind
	if( bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("ERROR: bind failed.");
		return -1;
	}
	puts("INFO, bind done.");
	
	// Listen
	listen(socket_desc, BACK_LOG);
	
	//Accept and incoming connection
	puts("INFO, Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
		puts("INFO, Connection accepted.");
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		if(pthread_create(&sniffer_thread, NULL, connection_handler, (void*) new_sock) < 0) {
			perror("ERROR: could not create thread.");
			break;
		}
		
		puts("INFO, New client connected.");
	}
	
	if (client_sock < 0) {
		perror("ERROR: socket accept failed.");
		return -1;
	}

    	close(client_sock);
    	close(socket_desc);
	return 0;
}
