/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd;
    char buf[BUFSIZE];
    struct addrinfo hints, *servinfo, *p;
    size_t n;
    socklen_t servlen;
    int status;
    char cmd[10];

    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
      return 1;
    }


    for (p = servinfo; p != NULL; p = p->ai_next)
    {
      if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      {
        perror("error making socket\n");
        continue;
      }
      break;
    }

    if (p == NULL)
    {
      perror("failed to create to socket\n");
      return 2;
    }

    servlen = sizeof(struct sockaddr_storage);
    while (strcmp(cmd, "exit") != 0)
    {
      bzero(buf, BUFSIZE);
      printf("Please enter msg: ");
      fgets(buf, BUFSIZE-1, stdin); // this includes \n character adding +2 to strlen() output
      sscanf(buf, "%s", cmd);

      if((n = sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen)) <= 0)
      {
        error("ERROR in sendto");
      }
      
      if((n = recvfrom(sockfd, buf, BUFSIZE, 0, p->ai_addr, &servlen)) <= 0)
      {
        error("ERROR in recvfrom");
      }
      printf("Echo from server: %s", buf);
    }
    return 0;
}