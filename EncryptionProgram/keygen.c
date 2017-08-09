/* 
File: keygen.c
Author: Adeline Harcourt 
Description: A program that generates a one-time pad key text file
		of a specified length filled with random uppercase letters 
		and space characters.
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
 
int main(int argc, char *argv[]) {
	int i, randNum, keylength;
	srand(time(NULL));

	// Check user input format
	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s keylength\n", argv[0]); 
		exit(0); 
	} 
	
	// Get length of key to be generated
	keylength = atoi(argv[1]);	

	// Generate random key and print to stdout
	for (i = 0; i < keylength; i++) {
		randNum = rand() % 27 + 65;
		
		// replace 91 with spaces
		if (randNum == 91) {
			printf(" ");
		}
		else {
			printf("%c", (char)randNum);
		}
	}
	
	// Add newline char
	printf("\n");
	
	return 0;
}
