// File: Harcoura.buildrooms.c
// Author: Adeline Harcourt (The skeleton for this code was obtained from
// Professor Benjamin Brewster, CS344 - Spring 2017)
// Description: The build rooms program for CS344 Program 2 - Spring 2017. This 
// program is intended to be run before the "Harcoura.adventure.c" program. It
// generates 7 room files in a directory that correspond to rooms in an adventure 
// game. Each room has a name, a list of random room connections, and a type (start,
// end or middle room).
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Global room arrays
char *roomNames[10] = {"Library", "Terrace", "Kitchen", "Dining Room", "Lounge", "Study", "Wine Cellar", "Billiard's Room", "Humidor", "Moon Garden"}; 	// Room names
int roomRelate[10][10] = {0};	   // Matrix of room connection relationships
int roomConnections[10] = {0};	   // Array containing the number of connections each room has

// A function that returns the number of rooms with "x" number of connections
// Accepts: An integer x representing the number of connections required of a room
// Returns: The number of rooms meeting that criteria
int GetNumConnectedRooms(int x) {
	int i; 		     // Iterator
	int roomCount = 0;   // Number of rooms connected
	
	for (i = 0; i < 10; i++) {
		if (roomConnections[i] > x) {
			roomCount++;
		}
	}
	return roomCount;
}

// A function that returns true if all rooms have 3 to 6 outbound connections, and
// returns false otherwise.
int IsGraphFull() {
	int i;		      // Iterator	
	
	//If seven rooms have at least 3 connections, return true
	if (GetNumConnectedRooms(3) >= 7 ) {
		return 1;
	}
	else {
		return 0;
	}
}

// A function that returns a random integer between 0 and 9, does not validate if 
// a connection can be added to this room.
int GetRandomRoom()
{
	return rand() % 10;
}

// A function that returns true if a connection can be added from room x, false otherwise
int CanAddConnectionFrom(int x) {
	int i;			// Iterator	
	
	//Check if room x already has 6 connections
	if (roomConnections[x] >= 6) {
		return 0;
	}
	else {
		return 1;
	}
}


// Connects rooms x and y together, does not check if this connection is valid
void ConnectRoom(int x, int y) {
	// Fill in relationships in matrix
	if (roomRelate[x][y] == 0) {
		roomRelate[x][y] = 1;
		roomRelate[y][x] = 1;
		
		// Increment connections array
		roomConnections[x]++;
		roomConnections[y]++;
	}
}

// Returns true if Rooms x and y are the same Room, false otherwise
int IsSameRoom(int x, int y) {
	return x == y;
}

// A function that returns a room with at least one connection
int GetConnectedRoom() {
	int room;
	
	while (1) {
		room = GetRandomRoom();	
		if (roomConnections[room] > 0) {
			return room;
		}
	}
}

// A function that adds a random, valid outbound connection from a room to 
// another room.
void AddRandomConnection()  
{
	int A;  // Room A
 	int B;	// Room B

	// Keep getting random rooms until one is not full of connections and does not validate the 7
	// connected rooms rule
  	do {
   		A = GetRandomRoom();
  	} while(CanAddConnectionFrom(A) == 0 || (GetNumConnectedRooms(0) >= 7 && roomConnections[A] == 0));

	// Keep getting random rooms until one is not full of connections, does not validate the 7
	// connected rooms rule, and is not equal to room A.
  	do {
    		B = GetRandomRoom();
  	} while((CanAddConnectionFrom(B) == 0 || IsSameRoom(A, B) == 1) || (GetNumConnectedRooms(0) >= 6 && roomConnections[B] == 0));

 	ConnectRoom(A, B);
}

int main(){
	int i,j;			// Iterators
	int connectNum;			// Int to keep track of number of room connections
	int startRoom;			// Index of starting room
	int endRoom;			// Index of ending room
	char directory[100];		// Name of directory to store rooms
	memset(directory, '\0', 100); 	
	char currFile[100];		// Name of current room file
	memset(currFile, '\0', 100);
	pid_t pid = getpid();		// Current process ID	
		
	srand(time(NULL));

	// Create all connections in graph
	while (IsGraphFull() == 0)
	{
		AddRandomConnection();
	}
	
	// Create directory from PID
	snprintf(directory, sizeof(directory), "Harcoura.rooms.%d", (int)pid);
	mkdir(directory, 0777);
	
	// Set start room and end room values
	startRoom = GetConnectedRoom();
	endRoom = startRoom;	
	while (startRoom == endRoom) { 
		endRoom = GetConnectedRoom();
	}

	// Loop through all rooms and create a room file if they are connected
	for (i = 0; i < 10; i++) {
		if (roomConnections[i] != 0) {
			FILE *roomFile = NULL;
			
			// Create file name string
			snprintf(currFile, sizeof(currFile), "./%s/Room%d.txt", directory, i);
			roomFile = fopen(currFile, "a");
			
			// Add room name
			fprintf(roomFile, "ROOM NAME: %s\n", roomNames[i]);
	
			// Add room connections
			connectNum = 0;		
			for (j = 0; j < 10; j++) {
				if (roomRelate[i][j] == 1) {
					fprintf(roomFile, "CONNECTION %d: %s\n", ++connectNum, roomNames[j]);	
				}
			}
			
			// Add room type
			fprintf(roomFile, "ROOM TYPE: ");
			if (i == startRoom) {
				fprintf(roomFile, "START_ROOM\n");
			}
			else if (i == endRoom) {
				fprintf(roomFile, "END_ROOM\n");
			}
			else {
				fprintf(roomFile, "MID_ROOM\n");
			}
			
			fclose(roomFile);
		}
	}
	return 0;
}
