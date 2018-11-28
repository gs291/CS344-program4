/*
 * Name: Gregory Sanchez
 * File: Encryption server
 * Description: Server side use for encypting a message
 * Date: 11-30-2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int textLen = 0, keyLen = 0, numOfProcess = 0;
char plaintext[75000], key[75000];

/*
 * Name: parse message
 * Description: parses the received message into the plain text and key
 * Return: 0 if the message was invalid, 1 if the message was parsed correctly
 */
int parseMsg(char *msg) {
	int i = 1;
	memset(plaintext, '\0', sizeof(plaintext));
	memset(key, '\0', sizeof(key));

	if(msg[0] == '$') { //If the message came from otp_enc
		while(msg[i] != '@') { //Loop through the message until the next special character signaling we read the entire plaintext
			plaintext[textLen] = msg[i];
			i++;
			textLen++;
		}
		if(msg[i] == '@') { // Double check we are ready to parse the key
			i++;
			while(msg[i] != '\0') { // Loop through the rest of the message to read the entire key
				key[keyLen] = msg[i];
				i++;
				keyLen++;
			}
		} else { return 0; }
	} else { return 0; }

	return 1;
}

/*
 * Name: encrypt message
 * Description: encrypts a message with the plain text and key
 * Return: 0 if the key is not long enough, 1 if the message was sucessfully encrypted
 */
int encText (char *encMsg) {
	int i, cT, cK;

	memset(encMsg, '\0', sizeof(encMsg));
	if(keyLen >= textLen) { // Check the key is long enough for the plain text
			for (i = 0; i < textLen; i++) { // Loop through the plaintext length to create a new encrypted message
				cT = plaintext[i] - 65; cK = key[i] - 65; // Convert ASCII codes 'A'-'Z' (65-90) to 0-25
				if(cT < 0) { cT = 26; } if (cK < 0) { cK = 26; } // If the ASCII code was 32 (space) then convert that to 26
				int cMsg = (cT + cK) % 27 + 65; // Add the plaint text and key character, mod it by 27, then add 65 to the remainder of that
				if(cMsg == 91) {cMsg = 32;} // If the encrypted character is ']' then it should be a space ' '
				encMsg[i] = cMsg;
			}
	} else
		return 0;

		return 1;
}

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/*
 * Name: send message
 * Description: sends a message to an established socket connection
 */
void sendMsg(int connectionFD, char *msg, int length) {
	// Send a Success message back to the client;
	char *ptr = msg;
	int charsWritten = send(connectionFD, msg, length, 0); // Write to the client
	if (charsWritten < 0) error("ERROR writing to socket");
	while (charsWritten < (length)) { // Keep looping if there are more characters to send
    ptr = msg + charsWritten;
    charsWritten += send(connectionFD, ptr, 1024, 0); // Write to the client
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  }
}

/*
 * Name: recieve message
 * Description: recieves a message from an established socket connection
 */
void recvMsg(int connectionFD, char *msg) {
	char buffer[1024];
	memset(buffer, '\0', 1024);
	memset(msg, '\0', 262144);

	int charsRead = recv(connectionFD, buffer, 1024, 0); // Read the client's message from the socket
	buffer[1024] = '\0';
	strcpy(msg, buffer);

	if (charsRead < 0) error("ERROR reading from socket");
	while (buffer[1024-1] != '\0') { //Keep looping if there are possibly more characters to recieve
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
	char msg[262144], encMsg[262144];
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

	while(1) { // Infinitely loop checking for any new connections
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		spawnPid = fork(); // Create a child process to handle the encypting
		switch (spawnPid) {
		      case -1: {
		        perror("Hull Breach!\n"); exit(1); break;
		      }
		    case 0: {  /* CHILD PROCESS */
					memset(msg, '\0', 262144);
					recvMsg(establishedConnectionFD, msg); // Get the entire message from the client
		        if(parseMsg(msg) == 1) { // Parse the message
							if(encText(encMsg) == 1) { // Encypt the message
								sendMsg(establishedConnectionFD, encMsg, textLen); // If the encyption was sucessfull, send the entire encyption to the client.
								exit(0);
							} else { // If the encyption was not sucessfull, send a server ERROR code "!key" to the client
								sendMsg(establishedConnectionFD, "!key", 4);
								exit(2);
							}
						} else { // If the parsing was not sucessfull, send a server ERROR code "!con" to the client
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
