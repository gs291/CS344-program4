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

int checkFileContents (const char *file, char *buffer) {
  int i = 0;
  FILE *fp;

  fp = fopen(file, "r");

  if(fp == NULL) {
    printf("Unable to open %s\n", file);
    perror("In checkFileContents\n");
    exit(1);
  }
  	memset(buffer, '\0', sizeof(buffer));
    if ( fgets(buffer, 1024, fp) !=NULL ) {
      buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
    }
    fclose(fp);

    while(buffer[i] != '\0') {
      if(buffer[i] > 90 || (buffer[i] < 65 && buffer[i] != 32))
        return 0;
      i++;
    }
    return 1;
}

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[1024], sendingMsg[2048];

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

	// Get input message from user
	// printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
  memset(sendingMsg, '\0', sizeof(sendingMsg)); // Clear out the sendingMsg array
  if(checkFileContents(argv[1], buffer) == 0) {
    fprintf(stderr, "otp_enc error: input contains bad characters\n");
    exit(1);
  }
  strcpy(sendingMsg, "@");
  strcat(sendingMsg, buffer);
  memset(buffer, '\0', sizeof(buffer));
  if(checkFileContents(argv[2], buffer) == 0) {
    fprintf(stderr, "otp_enc error: input contains bad characters\n");
    exit(1);
  }
  strcat(sendingMsg, "$");
  strcat(sendingMsg, buffer);
	// Send message to server
	charsWritten = send(socketFD, sendingMsg, strlen(sendingMsg), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(sendingMsg)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

  if(strcmp(buffer, "!key") == 0) {
    fprintf(stderr, "ERROR: key '%s' is too short\n", argv[2]);
  } else if (strcmp(buffer, "!con") == 0) {
    fprintf(stderr, "ERROR: could not contact otp_enc_d on port %d\n", portNumber);
  } else {
    printf("%s\n", buffer);
  }

	close(socketFD); // Close the socket
	return 0;
}
