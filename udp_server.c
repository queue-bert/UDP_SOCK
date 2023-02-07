/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "queue.h"
#include "util.h"

#define BUFSIZE 512
#define HEADER 3
#define PACKET BUFSIZE + HEADER

void error(char *msg) {
  perror(msg);
  exit(1);
}

void *get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  else
  {
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv) {
  int sockfd;
  socklen_t clientlen;
  struct sockaddr_storage their_addr;
  char buf[BUFSIZE];
  char host[NI_MAXHOST];
  char service[NI_MAXSERV];
  int optval;
  size_t n;
  struct addrinfo hints, *res, *p;
  int status; // return status of getaddrinfo()
  int num_assign; // number of successful assignments in sscanf()

  // storing user commands from the client
  char cmd[10];
  char file[256]; // max filepath size according to IBM


  // file manipulation and reading
  FILE *fp;
  struct stat st;
  off_t num_bytes;
  
  // handling packetization
  char packet[PACKET+1];
  char header[HEADER];
  char packet_data[BUFSIZE];
  //Queue* intake = createQueue(); 
  

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;


  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  if((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
  }

  for (p = res; p != NULL; p = p->ai_next)
  {
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("listener: socket");
      continue;
    }

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("listener: bind");
      continue;
    }
    break;
  }

  clientlen = sizeof(their_addr);
  for (;;) {
    bzero(buf, BUFSIZE);
    if((n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &their_addr, &clientlen)) <= 0)
    {
      error("ERROR in recvfrom");
    }
      

    if(getnameinfo((struct sockaddr *) &their_addr, sizeof(their_addr), host, NI_MAXHOST, service, NI_MAXSERV, NI_NAMEREQD) == 0)
    {
      printf("server received datagram from %s (%s)\n", host, service);
      printf("server received %zu/%zu bytes: %s\n", strlen(buf), n, buf);
    }

    num_assign = sscanf(buf, "%s %s", cmd, file);

    if(strcmp(cmd, "get") == 0)
    {
      if (num_assign != 2)
      {
        if ((n = sendto(sockfd, "ENTER VALID FILENAME\n", BUFSIZE, 0, (struct sockaddr *) &their_addr, clientlen)) <= 0) 
          error("ERROR in sendto");
        continue;
      }

      // get the size of the file requested by the user
      if(stat(file, &st) == 0)
      {
        num_bytes = (off_t)st.st_size;
      }
      else
      {
        perror("error determining the filesize");
        return 1;
      }

      printf("FILE SIZE: %ld\n", num_bytes);
      // not sure if i have to send a size packet because the end of my loop could just send
      // a completion byte in the last packet and the packet size
      // [mode][size][data] = 1 + 3 + 512


      fp = fopen(file, "r");

      if (fp == NULL)
      {
        perror("fopen failed");
        return 1;
      }

      while (num_bytes > 0)
      {
        // reset buffers for packet-population
        memset(header, '?', HEADER);
        bzero(packet_data, BUFSIZE);

        // initializing packet datum
        size_t numb_read = fread(buf, sizeof(char), BUFSIZE, fp); // will read past BUFSIZE a little so handle with %.*s width argument
        uint16_t ns_length = htons((uint16_t) numb_read);
        char* mode;
        int packet_data_written = sprintf(packet_data, "%.*s", (int)numb_read, buf);

        if(numb_read < BUFSIZE)
        {
          // stuffing exit packet ;)
          mode = "X";
          memcpy(header, mode, 1);
          memcpy(header + 1, &ns_length, sizeof(ns_length));
          memcpy(packet, header, HEADER);
          memcpy(packet + HEADER, packet_data, packet_data_written);
          packet[packet_data_written+HEADER] = '\0';
          printf("%s", packet+HEADER);

          int packet_sz = HEADER + packet_data_written;
          if (sendall(sockfd, packet, &packet_sz, (struct sockaddr *) &their_addr) == -1)
          {
            printf("ONLY SENT %d BYTES DUE TO ERROR\n", packet_sz);
            error("sendall");
          }
          
          break;
        }

        // stuffing packets ;)
        mode = "O";
        memcpy(header, mode, 1);
        memcpy(header + 1, &ns_length, sizeof(ns_length));
        memcpy(packet, header, HEADER);
        memcpy(packet + HEADER, packet_data, packet_data_written);
        packet[PACKET] = '\0';
        printf("%s", packet+HEADER);

        int packet_sz = PACKET;
        if (sendall(sockfd, packet, &packet_sz, (struct sockaddr *) &their_addr) == -1)
        {
          printf("ONLY SENT %d BYTES DUE TO ERROR\n", packet_sz);
          error("sendall");
        } 

        num_bytes -= numb_read;
      }   
    }
    else if(strcmp(cmd, "put") == 0)
    {
      n = sendto(sockfd, "RECEIVED PUT\n", strlen(buf), 0, (struct sockaddr *) &their_addr, clientlen);
      if (n <= 0) 
        error("ERROR in sendto");
    }
    else if(strcmp(cmd, "delete") == 0)
    {
      // DELETE IS DONE!!!
      if (num_assign != 2)
      {
        if ((n = sendto(sockfd, "ENTER VALID FILENAME TO DELETE\n", BUFSIZE, 0, (struct sockaddr *) &their_addr, clientlen)) <= 0) 
          error("ERROR in sendto");
        continue;
      }

      int rm_status = remove(file);
      if(rm_status == 0)
      {
        if ((n = sendto(sockfd, "FILE DELETED\n", BUFSIZE, 0, (struct sockaddr *) &their_addr, clientlen)) <= 0) 
          error("ERROR in sendto");
        continue;
      }
      else
      {
        error("Error deleting file");
      }

    }
    else if(strcmp(cmd, "ls") == 0)
    {
      fp = popen("ls", "r");
      if(fp == NULL)
      {
        error("Unable to get list of current directory files");
      }

      //while(fgets())
      n = sendto(sockfd, "RECEIVED LIST\n", strlen(buf), 0, (struct sockaddr *) &their_addr, clientlen);
      if (n <= 0) 
        error("ERROR in sendto");
    }
    else if(strcmp(cmd, "exit") == 0)
    {
      n = sendto(sockfd, "RECEIVED EXIT\n", strlen(buf), 0, (struct sockaddr *) &their_addr, clientlen);
      if (n <= 0) 
        error("ERROR in sendto");
    }
    else
    {
      n = sendto(sockfd, "INVALID INSTRUCTION\n", strlen(buf), 0, (struct sockaddr *) &their_addr, clientlen);
      if (n <= 0) 
        error("ERROR in sendto");
    }
  }
}
