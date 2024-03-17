#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define EVENT_SIZE (sizeof(struct inotify_event)) //size of one event
#define MAX_EVENT_MONITOR 2048 //Maximum number of events that this script can watch for- I think? inotify documentation is pretty terrible so I would like to write a script to test this 
#define NAME_LEN 32 //filename length
#define BUFFER_LEN MAX_EVENT_MONITOR*(EVENT_SIZE+NAME_LEN) //buffer length
int isCSV(char* fileName);
void parseData(int fd);
void getFullPath(char* writeLoc, char* fileName, char* filePath);
char* addExtension(char* addOn, char* fileName);
void moveFile(char* oldPath, char* newPath);

char* dirToWatch = "/home/josh/C-DirectoryScan/aerospace-testing/watchDir";
int main(int argc, char* argv[]){
	// //This error checking will be useless is final version as all the final paths will be hard coded, we should not be changing directory names so this will take no arguments
	//  if(argc!=1){
	// 	printf("Invalid usage: ./main.c watchDir\n");
	// 	return -1;
	// }
	char* stashExtension = "/home/josh/C-DirectoryScan/aerospace-testing/ProcessedFiles";
	char* oddExtension = "/home/josh/C-DirectoryScan/aerospace-testing/OddBallFiles";
	char buffer[BUFFER_LEN];
	char* filePath = argv[1];
	int fd, wd;
	fd = inotify_init();
	if(fd<0){
		perror("Something broke\n");
		return -1;
	}
	
	wd = inotify_add_watch(fd, filePath, IN_CREATE | IN_MOVE);
	if(wd==-1){
		perror("Something else broke\n");
		return -1;
	}
	int i = 0;
	char* newFile = malloc(sizeof(char)*NAME_LEN);
	int curFD;
	char* oldFileName = malloc(sizeof(char)*NAME_LEN);
	while(1){
		memset(newFile, 0, NAME_LEN);
		i = 0;
		int readBytes = read(fd, &buffer, BUFFER_LEN);
		if(readBytes<0){
			perror("Read failed\n");
		}
		while(i<readBytes){
			struct inotify_event *event = (struct inotify_event*) &buffer[i];
			if(event->len){
				if(event->mask & IN_CREATE){
					sleep(2); //From my testing, I found that the move file logic can have a few multiple second delay sometimes, to account for that there is this sleep
					if(strcmp(oldFileName, event->name)!=0){
						//From my testing, I found that the move file logic can have a few multiple second delay sometimes, to account for that there is this sleep
						if(event->mask & IN_ISDIR){
							//printf("New directory %s created\n", event->name);
							//Do nothing as we aren't expecting any directories
						}else{
							printf("New file %s created\n", event->name);
							strcpy(newFile, event->name);
							strcpy(oldFileName, event->name);
							char* fullDirectory = malloc(strlen(newFile)+strlen(filePath)+2);
							getFullPath(fullDirectory, newFile, argv[1]);		
							if(isCSV(newFile)){
								curFD = open(fullDirectory, O_RDONLY); //We should not be modifying the data so this will be read only
								parseData(curFD);
								close(curFD);
								char* newLocation= malloc(strlen(stashExtension)+strlen(newFile)+2);
								char* newLoc = addExtension(stashExtension, newFile);
								moveFile(fullDirectory, newLoc);
								free(newLoc);
							}else{
							    perror("Incorrect file type inserted, moving to odd ball folder\n");
								char* newLoc = addExtension(oddExtension, newFile);
								moveFile(fullDirectory, newLoc);
								free(newLoc);
							}
							memset(fullDirectory, 0, strlen(newFile)+strlen(filePath)+2);
							free(fullDirectory);
						}
					}
				}else if(event->mask & IN_MOVE){
					sleep(2); //From my testing, I found that the move file logic can have a few multiple second delay sometimes, to account for that there is this sleep
					if(strcmp(oldFileName, event->name)!=0){
						if(event->mask & IN_ISDIR){
							//printf("New directory %s created\n", event->name);
							//Do nothing as we aren't expecting any directories
						}else{
							printf("File %s moved in\n", event->name);
							strcpy(newFile, event->name);
							strcpy(oldFileName, event->name);
							char* fullDirectory = malloc(strlen(newFile)+strlen(filePath)+2);
							getFullPath(fullDirectory, newFile, argv[1]);		
							if(isCSV(newFile)){
								curFD = open(fullDirectory, O_RDONLY); //We should not be modifying the data so this will be read only
								parseData(curFD);
								close(curFD);
								char* newLoc = addExtension(stashExtension, newFile);
								moveFile(fullDirectory, newLoc);
								free(newLoc);
							}else{
								perror("Incorrect file type inserted, moving to odd ball folder\n");
								char* newLoc = addExtension(oddExtension, newFile);
								moveFile(fullDirectory, newLoc);
								free(newLoc);
							}
							free(fullDirectory);
						}
					}
				}
				
				i+=EVENT_SIZE+event->len;
			}
		}
	}
	free(newFile);
	free(oldFileName);
	return 0;
}
char* addExtension(char* addOn, char* fileName){
	char* fullPath = malloc(strlen(addOn)+2 +strlen(fileName));
	//This memset is very important, the filepath seems to get corrupted after a few usages without this here
	memset(fullPath, 0, (strlen(addOn)+2+strlen(fileName)));
	strcat(fullPath, addOn);
	strcat(fullPath, "/");
	strcat(fullPath, fileName);
	return fullPath;
}
int isCSV(char* fileName){
	char* extension = strrchr(fileName, '.');
	return extension && strcmp(extension + 1, "csv") == 0;
}
void moveFile(char* oldPath, char* newPath){
	printf("Old file path is %s\n", oldPath);
	printf("New file path is %s\n", newPath);
	int response = rename(oldPath, newPath);
	if(response==-1){
		perror("File move failed\n");
	}
}
void parseData(int fd){

}
void getFullPath(char* writeLoc, char* fileName, char* filePath){
	memset(writeLoc, 0, strlen(writeLoc));
	strcat(writeLoc, filePath);
	strcat(writeLoc, "/");
	strcat(writeLoc, fileName);
}


/*
Moving Forward:
1. Need an example of the packed data so I can write parseData
2. Need to know what kind of file names we'll be getting, if there will be duplicate files rename will fail and eventually clutter our watched directory
	a. This can be handled by just renaming the files to a timestamp instead of their original name so its not a hard fix but its still worth looking into
3. Hardest part will be figuring out how to interface this to write over CAN to COSMOS
	a. https://www.kernel.org/doc/html/next/networking/can.html
	We shouldn't need any external CAN library, SocketCAN is built into the linux kernal and allows us to write over CAN via a socket.

How to parse CSV?
Probably easiest to use a csv C library that already exist
https://github.com/jandoczy/csv-fast-reader
Now I need to figure out how to convert parsed CSV into a format that COSMOS will accept, once I have that I can open a socket and use SocketCAn to write to COSMOS


QOL Features to look into:
	Add a log.txt file and convert all the printfs into writes that output to the log
	Add a timestamp on each action
	Add some sort of 'restart' function in the event of any failures to prevent this code from ever stalling
		Idea: Could fork into a child that actually does the watching while the parent just makes sure the process stays alive, if it ever crashes just have the parent restart it and continue watching

Testing Stuff:
	Tested with spamming new entries, it will have a bit of a delay processing everything due to the sleeps however it does process every file
	Write a script that will create at least 2048  files and move them into the watched directory to try to force a failure

*/

