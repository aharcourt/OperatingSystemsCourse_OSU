/* 
File: otp_enc_d.c
Author: Adeline Harcourt (based on skeleton server.c code from Professor Benjamin
		Brewster, OSU CS344 Spring 2017 Semester)
Description: A program that runs in the background and simulates a daemon. The
		function of the daemon is to perform the encoding of a plain text file.
		Up to five concurrent connections can be made to this daemon from 
		otp_enc for encoding purposes. Otp_enc is the client that connects to
		otp_enc_d and sends it plain text and a key. Otp_enc_d then returns 
		the cipher text back to otp_enc.
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
	int i, fileSizeS, listenSocketFD, establishedConnectionFD, portNumber, tempChars, charsRead, charsWritten;
	socklen_t sizeOfClientInfo;
	pid_t spawnpid = -5;
	char idBuffer[4];
	memset(idBuffer, '\0', sizeof(idBuffer));
	struct sockaddr_in serverAddress, clientAddress;

	// Check input format from user
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
	if (listenSocketFD < 0) error("otp_enc_d could not open socket", 1);

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("otp_enc_d could not bind socket", 1);
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	
	// Keep the daemon running
	while (1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("otp_enc_d could not accept connection", 1);
		
		// Spawn child process to perform encryption
		spawnpid = fork();
		switch (spawnpid) {
			case -1:
				// Error with fork
				error("otp_enc_d had issue forking", 1);
				break;
			case 0:
				// In child process:
				// Get the client identifier from otp_enc
				charsRead = 0;
				charsRead = recv(establishedConnectionFD, idBuffer, 3, 0);
				if(charsRead != 3) error("otp_enc_d had issue reading from socket", 1);	
			
				// Verify that connection is with otp_enc
				if(strcmp(idBuffer, "ENC") != 0) {
					strcpy(idBuffer, "NO");
				}
				else {
					strcpy(idBuffer, "OK");
				}

				// Send verification to otp_enc - OK if otp_enc, NO if other
				charsWritten = 0;
				charsWritten = send(establishedConnectionFD, idBuffer, 2, 0);			    
				if (charsWritten != 2) error("otp_enc_d had issue writing to socket", 1);
				if (strcmp(idBuffer, "OK") != 0) exit(1);

				// Get the incoming plain text size from otp_enc
				charsRead = 0;
				charsRead = recv(establishedConnectionFD, &fileSizeS, sizeof(fileSizeS), 0);
				if(charsRead != sizeof(fileSizeS)) error("otp_enc_d had issue reading from socket", 1);	
				if (fileSizeS < 0) error("ERROR reading fileSize", 1);
				
				// Create buffers to store plain text and key text
				char buffer[fileSizeS];
				char plainText[fileSizeS];
				memset(buffer, '\0', sizeof(buffer));
				memset(plainText, '\0', sizeof(plainText));
	
				// Read plain text into buffer
				charsRead = 0;
				while (charsRead < (sizeof(buffer) - 1)) {
					tempChars = recv(establishedConnectionFD, &buffer[charsRead], sizeof(buffer)-charsRead-1, 0);
					if (tempChars < 0) error("otp_enc_d had issue reading from socket", 1);
					charsRead += tempChars;
				}
				strcpy(plainText, buffer);
			
				// Read key text into buffer
				memset(buffer, '\0', sizeof(buffer));
				charsRead = 0;
				while (charsRead < sizeof(buffer)-1) {
					tempChars = recv(establishedConnectionFD, &buffer[charsRead], 1, 0); 
					if (tempChars < 0) error("otp_enc_d had issue reading from socket", 1);
					charsRead += tempChars;
				}
	
				// Generate ciphertext
				for (i = 0; i < sizeof(buffer) - 1; i++) {
					// Replace space chars with 91 for proper modulus
					if(buffer[i] == 32) buffer[i] = 91;
					if(plainText[i] == 32) plainText[i] = 91;
					buffer[i] = (char)((buffer[i] + plainText[i] - 130) % 27 + 65);
					
					// Replace 91s with space chars
					if (buffer[i] == 91) {
						buffer[i] = ' ';
					}
				}	
	
				// Write ciphertext back to otp_enc
				charsWritten = 0;
				while (charsWritten < sizeof(buffer) - 1) {
					tempChars = send(establishedConnectionFD, &buffer[charsWritten], sizeof(buffer) - charsWritten - 1, 0); 
					if (tempChars < 0) error("otp_enc_d had issue writing to socket", 1);
					charsWritten += tempChars;
				}
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				break;
		}
	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
