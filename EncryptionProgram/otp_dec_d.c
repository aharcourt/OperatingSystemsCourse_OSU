/* 
File: otp_dec_d.c
Author: Adeline Harcourt (based on skeleton server.c code from Professor Benjamin
		Brewster, OSU CS344 Spring 2017 Semester)
Description: A program that runs in the background and simulates a daemon. The
		function of the daemon is to perform the decoding of a encrypted file.
		Up to five concurrent connections can be made to this daemon from 
		otp_dec for decoding purposes. Otp_dec is the client that connects to
		otp_dec_d and sends it encryted text and a key. Otp_dec_d then returns 
		the unencrypted text back to otp_dec.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues with custom exit value
void error(const char *msg, int exitVal) { 
	fprintf(stderr, "ERROR: %s\n", msg); 
	exit(exitVal); 
} 

int main(int argc, char *argv[])
{
	int i, tempInt, fileSizeS, listenSocketFD, establishedConnectionFD, portNumber, 
	    tempChars, charsRead, charsWritten; 
	socklen_t sizeOfClientInfo;
	pid_t spawnpid = -5;
	char idBuffer[4];
	memset(idBuffer, '\0', sizeof(idBuffer));
	struct sockaddr_in serverAddress, clientAddress;

	// Check the usage and args entered by user
	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
		exit(1); 
	}

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("otp_dec_d could not open socket", 1);

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
		error("otp_dec_d could not bind socket", 1);
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	
	// Keep the daemon running
	while (1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("otp_dec_d could not accept connection", 1);
		
		// Spawn child process to take over decryption
		spawnpid = fork();
		switch (spawnpid) {
			case -1:
				// Check for fork error
				error("otp_dec_d had issue forking", 1); 
				break;
			case 0:
				// In child:
				// Get the client identifier from otp_dec
				charsRead = 0;
				charsRead = recv(establishedConnectionFD, idBuffer, 3, 0);
				if(charsRead != 3) error("otp_dec_d had issue reading from socket", 1);	
			
				// Validate that the connection is to otp_dec
				if(strcmp(idBuffer, "DEC") != 0) {
					strcpy(idBuffer, "NO");
				}
				else {
					strcpy(idBuffer, "OK");
				}

				// Send verification to otp_dec - OK if otp_dec, NO if other
				charsWritten = 0;
				charsWritten = send(establishedConnectionFD, idBuffer, 2, 0);			    
				if (charsWritten != 2) error("otp_dec_d had issue writing to socket", 1);
				if (strcmp(idBuffer, "OK") != 0) exit(1);

				// Get the incoming ciphertext size from otp_dec
				charsRead = 0;
				charsRead = recv(establishedConnectionFD, &fileSizeS, sizeof(fileSizeS), 0);
				if(charsRead != sizeof(fileSizeS)) error("otp_dec_d had issue reading from socket", 1);	
				if (fileSizeS < 0) error("ERROR reading fileSize", 1);
				
				// Create buffers to hold key text and cipher text
				char buffer[fileSizeS];
				char cipherText[fileSizeS];
				memset(buffer, '\0', sizeof(buffer));
				memset(cipherText, '\0', sizeof(cipherText));
		
				// Read cipher text into buffer
				charsRead = 0;
				while (charsRead < (sizeof(buffer) - 1)) {
					tempChars = recv(establishedConnectionFD, &buffer[charsRead], sizeof(buffer)-charsRead-1, 0); 
					if (tempChars < 0) error("otp_dec_d had issue reading from socket", 1);
					charsRead += tempChars;
				}
				strcpy(cipherText, buffer);
				
				// Read key text from client
				memset(buffer, '\0', sizeof(buffer));
				charsRead = 0;
				while (charsRead < sizeof(buffer)-1) {
					tempChars = recv(establishedConnectionFD, &buffer[charsRead], 1, 0); 
					if (tempChars < 0) error("otp_dec_d had issue reading from socket", 1);
					charsRead += tempChars;
				}
	
				// Decode cipher
				for (i = 0; i < sizeof(buffer) - 1; i++) {
					// Replace space chars with ASCII 91 for proper modulus
					if(buffer[i] == 32) buffer[i] = 91;
					if(cipherText[i] == 32) cipherText[i] = 91;
					
					// Modulus and account for negative numbers
					tempInt = (cipherText[i] - buffer[i]) % 27;
					if(tempInt < 0) tempInt += 27; 
					
					// Add 65 to reach uppercase alphabet chars
					buffer[i] = (char)(tempInt + 65);

					// Replace all 91s with space chars
					if (buffer[i] == 91) {
						buffer[i] = ' ';
					}
				}	
	
				// Write plain text back to client
				charsWritten = 0;
				while (charsWritten < sizeof(buffer) - 1) {
					tempChars = send(establishedConnectionFD, &buffer[charsWritten], sizeof(buffer) - charsWritten - 1, 0); 
					if (tempChars < 0) error("otp_dec_d had issue writing to socket", 1);
					charsWritten += tempChars;
				}
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				break;
		}
	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
