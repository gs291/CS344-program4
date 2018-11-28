#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int textLen = 0, keyLen = 0, numOfProcess = 0;
char plaintext[75000], key[75000];

int parseMsg(char *msg) {
	int i = 1;
	memset(plaintext, '\0', sizeof(plaintext));
	memset(key, '\0', sizeof(key));

	if(msg[0] == '$') {
		while(msg[i] != '@') {
			plaintext[textLen] = msg[i];
			i++;
			textLen++;
		}
		if(msg[i] == '@') {
			i++;
			while(msg[i] != '\0') {
				key[keyLen] = msg[i];
				i++;
				keyLen++;
			}
		} else { return 0; }
	} else { return 0; }

	return 1;
}

int encText (char *encMsg) {
	int i, cT, cK;

	if(keyLen >= textLen) {
			for (i = 0; i < textLen; i++) {
				cT = plaintext[i] - 65; cK = key[i] - 65;
				if(cT < 0) { cT = 26; } if (cK < 0) { cK = 26; }
				int cMsg = (cT + cK) % 27 + 65;
				if(cMsg == 91) {cMsg = 32;}
				encMsg[i] = cMsg;
			}
	} else
		return 0;

		return 1;
}

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

void sendMsg(int connectionFD, char *msg, int length) {
	// Send a Success message back to the client;
	char *ptr = msg;
	int charsWritten = send(connectionFD, msg, length, 0); // Send success back
	if (charsWritten < 0) error("ERROR writing to socket");
	while (charsWritten < (length)) {
    ptr = msg + charsWritten;
    charsWritten += send(connectionFD, ptr, 1024, 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  }
}

void recvMsg(int connectionFD, char *msg) {
	char buffer[1024];
	memset(buffer, '\0', 1024);
	memset(msg, '\0', 262144);

	int charsRead = recv(connectionFD, buffer, 1024, 0); // Read the client's message from the socket
	buffer[1024] = '\0';
	strcpy(msg, buffer);

	if (charsRead < 0) error("ERROR reading from socket");
	while (buffer[1024-1] != '\0') {
		memset(buffer, '\0', 1024);
		charsRead = recv(connectionFD, buffer, 1024, 0); // Read the client's message from the socket
		buffer[1024] = '\0';
		if (charsRead < 0) error("ERROR reading from socket");
		strcat(msg, buffer);
	}
}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, childExitStatus = -5;
	pid_t spawnPid = -5;
	socklen_t sizeOfClientInfo;
	char msg[262144], encMsg[131072];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		spawnPid = fork();
		switch (spawnPid) {
		      case -1: {
		        perror("Hull Breach!\n"); exit(1); break;
		      }
		    case 0: {  /* CHILD PROCESS */
					// Get the message from the client and display it
					memset(msg, '\0', 262144);
					recvMsg(establishedConnectionFD, msg);
		        if(parseMsg(msg) == 1) {
							if(encText(encMsg) == 1) {
								sendMsg(establishedConnectionFD, encMsg, textLen/*encMsg, textLen*/);
								exit(0);
							} else {
								sendMsg(establishedConnectionFD, "!key", 4);
								exit(2);
							}
						} else {
							sendMsg(establishedConnectionFD, "!con", 4);
			      	exit(2);
						}
						 break;
		    }
		    default: {  /* PARENT PROCESS */
					numOfProcess++;
					pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0); /* Waits until the child completes */
		      break;
		    }
		  }


		close(establishedConnectionFD); // Close the existing socket which is connected to the client
	}
	close(listenSocketFD); // Close the listening socket
	return 0;
}
