// File: smallsh.c
// Author: Adeline Harcourt
// Description: The shell program for CS344 Program 3 - Spring 2017. This 
// is a shell program with 3 built-in commands and background processing 
// capabilities. The syntax for a command line entry is as follows:
// "command [arg1 arg2 ...] [< input_file] [> output_file] [&]" where
// items in brackets are optional.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <wait.h>

// GLOBAL VARIABLES
pid_t childPIDs[500];	//Array of background processes not cleaned up yet
int numPIDs = 0;		//Number of background processes in childPID array
int backgroundMode = 0;		//0 = background mode can be turned on, 1 = can't

// The SIGTSTP function handler (only used by the parent process). This 
// signal causes the parent process to enter "foreground-only" mode, 
// which ignores all "&" symbol entries. If the process is already
// in foreground-mode, the signal reverts it back to normal mode.
void CatchSIGTSTP(int signo) {
	if (backgroundMode) {
		char *message = "\nexiting foreground-only mode\n: ";
		write(STDOUT_FILENO, message, 32);
		backgroundMode = 0;		// Set backgroundMode switch
	}
	else {
		char *message = "\nentering foreground-only mode (& is now ignored)\n: ";
		write(STDOUT_FILENO, message, 52);
		backgroundMode = 1;		// Set backgroundMode switch
	}
}

// A function that sets the signal actions for the SIGINT and SIGTSTP signals.
void SetSigActions(struct sigaction *SIGINT_action, struct sigaction *SIGTSTP_action) {
	SIGINT_action->sa_handler = SIG_IGN;	// Parent should ignore SIGINT

	SIGTSTP_action->sa_handler = CatchSIGTSTP;	
	SIGTSTP_action->sa_flags = SA_RESTART;	// Restart flag for getline()
	
	sigaction(SIGINT, SIGINT_action, NULL);
	sigaction(SIGTSTP, SIGTSTP_action, NULL);	
}

// A function that terminates all background processes that have not yet been cleaned up.
void TerminateChildren() {
	int i;
	int childExitMethod;

	for (i = 0; i < numPIDs; i++) {
		pid_t childPID = childPIDs[i];	// Get PID from childPIDs array
		
		childPID = waitpid(childPID, &childExitMethod, WNOHANG); // Check if done
		if (childPID == 0) {	// If still running, kill it and wait for it to die
			kill(childPIDs[i], SIGTERM);
			waitpid(childPIDs[i], &childExitMethod, 0);
		}
	}
}

// A function that handles the built-in "cd" command. If no path is specified, the function
// sets the working directory to be the home directory. Otherwise, it takes you to the specified
// path.
void CD(char *path) {
	int result;		
	char* home = getenv("HOME");
	
	if (path == NULL) {
		result = chdir(home);	// No path specified, go to home dir
	}
	else {
		result = chdir(path);	// Change dir to specified path
	}
	if (result == -1) {
		printf("Error! Not a valid directory: %s\n", path);
		fflush(stdout);
	}
}

// A function that handles the built-in "status" command. This function
// accepts integers corresponding to the exit status and signal termination
// status of the last completed foreground process. The function prints a 
// message about how this last process was terminated.
void PrintStatus(int fgExitStat, int fgTermSig) {
	if (fgExitStat == -1 && fgTermSig == -1) {	// If both are -1, no fg processes have run
		printf("No foreground processes run\n");
		fflush(stdout);
	}
	else if (fgExitStat != -1) {	// If exited, display exit status
		printf("exit value %d\n", fgExitStat);
		fflush(stdout);
	}
	else if (fgTermSig != -1) {		// If signal terminated, display signal number
		printf("terminated by signal %d\n", fgTermSig);
		fflush(stdout);
	}
}

// This function tests for and handles an error. The function accepts a
// result code, an error value, and an error message. If the result code is 
// equal to the error value, the function displays the message and then exits 
// with a status of 1.
void PrintError(int error, int erVal, char *message) {
	if (error == erVal) {
		printf("%s\n", message);
		fflush(stdout);
		exit(1);
	}
}

