/*
 * Name: Gregory Sanchez
 * File: Decryption server
 * Description: Server side use for decypting a message
 * Date: 11-30-2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int ciphLen = 0, keyLen = 0;
char cipher[75000], key[75000];

/*
 * Name: parse message
 * Description: parses the received message into the plain text and key
 * Return: 0 if the message was invalid, 1 if the message was parsed correctly
 */
int parseMsg(char *msg) {
	int i = 1;
	memset(cipher, '\0', sizeof(cipher));
	memset(key, '\0', sizeof(key));

	if(msg[0] == '@') { //If the message came from otp_enc
		while(msg[i] != '$') { //Loop through the message until the next special character signaling we read the entire cipher
			cipher[ciphLen] = msg[i];
			i++; ciphLen++;
		}
		if(msg[i] == '$') { // Double check we are ready to parse the key
			i++;
			while(msg[i] != '\0') { // Loop through the rest of the message to read the entire key
				key[keyLen] = msg[i];
				i++; keyLen++;
			}
		} else
			return 0;
	} else
		return 0;

	return 1;
}

/*
 * Name: decrypt message
 * Description: decrypts a message with the cipher text and key
 * Return: 0 if the key is not long enough, 1 if the message was sucessfully decrypted
 */
int decText (char *decMsg) {
	int i, cT, cK;

  memset(decMsg, '\0', 262144);
	if(keyLen >= ciphLen) { // Check the key is long enough for the cipher text
			for (i = 0; i < ciphLen; i++) { // Loop through the cipher length to create a new decrypted message
				cT = cipher[i] - 65; cK = key[i] - 65; // Convert ASCII codes 'A'-'Z' (65-90) to 0-25
				if(cT < 0) { cT = 26; } if (cK < 0) { cK = 26; } // If the ASCII code was 32 (space) then convert that to 26
				int cMsg = (cT - cK); // Subtract the plaint text and key character
        if(cMsg < 0) { cMsg+= 27; } // If the character is now negative add 27
        cMsg = cMsg % 27 + 65; // Then mod it by 27, then add 65 to the remainder of that
				if(cMsg == 91) {cMsg = 32;} // If the encrypted character is ']' then it should be a space ' '
				decMsg[i] = cMsg;
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
	    printf("SERVER[%d]:\n", charsWritten);
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
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, childExitStatus = -5;;
	pid_t spawnPid = -5;
	socklen_t sizeOfClientInfo;
	char msg[262144], decMsg[262144];
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

  	spawnPid = fork(); // Create a child process to handle the decrypting
  	switch (spawnPid) {
  	      case -1: {
  	        perror("Hull Breach!\n"); exit(1); break;
  	      }
  	    case 0: {  /* CHILD PROCESS */
					recvMsg(establishedConnectionFD, msg); // Get the entire message from the client
  	        if(parseMsg(msg) == 1) { // Parse the message
  						if(decText(decMsg) == 1) { // Decrypt the message
  							sendMsg(establishedConnectionFD, decMsg, ciphLen); // If the decryption was sucessfull, send the entire decryption to the client.
  							exit(0);
  						} else { // If the decryption was not sucessfull, send a server ERROR code "!key" to the client
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
  				pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0); /* Waits until the child completes */
  	      break;
  	    }
  	  }

  	close(establishedConnectionFD); // Close the existing socket which is connected to the client
  }
  close(listenSocketFD); // Close the listening socket
	return 0;
}
