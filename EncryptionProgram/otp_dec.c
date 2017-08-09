/* 
File: otp_dec.c
Author: Adeline Harcourt (based on skeleton client.c code from Professor Benjamin
		Brewster, OSU CS344 Spring 2017 Semester)
Description: A program that runs in the foreground and connects to otp_dec_d. The
		function of the program is to send a cipher text and key file to the server
		(otp_dec_d on a specified port for decoding.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

// Error function used for reporting issues
void error(const char *msg, int exitVal) { 
	fprintf(stderr, "ERROR: %s\n", msg); 
	exit(exitVal); 
} 

int main(int argc, char *argv[])
{
	int i, bufferSize, fileSizeC, socketFD, portNumber, tempChars, charsWritten, charsRead;
	char servVer[3];
	memset(servVer, '\0', sizeof(servVer));
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	
    // Check user input format
	if (argc < 4) { fprintf(stderr,"USAGE: %s ciphertextfile keyfile port\n", argv[0]); exit(0); } 

	// Open ciphertext file and key file
	FILE *cipherFile = fopen(argv[1], "r");
	FILE *keyFile = fopen(argv[2], "r");
	
	// Display error and exit if files cannot be opened
	if (cipherFile == NULL) error("Can't open ciphertext file", 1);
	if (keyFile == NULL) error("Can't open key file", 1);
	
	// Get size of cipher file
	fseek(cipherFile, 0L, SEEK_END);
	fileSizeC = ftell(cipherFile);
	fseek(cipherFile, 0L, SEEK_SET);
	
	// If key file is not long enough, display error and exit
	fseek(keyFile, 0L, SEEK_END);
	if (ftell(keyFile) < fileSizeC) error("Key file is too short", 1);
	fseek(keyFile, 0L, SEEK_SET);

	// Create buffers to hold cipher and key text
	char buffer[fileSizeC];
	char key[fileSizeC];

	// Input cipher text into buffer from file
	memset(buffer, '\0', sizeof(buffer)); 
	fread(buffer, fileSizeC, 1, cipherFile);
	buffer[fileSizeC-1] = '\0';
	fclose(cipherFile);
	bufferSize = sizeof(buffer);

	// Input key text into buffer
	memset(key, '\0', sizeof(key)); 
	fread(key, fileSizeC, 1, keyFile);
	key[fileSizeC-1] = '\0';
	fclose(keyFile);

	// Verify that cipher text and key characters are valid
	for (i = 0; i < (sizeof(buffer) -1); i++) {
		if ((buffer[i] < 65 || buffer[i] > 90) && buffer[i] != 32) {
			fprintf(stderr, "ERROR: %s contains bad characters\n", argv[1]); exit(1); 
		}
		if ((key[i] < 65 || key[i] > 90) && key[i] != 32) {
			fprintf(stderr, "ERROR: %s contains bad characters\n", argv[2]); exit(1); 
		}
	}	

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) {
		fprintf(stderr, "ERROR: bad port %d\n", portNumber); exit(2); 
	} // Check usage & args
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
		fprintf(stderr, "ERROR: bad port %d\n", portNumber); exit(2); 
	} // Check usage & args	
	
	// Send client identifier to server
	charsWritten = 0;
	charsWritten = send(socketFD, "DEC", 3, 0); 
	if (charsWritten != 3) error("otp_dec had issue writing to socket", 1);
	
	// Get OK from server (if allowed to connect)
	charsRead = 0;
	charsRead = recv(socketFD, servVer, 2, 0); 
	if (charsRead != 2) error("otp_dec had issue reading from socket", 1);
	if (strcmp(servVer, "OK") != 0) error("otp_dec is not verified to connect to otp_enc_d", 1);
	
	// Send size of cipher text buffer to server
	charsWritten = 0;
	charsWritten = send(socketFD, &bufferSize, sizeof(int), 0); 
	if (charsWritten != sizeof(int)) error("otp_dec had issue writing to socket", 1);
	
	// Send cipher text to server
	charsWritten = 0;
	while (charsWritten < sizeof(buffer) - 1) {
		tempChars = send(socketFD, &buffer[charsWritten], sizeof(buffer) - charsWritten - 1, 0);
		if (tempChars < 0) error("otp_dec had issue writing to socket", 1);
		charsWritten += tempChars;
	}
	
	// Send key text to server
	charsWritten = 0;
	while (charsWritten < sizeof(key) - 1) {
		tempChars = send(socketFD, &key[charsWritten], sizeof(key) - charsWritten - 1, 0); 
		if (tempChars < 0) error("otp_dec had issue writing to socket", 1);
		charsWritten += tempChars;
	}
	// Get decoded message from server
	memset(buffer, '\0', sizeof(buffer)); 
	charsRead = 0;
	while (charsRead < sizeof(buffer)-1) {
		tempChars = recv(socketFD, &buffer[charsRead], sizeof(buffer) - charsRead - 1, 0);
		if (tempChars < 0) error("otp_dec had issue reading from socket", 1);
		charsRead += tempChars;
	}
	
	// Print message to stdout
	printf("%s\n", buffer);

	close(socketFD); // Close the socket
	return 0;
}