// SOURCE: Adapted from solution from "The Paramagnetic Croissant": stackoverflow.com/questions/
// 32413667/replace-all-occurrences-of-a-substring-in-a-string-in-c.
// This function replaces all instances of "$$" within the text string with the current PID.
void ReplacePID(char *text) {
	char tempString[2049];	// Buffer to build up new text
	memset(tempString, '\0', sizeof(tempString));	// Clear string
	char *insertPt = &tempString[0];	// Insertion point of copied text
	char *tmp = text;	// Temp pointer to text string
	char myPID[20];		// Store string version of process ID
	sprintf(&myPID[0], "%d", (int)getpid());
	int myPIDLen = strlen(myPID);	// Length of process ID

	while (1) {
		char *p = strstr(tmp, "$$");	// Find next instance of $$
		
		// Copy over last section of string after $$
		if (p == NULL) {
			strcpy(insertPt, tmp);
			break;
		}

		// Copy part before $$
		memcpy(insertPt, tmp, p - tmp);
		insertPt += p - tmp;

		// Copy over PID text
		memcpy(insertPt, myPID, myPIDLen);
		insertPt += myPIDLen;
		
		// Adjust pointer
		tmp = p + 2;
	}
	// Copu string back into place
	strcpy(text, tempString);
}

int main() {
	size_t bufferMax = 2049;	// Max input from user
	char* commandEntry = NULL;	// String pointer to store user input
	int i, j;			// Iterators
	int numCharEnt;			// Number of chars from getline()
	int fgExitStat = -1;		// Exit status from last fg process (-1 if N/A)
	int fgTermSig = -1;		// Termination signal from last fg process (-1 if N/A)
	struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};	//Sigaction structs for SIGINT, SIGTSTP
	
	SetSigActions(&SIGINT_action, &SIGTSTP_action);
	
	while (1) {
		// Check for finished background processes
		for (i = 0; i < numPIDs; i++) {
			int childExitMethod;
			pid_t childPID = childPIDs[i];
			
			childPID = waitpid(childPID, &childExitMethod, WNOHANG);
			if (childPID != 0) {		// If finished
				if (WIFEXITED(childExitMethod)) {	// If exited, display exit value
					printf("background pid %d is done: exit value %d\n", 
						childPID, WEXITSTATUS(childExitMethod)); 
					fflush(stdout);
				}
				else {		// If terminated, display termination signal
					printf("background pid %d is done: terminated by signal %d\n", 
						childPID, WTERMSIG(childExitMethod)); 
					fflush(stdout);
				}
				// Reduce number of background PIDs in childPID array
				numPIDs--;
				// Move over remaining PIDs in array to avoid hole
				for ( j = i; j < numPIDs; j++ ) {
					childPIDs[j] = childPIDs[j + 1];
				}
				i--; // Set i back so that new PID in old PID spot is not skipped
			}
		}
		// PROMPT USER
		printf(": ");
		fflush(stdout);
		numCharEnt = getline(&commandEntry, &bufferMax, stdin);
		if (numCharEnt == -1) {
			clearerr(stdin);
		}
		// Replace $$ with PID
		ReplacePID(commandEntry);	

		// PARSE INPUT
		char *token = strtok(commandEntry, " \n");
		// Ignore blank or commented lines
		if (strlen(commandEntry) < 2 || commandEntry[0] == '#') {}
		else if (strstr(token, "exit") != NULL) {	// Handle exit command
			TerminateChildren();
			exit(0);
		}
		else if (strstr(token, "status") != NULL) {	// Handle status command
			PrintStatus(fgExitStat, fgTermSig);
		}
		else if (strstr(token, "cd") != NULL) {		// Handle cd command
			CD(strtok(NULL, " \n"));
		}
		else {	
			int isBackground = 0;	// 1 = background process, 0 = foreground process
			char inputFile[256];	// String to hold input file name, if provided
			memset(inputFile, '\0', sizeof(inputFile));
			char outputFile[256];   // String to hold output file name, if provided
			memset(outputFile, '\0', sizeof(outputFile));
			char *args[513];		// Array to hold command arguments
			i = 0;					// Initialize i to represent arg array index
			
			// Get command and args
			while (token != NULL && (strcmp(token, "<") != 0 && strcmp(token, ">") != 0)) {
				args[i] = token;		
				token = strtok(NULL, " \n");
				i++;
			}
			// Get input file
			if (token != NULL && strcmp(token, "<") == 0) {
				strcpy(inputFile, strtok(NULL, " \n"));
				token = strtok(NULL, " \n");
			}		
			// Get output file
			if (token != NULL && strcmp(token, ">") == 0) {
				strcpy(outputFile, strtok(NULL, " \n"));
				token = strtok(NULL, " \n");
			}
			// Set background mode (if allowed by SIGTSTP signal)
			if ((token != NULL && strstr(token, "&") != NULL) || strcmp(args[i - 1], "&") == 0) {
				if (backgroundMode == 0) {
					isBackground = 1;
				}
				if (strstr(args[i - 1], "&") != NULL) {
					i--;	// Do not include & in argument list
				}
			}
			else {
				isBackground = 0;
			}
			args[i] = NULL;		// Add NULL to end of args array
			
			// If background process with no input/output files, reroute to /dev/null
			if(isBackground == 1) {
				if (inputFile[0] == '\0') 
					strcpy(inputFile, "/dev/null");
				if (outputFile[0] == '\0') 
					strcpy(outputFile, "/dev/null");
			}
			
			// Fork child process
			childPIDs[numPIDs] = fork();
			int inputFD, outputFD, result;
			PrintError(childPIDs[numPIDs], -1, "error with fork");
			
			if(childPIDs[numPIDs] == 0) {
				// Set sigactions for child process
				SIGINT_action.sa_handler = SIG_DFL;
				sigaction(SIGINT, &SIGINT_action, NULL);
				
				SIGTSTP_action.sa_handler = SIG_IGN;
				sigfillset(&SIGINT_action.sa_mask);
				sigaction(SIGTSTP, &SIGTSTP_action, NULL);

				// If input needs to be rerouted, use dup2
				if(inputFile[0] != '\0') {
					inputFD = open(inputFile, O_RDONLY);
					PrintError(inputFD, -1, "cannot open file for input");
					result = dup2(inputFD, 0);
					PrintError(result, -1, "errot with input redirection");
				}
				
				// If output needs to be rerouted, use dup2
				if(outputFile[0] != '\0') {
					outputFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0622);
					PrintError(outputFD, -1, "cannot open file for output");
					result = dup2(outputFD, 1);	
					PrintError(result, -1, "error with output redirection");
				}
				
				// Use execvp for non-builtin commands. Pass in arg list.
				execvp(args[0], args);
				PrintError(-1, -1, "no such file, command, or directory");
			}
			// If background process, display the pid
			if (isBackground == 1) {
				printf("background pid is %d\n", childPIDs[numPIDs]);
				fflush(stdout);
				numPIDs++;	
			}
			// If foreground process, wait for completion, and then set the signal and exit status values
			else {
				int childExitMethod;
				waitpid(childPIDs[numPIDs], &childExitMethod, 0);
				
				if (WIFEXITED(childExitMethod)) {
					fgExitStat = WEXITSTATUS(childExitMethod);
					fgTermSig = -1;
				}
				if (WIFSIGNALED(childExitMethod)) {
					fgTermSig = WTERMSIG(childExitMethod);
					fgExitStat = -1;
					printf("%d", fgTermSig);
					fflush(stdout);
					PrintStatus(fgExitStat, fgTermSig);
				}
			}
		}
	}
	
	return 0;
}

