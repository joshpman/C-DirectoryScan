#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define __BYTE_ORDER __BIG_ENDIAN__
#define htonll(x)                                                              \
  ((1 == htonl(1))                                                             \
       ? (x)                                                                   \
       : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define hexterminator 0xBEEF;
#define EVENT_SIZE (sizeof(struct inotify_event)) // size of one event
#define MAX_EVENT_MONITOR                                                      \
  2048 // Maximum number of events that this script can watch for- I think?
       // inotify documentation is pretty terrible so I would like to write a
       // script to test this
#define NAME_LEN 32                                           // filename length
#define BUFFER_LEN MAX_EVENT_MONITOR *(EVENT_SIZE + NAME_LEN) // buffer length
int isCSV(char *fileName);
void parseData(int fd);
void getFullPath(char *writeLoc, char *fileName, char *filePath);
char *addExtension(char *addOn, char *fileName);
void moveFile(char *oldPath, char *newPath);
void encodeAndSend(int dataSize, void *data, uint8_t header, int signature);
int socketFD;
typedef struct {
  uint8_t id;
  uint8_t data;
  int16_t terminator;
} packet8;
typedef struct {
  uint8_t id;
  uint16_t data;
  int16_t terminator;
} packet16;
typedef struct {
  uint8_t id;
  uint32_t data;
  int16_t terminator;
} packet32;
typedef struct {
  uint8_t id;
  uint64_t data;
  int16_t terminator;
} packet64;
typedef struct {
  uint8_t id;
  int8_t data;
  int16_t terminator;
} packet8unsigned;
typedef struct {
  uint8_t id;
  int16_t data;
  int16_t terminator;
} packet16unsigned;
typedef struct {
  uint8_t id;
  int32_t data;
  int16_t terminator;
} packet32unsigned;
typedef struct {
  uint8_t id;
  int64_t data;
  int16_t terminator;
} packet64unsigned;
char *dirToWatch = "/home/josh/C-DirectoryScan/aerospace-testing/watchDir";

int main(int argc, char *argv[]) {
  // //This error checking will be useless is final version as all the final
  // paths will be hard coded, we should not be changing directory names so this
  // will take no arguments
  //  if(argc!=1){
  // 	printf("Invalid usage: ./main.c watchDir\n");
  // 	return -1;
  // }
  char *stashExtension =
      "/home/josh/C-DirectoryScan/aerospace-testing/ProcessedFiles";
  char *oddExtension =
      "/home/josh/C-DirectoryScan/aerospace-testing/OddBallFiles";
  char buffer[BUFFER_LEN];
  char *filePath = argv[1];
  int fd, wd;
  fd = inotify_init();
  if (fd < 0) {
    perror("Something broke\n");
    return -1;
  }

  wd = inotify_add_watch(fd, filePath, IN_CREATE | IN_MOVE);
  if (wd == -1) {
    perror("Something else broke\n");
    return -1;
  }
  int i = 0;
  char *newFile = malloc(sizeof(char) * NAME_LEN);
  int curFD;
  char *oldFileName = malloc(sizeof(char) * NAME_LEN);
  while (1) {
    memset(newFile, 0, NAME_LEN);
    i = 0;
    int readBytes = read(fd, &buffer, BUFFER_LEN);
    if (readBytes < 0) {
      perror("Read failed\n");
    }
    while (i < readBytes) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len) {
        if (event->mask & IN_CREATE) {
          sleep(2); // From my testing, I found that the move file logic can
                    // have a few multiple second delay sometimes, to account
                    // for that there is this sleep
          if (strcmp(oldFileName, event->name) != 0) {
            // From my testing, I found that the move file logic can have a few
            // multiple second delay sometimes, to account for that there is
            // this sleep
            if (event->mask & IN_ISDIR) {
              // printf("New directory %s created\n", event->name);
              // Do nothing as we aren't expecting any directories
            } else {
              printf("New file %s created\n", event->name);
              strcpy(newFile, event->name);
              strcpy(oldFileName, event->name);
              char *fullDirectory =
                  malloc(strlen(newFile) + strlen(filePath) + 2);
              getFullPath(fullDirectory, newFile, argv[1]);
              if (isCSV(newFile)) {
                curFD = open(fullDirectory,
                             O_RDONLY); // We should not be modifying the data
                                        // so this will be read only
                parseData(curFD);
                close(curFD);
                char *newLocation =
                    malloc(strlen(stashExtension) + strlen(newFile) + 2);
                char *newLoc = addExtension(stashExtension, newFile);
                moveFile(fullDirectory, newLoc);
                free(newLoc);
              } else {
                perror("Incorrect file type inserted, moving to odd ball "
                       "folder\n");
                char *newLoc = addExtension(oddExtension, newFile);
                moveFile(fullDirectory, newLoc);
                free(newLoc);
              }
              memset(fullDirectory, 0, strlen(newFile) + strlen(filePath) + 2);
              free(fullDirectory);
            }
          }
        } else if (event->mask & IN_MOVE) {
          sleep(2); // From my testing, I found that the move file logic can
                    // have a few multiple second delay sometimes, to account
                    // for that there is this sleep
          if (strcmp(oldFileName, event->name) != 0) {
            if (event->mask & IN_ISDIR) {
              // printf("New directory %s created\n", event->name);
              // Do nothing as we aren't expecting any directories
            } else {
              printf("File %s moved in\n", event->name);
              strcpy(newFile, event->name);
              strcpy(oldFileName, event->name);
              char *fullDirectory =
                  malloc(strlen(newFile) + strlen(filePath) + 2);
              getFullPath(fullDirectory, newFile, argv[1]);
              if (isCSV(newFile)) {
                curFD = open(fullDirectory,
                             O_RDONLY); // We should not be modifying the data
                                        // so this will be read only
                parseData(curFD);
                close(curFD);
                char *newLoc = addExtension(stashExtension, newFile);
                moveFile(fullDirectory, newLoc);
                free(newLoc);
              } else {
                perror("Incorrect file type inserted, moving to odd ball "
                       "folder\n");
                char *newLoc = addExtension(oddExtension, newFile);
                moveFile(fullDirectory, newLoc);
                free(newLoc);
              }
              free(fullDirectory);
            }
          }
        }

        i += EVENT_SIZE + event->len;
      }
    }
  }
  free(newFile);
  free(oldFileName);
  return 0;
}
char *addExtension(char *addOn, char *fileName) {
  char *fullPath = malloc(strlen(addOn) + 2 + strlen(fileName));
  // This memset is very important, the filepath seems to get corrupted after a
  // few usages without this here
  memset(fullPath, 0, (strlen(addOn) + 2 + strlen(fileName)));
  strcat(fullPath, addOn);
  strcat(fullPath, "/");
  strcat(fullPath, fileName);
  return fullPath;
}
int isCSV(char *fileName) {
  char *extension = strrchr(fileName, '.');
  return extension && strcmp(extension + 1, "csv") == 0;
}
void moveFile(char *oldPath, char *newPath) {
  printf("Old file path is %s\n", oldPath);
  printf("New file path is %s\n", newPath);
  int response = rename(oldPath, newPath);
  if (response == -1) {
    perror("File move failed\n");
  }
}
void getFullPath(char *writeLoc, char *fileName, char *filePath) {
  memset(writeLoc, 0, strlen(writeLoc));
  strcat(writeLoc, filePath);
  strcat(writeLoc, "/");
  strcat(writeLoc, fileName);
}
void parseData(int fd) {
  char dataArr[500];
  int i = 0;
  while (read(fd, &dataArr[i++], 1) > 0)
    ; // Reading data byte-by-byte
  encodeAndSend(16, (void *)(((uint8_t)dataArr[2] << 8) | (uint8_t)dataArr[3]),
                (uint8_t)123, 0);

  for (int i = 4; i < 15; i++) {
    encodeAndSend(8, (void *)((uint8_t)dataArr[i]), (uint8_t)125, 0);
  }
  encodeAndSend(16,
                (void *)(((uint8_t)dataArr[16] << 8) | (uint8_t)dataArr[17]),
                (uint8_t)100, 0);

  encodeAndSend(16,
                (void *)(((uint8_t)dataArr[18] << 8) | (uint8_t)dataArr[19]),
                (uint8_t)101, 0);
  for (int i = 20; i < 99; i += 8) {
    uint64_t value;
    for (int j = i; j < j + 7; j++) {
      value = (value << 8) | dataArr[j];
    }
    encodeAndSend(64, (void *)(value), 113, 0);
  }
  for (int i = 116; i < 139; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 114,
        0);
  }
  for (int i = 140; i < 163; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 115,
        0);
  }
  for (int i = 163; i < 187; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 116,
        0);
  }
  for (int i = 188; i < 211; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 117,
        0);
  }
  for (int i = 212; i < 235; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 118,
        0);
  }
  for (int i = 236; i < 259; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 120,
        0);
  }
  for (int i = 260; i < 283; i += 2) {
    encodeAndSend(
        16, (void *)(((uint8_t)dataArr[i] << 8) | (uint8_t)dataArr[i + 1]), 121,
        0);
  }
  i = 284;
  encodeAndSend(16, (void *)(((int8_t)dataArr[i] << 8) | (int8_t)dataArr[++i]),
                106, 1);
  for (int i = 286; i < 291; i++) {
    encodeAndSend(8, (void *)((uint8_t)dataArr[i]), 111, 0);
  }

}
// 0 for unsigned 1 for signed
void encodeAndSend(int dataSize, void *data, uint8_t header, int signature) {
  switch (dataSize) {
  case 8:
    if (signature == 0) {
      packet8 packet;
      packet.id = htons(header);
      packet.data = htons(*(uint8_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    } else {
      packet8unsigned packet;
      packet.id = htons(header);
      packet.data = htons(*(int8_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    }

    break;
  case 16:
    if (signature == 0) {
      packet16 packet;
      packet.id = htons(header);
      packet.data = htons(*(uint16_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    } else {
      packet16unsigned packet;
      packet.id = htons(header);
      packet.data = htons(*(int16_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    }

  case 32:
    if (signature == 0) {
      packet32 packet;
      packet.id = htons(header);
      packet.data = htonl(*(uint32_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    } else {
      packet32unsigned packet;
      packet.id = htons(header);
      packet.data = htonl(*(int32_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    }
    break;
  case 64:
    if (signature == 0) {
      packet64 packet;
      packet.id = htons(header);
      packet.data = htonll(*(uint8_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    } else {
      packet8unsigned packet;
      packet.id = htons(header);
      packet.data = htonll(*(int8_t *)(data));
      write(socketFD, &packet, sizeof(packet));
    }
    break;
  default:
    return;
  }
}

