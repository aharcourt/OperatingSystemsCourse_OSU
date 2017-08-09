// File: Harcoura.adventure.c
// Author: Adeline Harcourt
// Description: The adventure game program for CS344 Program 2 - Spring 2017. This 
// program must be run after the "Harcoura.buildrooms.c" program. It reads in the 
// room files created from the buildrooms program, and then simulates an adventure
// game using their contents. The program also has a time function that utilizes 
// threads and mutexes.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>

// Global mutex variable
pthread_mutex_t mutexID = PTHREAD_MUTEX_INITIALIZER;

// Struct to hold room information from each file
struct Room {
	char name[50];		 // Name of Room
	int numConnects;	 // Number of connected rooms
	char *connected[6];  	 // Names of connected rooms
	char type[20];		 // Type of Room
};

// A function that displays the current room and choices for moves.
// Accepts: the Room struct corresponding to the current location
void GamePrompt(struct Room currRoom) {
	int i;		// Iterator

	printf("CURRENT LOCATION: %s\n", currRoom.name);
	printf("POSSIBLE CONNECTIONS:");
	for (i = 0; i < currRoom.numConnects; i++) {
		printf(" %s", currRoom.connected[i]);
		if (i + 1 != currRoom.numConnects) {	// If not last room, print comma
			printf(",");
		}
		else {					// Otherwise, print period and prompt
			printf(".\nWHERE TO?  >");
		}
	}
}
	
// A function that accepts user input and determines if it is valid.
// Valid entries include a connected room name or the phrase "time".
// Accepts: the Room struct corresponding to the current location.
// Returns: the valid input.
char *ValRoomInput(struct Room currRoom) {
	int i;				// Iterator
	size_t inputSize = 100;		// Size of user input
	char *inputText = NULL;		// String to hold user input
	
	while (1) {
		getline(&inputText, &inputSize, stdin);
		printf("\n\n");
		inputText[strcspn(inputText, "\r\n")] = 0; 		// Strip EOL characters
	
		if (strcmp(inputText, "time") == 0) {			// Check if user entered "time"
			return "time";
		}
																								
		for(i = 0; i < currRoom.numConnects; i++) {		// Check if user entered valid room
			if (strcmp(inputText, currRoom.connected[i]) == 0) {
				free(inputText);			// Free getline memory
				return currRoom.connected[i];				
			}
		}
		printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n\n");
		GamePrompt(currRoom);
	}
}
	
// A function that is utilized by the second program thread in order
// to write the current time to a file. A mutex is used to regulate
// the thread actions.
void* WriteTime(void *argument)
{
	pthread_mutex_lock(&mutexID);			// Lock the mutex
 	char timeString[200];				// String to hold formatted time
	memset(timeString, '\0', sizeof(timeString));	
   	time_t timeNow;					// time_t variable to hold current time
   	struct tm *timeStruct;				// Time struct to break down time
	FILE *timeFile; 				// File to output time into
	
	// Format time
   	timeNow = time(NULL);		
   	timeStruct = localtime(&timeNow);
   	strftime(timeString, sizeof(timeString), "%l:%M%P, %A, %B %e, %Y", timeStruct);	
	
	// Write time to file
	timeFile = fopen("currentTime.txt", "w");
    fwrite(timeString, sizeof(char), strlen(timeString), timeFile);		
	fclose(timeFile);
  
    // Unlock mutex
    pthread_mutex_unlock(&mutexID);	
   	return NULL;
}

// A function that finds the most current directory that starts with 
// "Harcoura.rooms.". This is used to locate the most recent run of the
// buildrooms program.
// CODE SOURCE: Professor Benjamin Brewster, CS344 Lecture 2.4
void GetNewestRoomDir(char *newestDirName) {
	int newestDirTime = -1; 
	char targetDirPrefix[32] = "Harcoura.rooms."; 

	DIR* dirToCheck; 
	struct dirent *fileInDir; 
	struct stat dirAttributes; 

	dirToCheck = opendir("."); 
	
	if (dirToCheck > 0) 
	{
		while ((fileInDir = readdir(dirToCheck)) != NULL) 
		{
			if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) 
			{
				stat(fileInDir->d_name, &dirAttributes); 

				if ((int)dirAttributes.st_mtime > newestDirTime) 
				{
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDirName, '\0', 256);
					strcpy(newestDirName, fileInDir->d_name);
				}
			}
		}
	}
	closedir(dirToCheck); 
}

