#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
typedef struct {
  uint8_t id;
  int32_t data;
  int16_t terminator;
} packet32unsigned;
int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: ./client <ip> <port>\n");
    exit(1);
  }
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server;
  server.sin_family = (short)AF_INET;
  server.sin_addr.s_addr = htonl(atol(argv[1]));
  server.sin_port = htons((short)atoi(argv[2]));
  int length = sizeof(server);
  int returnVal = connect(sock, (struct sockaddr *)&server, (socklen_t)length);
  if (returnVal < 0) {
    printf("Returned %d\n", returnVal);
    exit(1);
  } else {
    printf("Connected!\n");
    while (1) {
        packet32unsigned testpacket;
        testpacket.id = htons((uint8_t) 108);
        testpacket.data = htons((uint16_t) 69);
        testpacket.terminator = 0xBEEF;
        ssize_t writtenBytes = write(sock, &testpacket, sizeof(testpacket));
        printf("Wrote %ld bytes\n", writtenBytes);
        sleep(10);
    }
  }
  exit(0);
}