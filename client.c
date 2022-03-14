//	C echo client

#include <stdio.h>	// printf
#include <stdlib.h>	// strlen, atoi
#include <string.h>	// strlen
#include <sys/socket.h>	// socket
#include <arpa/inet.h>	// inet_addr
#include <unistd.h>     // sleep
#include <pthread.h>    // for threading, link with -lpthread or -pthread

const char* possibleFlags = "\n    -ip [val] the server ip to connect to\n    -host [num] the server port to connect to\n";
int bufferSize = 1024;

void *receiverThreadFunc(void *socket_desc) {	
	int ret = 1;
	int sock = *(int*)socket_desc;
	char server_reply[bufferSize];
	size_t messageLen = 0;
	while (1) {
		// clearing the buffer
		memset(server_reply,'\0',sizeof(server_reply));
		
		messageLen = recv(sock, server_reply, bufferSize, 0);
		//Receive a reply from the server
		if (messageLen < 1) {
			if(messageLen < 0) {
				puts("ERROR: receive failed in thread.");
				break;
			}
			puts("ERROR: empty message received in thread.");
			break; 
		}
		
		fputs("Server reply: ", stdout);
		puts(server_reply);
	}
	close(sock);
	exit(0);
	//pthread_exit(&ret);
}

void *senderThreadFunc(void *socket_desc) {
	int sock = *(int*)socket_desc;
	int ret = 1;	
	char message[bufferSize];
	puts("INFO, Now can begin entering messages to server!");
	while (1) {
		// clearing the buffer
		memset(message,'\0',sizeof(message));
			
		//scanf("%s", &message);
		fgets(message, sizeof message, stdin);
		if (strlen(message) > bufferSize) {
			puts("Warning: message is long, will be divided to packets.");
		}
		// send the data
		if(send(sock, message, strlen(message), 0) < 0) {
			puts("ERROR: send failed in thread.");
			break;
		}
		//sleep(0.1);
	}
	close(sock);
	exit(0);
	//pthread_exit(&ret);
}

int main(int argc , char *argv[]) {
	
	char* serverIp = "127.0.0.1";
	short serverHost = 8080; 
	
	// try to get command line params
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-ip") == 0 && i+1 < argc) {
			serverIp = argv[++i];
		} else if (strcmp(argv[i], "-host") == 0 && i+1 < argc) {
			serverHost = (short)atoi(argv[++i]);
		} else if (strcmp(argv[i], "-help") == 0) {
			printf("Creating TCP client. The possible flags are: %s.", possibleFlags);
			return 0;
		} else {
			printf("Error! Unknown flag: %s . The possible flags are: %s.", argv[i], possibleFlags);
			return -1;
		}
	}
	
	//Create socket
	int sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) {
		perror("ERROR: Could not create socket.");
		return -1;
	}
	puts("INFO, Socket created.");
	
	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(serverIp);
	server.sin_family = AF_INET;
	server.sin_port = htons(serverHost);

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("ERROR: Connection failed.");
		return -1;
	}
	
	puts("INFO, Connected.");
	char serverFirstReply[bufferSize];
	if(recv(sock, serverFirstReply, bufferSize, 0) < 0) {
		puts("ERROR: receive failed.");
		return -1;
	}
	bufferSize = atoi(serverFirstReply);	
	printf("INFO, buffer size: %s.\n", serverFirstReply);
	
	// keep communicating with server, async input / output

	pthread_t receiverThread;
	if(pthread_create(&receiverThread, NULL, receiverThreadFunc, &sock) < 0) {
		perror("ERROR: could not create receiver thread.");
		return -1;
	}
	pthread_t senderThread;
	if(pthread_create(&senderThread, NULL, senderThreadFunc, &sock) < 0) {
		perror("ERROR: could not create sender thread.");
		return -1;
	}
	pthread_join(receiverThread, NULL);
	pthread_join(senderThread, NULL);
	sleep(0.5);
	// properly close connection
	close(sock);
	puts("INFO, client successfully closed.");
	return 0;
}