int main(){
	int i,j;			// Iterators
	int fileSize;			// Size of each input file
	int startRoom;			// Index of room with type START_ROOM
	int steps = 0;			// Number of steps required to win
	int currRoom = 0;		// Index of current room location
	int gameOver = 0;		// 1 = Game Won, 0 = Game still going
	char newestDirName[256];	// Name of the newest rooms directory
	memset(newestDirName, '\0', sizeof(newestDirName));
	char fileName[256];		// Name of the current Room file
	memset(fileName, '\0', sizeof(fileName));
	char fileContents[500];		// Contents of current Room file
	memset(fileContents, '\0', sizeof(fileContents));
	char *stepList[300];		// List of steps taken to win game
	char *currStep;			// Current input from user
	DIR* roomDir;			// Most recent room Directory
	struct dirent *roomItem; 	// Current file being looked at in directory
	struct Room roomArr[7];		// Array of rooms generated by room files	
		
	// Lock mutex and create thread 2
	pthread_mutex_lock(&mutexID);
	pthread_t threadID;
	pthread_create(&threadID, NULL, WriteTime, NULL);
	
	// Get the newest room directory
	GetNewestRoomDir(newestDirName);
	roomDir = opendir(newestDirName);

	
	// Loop through all room files 
	while ((roomItem = readdir(roomDir)) != NULL) 
	{
		if (strstr(roomItem->d_name, "Room") != NULL) {
			// Open next room file
			memset(fileName, '\0', sizeof(fileName));
			memset(fileContents, '\0', sizeof(fileContents));				
			snprintf(fileName, sizeof(fileName), "%s/%s", newestDirName, roomItem->d_name);	//Construct file name
			FILE *roomFile = fopen(fileName, "r");

			// Find size of file
			fseek(roomFile, 0L, SEEK_END);
			fileSize = ftell(roomFile);
			fseek(roomFile, 0L, SEEK_SET);
		
			// Read in file to string
			fread(fileContents, sizeof(char), fileSize, roomFile);
			fclose(roomFile);
			
			// Use strtok to inport file contents into a struct
			char *token = strtok(fileContents, ":\n");
			strcpy(roomArr[currRoom].name, strtok(NULL, ":\n") + 1);
			roomArr[currRoom].numConnects = 0;
			
			while (strstr(strtok(NULL, ":\n"), "CONNECTION") != NULL) {
				roomArr[currRoom].connected[roomArr[currRoom].numConnects] = malloc(50);
				strcpy(roomArr[currRoom].connected[roomArr[currRoom].numConnects], strtok(NULL, ":\n") + 1);
				roomArr[currRoom].numConnects++;
			}
			strcpy(roomArr[currRoom].type, strtok(NULL, ":\n") + 1);

			// If room is the start room, initialize startRoom.
			if (strcmp(roomArr[currRoom].type, "START_ROOM") == 0) {
				startRoom = currRoom;
			}
			currRoom++;	// Increment to next room file
		}
	}
	closedir(roomDir); // Close the directory we opened

	// Set the current room to the start room
	currRoom = startRoom;
	printf("\n");
	
	// Keep prompting user until the game is won
	do {		
		GamePrompt(roomArr[currRoom]);		    // Prompt user
		currStep = ValRoomInput(roomArr[currRoom]); //Get user input
		
		// User wants time
		if (strcmp(currStep, "time") == 0) {  
			pthread_mutex_unlock(&mutexID);     // Start thread 2
			pthread_join(threadID, NULL); 	    // Wait until thread 2 is done
			pthread_mutex_lock(&mutexID);	    // Relock mutex
			pthread_create(&threadID, NULL, WriteTime, NULL);

			// Open time file and read contents
			FILE *timeFile = fopen("currentTime.txt", "r");
			if (timeFile) {
				char c;
				while ((c = getc(timeFile)) != EOF)
					printf("%c", c);
				printf("\n\n\n");
				fclose(timeFile);
			}
		}
		
		// Find room user wants to enter and add it to steps
		for (i = 0; i < 8; i++) {
			if (strcmp(currStep, roomArr[i].name) == 0) {
				currRoom = i; 
				stepList[steps] = roomArr[i].name;
				steps++;
			}
		}
		
		// If the user won the game, display victory message and steps
		if (strcmp(roomArr[currRoom].type, "END_ROOM") == 0) {
			printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
			printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
			for (i = 0; i < steps; i++) {
				printf("%s\n", stepList[i]);
			}
			printf("\n");
			gameOver = 1;
		}
	} while (gameOver == 0); 

	// Free memory
	for (i = 0; i < 7; i++) {
		for (j = 0; j < roomArr[i].numConnects; j++) {
			free(roomArr[i].connected[j]);
		}
	}
	return 0;
}
