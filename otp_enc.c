/*
 * Name: Gregory Sanchez
 * File: Encryption Client
 * Description: Client side use for encypting a message
 * Date: 11-30-2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

/*
 * Name: Check File Contents
 * Description: opens a given file and outputs it into the buffer.
 *    Then check that the file contains no invalid characters
 * Return: 0 in the file contains invalide characters or the number of characters in the file
 */
int checkFileContents (const char *file, char *buffer) {
  int i = 0;
  FILE *fp;

  fp = fopen(file, "r");

  if(fp == NULL) { error("Unable to open file\n"); }
  	memset(buffer, '\0', sizeof(buffer));
    if ( fgets(buffer, 75000, fp) !=NULL ) { //Gets only the first line of the file and checks its not empty
      buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
    }
    fclose(fp);

    while(buffer[i] != '\0') { //Scan through the entire buffer to check for invalid characters
      if(buffer[i] > 90 || (buffer[i] < 65 && buffer[i] != 32))
        return 0;
      i++; //Keep count on the file size
    }
    return i; //return the file size if it's a valid file
}

/*
 * Name: send message
 * Description: sends a message to an established socket connection
 */
void sendMsg(int connectionFD, char *msg, int length) {
	// Send a Success message back to the client;
	char *ptr = msg;
	int charsWritten = send(connectionFD, msg, 1024, 0);  //Write to the server
	if (charsWritten < 0) error("ERROR writing to socket");
	while (charsWritten < (length)) { // Keep looping if there are more characters to send
    ptr = msg + charsWritten;
    charsWritten += send(connectionFD, ptr, 1024, 0); // Write to the server
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
	memset(msg, '\0', 75000);

  //Wait till the server responds
	int charsRead = recv(connectionFD, buffer, 1024, 0); // Read the client's message from the socket
	buffer[1024] = '\0';
	strcpy(msg, buffer);

	if (charsRead < 0) error("ERROR reading from socket");
	while (buffer[1024-1] != '\0') {  //Keep looping if there are possibly more characters to recieve
		memset(buffer, '\0', 1024);
		charsRead = recv(connectionFD, buffer, 1024, 0); // Read the client's message from the socket
		buffer[1024] = '\0';
		if (charsRead < 0) error("ERROR reading from socket");
		strcat(msg, buffer);
	}
}

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead, totalSize = 2;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[75000], sendingMsg[262144];

	if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
  memset(sendingMsg, '\0', sizeof(sendingMsg)); // Clear out the sendingMsg array

  int file1Size = checkFileContents(argv[1], buffer); //Get the contents of the plain text
  if(file1Size == 0) { fprintf(stderr, "otp_enc error: input contains bad characters\n"); exit(1); }

  //Create a special string that will be sent to the server
  //This string will have the format of "$plaintext@key"
  //Only the otp_enc_d server will interpret this string
  strcpy(sendingMsg, "$");
  strcat(sendingMsg, buffer);
  memset(buffer, '\0', sizeof(buffer));

  int file2Size = checkFileContents(argv[2], buffer); //Get the contents of the key text
  if(file2Size == 0) { fprintf(stderr, "otp_enc error: input contains bad characters\n"); exit(1); }

  totalSize = totalSize + file1Size + file2Size;
  strcat(sendingMsg, "@");
  strcat(sendingMsg, buffer);

  sendMsg(socketFD, sendingMsg, totalSize); //Send the "$plaintext@key" string to the server

  recvMsg(socketFD, buffer); //Wait till the server responds

  //Check what the server reponse was
  if(strcmp(buffer, "!key") == 0) { // Server ERROR code signaling the key was too short
    fprintf(stderr, "ERROR: key '%s' is too short\n", argv[2]);
    exit(1);
  } else if (strcmp(buffer, "!con") == 0) { // Server ERROR code signaling there was an attempted invalid connection
    fprintf(stderr, "ERROR: could not contact otp_enc_d on port %d\n", portNumber);
    exit(2);
  } else { // Else print the encrypted message from the server
    printf("%s\n", buffer);
  }

	close(socketFD); // Close the socket
	return 0;
}
