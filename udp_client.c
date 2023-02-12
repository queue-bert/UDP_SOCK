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
#include "queue.h"

#define BUFSIZE 1024
#define HEADER 3 // make sure this is the same between files
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
    char filename[256];

    // handling packets
    char header[HEADER];
    Queue* intake = createQueue(); 

    // downloading by opening a new file to write to
    FILE* fp;

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

    // maybe i could block ctrl+d or just any other keyboard input or it might just be an OS command
    // in which case i would have to probably deal with signals
    servlen = sizeof(struct sockaddr_storage);

    while (strcmp(cmd, "exit") != 0)
    {
      bzero(buf, BUFSIZE);
      printf("Please enter msg: ");
      fgets(buf, BUFSIZE-1, stdin); // this includes \n character adding +2 to strlen() output
      int cmd_bytes_written = sscanf(buf, "%s %s", cmd, filename);
      printf("THIS IS YOUR COMMAND %s of length %d\n", cmd, cmd_bytes_written);

      // send the command to server
      if((n = sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen)) <= 0)
      {
        error("ERROR in sendto");
      }

      // my code doesn't handle if a file on the server exists or not
      if(strcmp(cmd,"get") == 0)
      {
        int complete = 1;
        int cmp = 0;
        uint16_t bytes_left = 0;
        char* in_packet;
        char* ex_mode = "X";
        uint16_t ns_length;
        if ((fp = fopen("test_udp.txt", "w")) == NULL)
        {
          error("Could not open file for writing...\n");
        }

        // if needed i should set a timeout, this is a blocking call and might not unblock
        // until it receives something which could take forever
        if((n = recvfrom(sockfd, buf, BUFSIZE, 0, p->ai_addr, &servlen)) <= 0)
          error("ERROR, COULD NOT RECEIVE FILE FROM SERVER");
        memcpy(header, buf, HEADER);
        memcpy(&ns_length, header+1, sizeof(ns_length));
        ns_length = ntohs(ns_length);

        if((in_packet = (char*)malloc(ns_length * sizeof(char))) == NULL )
        {
          error("malloc failed to allocate memory");
        }
        enqueue(intake, in_packet);
        memcpy(in_packet, buf, HEADER);
        // is it possible that n could be greater than ns_length so we'd read in something bigger?
        memcpy(in_packet, buf+HEADER, n-HEADER); // adding bytes we've received to packet buffer
        
        if((uint16_t)n < ns_length)
        {
          complete = 0;
          bytes_left = ns_length - (uint16_t)n;
        }
        else
        {
          char* free_packet = dequeue(intake);
          //printf("%s", free_packet+HEADER);
          fwrite(free_packet+HEADER, sizeof(char), ns_length-HEADER, fp);
          free(free_packet);
        }

        while((cmp = strncmp(header, ex_mode , sizeof(char))) != 0 || !complete) // must use single quotes for char literals
        {
          bzero(buf, BUFSIZE);
          if((n = recvfrom(sockfd, buf, BUFSIZE, 0, p->ai_addr, &servlen)) <= 0)
            error("ERROR, COULD NOT RECEIVE PACKET FROM SERVER");


          if(!complete)
          {
            if (n > bytes_left) // edge case :: only two bytes are left after subtracting what was missing
            {
              memcpy(in_packet+(ns_length-bytes_left), buf, bytes_left); // might need to add '\0'
              char* free_packet = dequeue(intake);
              // write bytes to file 'fp'
              fwrite(free_packet+HEADER, sizeof(char), ns_length-HEADER, fp);
              //printf("NOT COMPLETE: %s", free_packet+HEADER);
              free(free_packet);
              complete = 1;
            }
            else
            {
              memcpy(in_packet+(ns_length-bytes_left), buf, n);
              if((bytes_left -= n) == 0)
              {
                char* free_packet = dequeue(intake);
                // write bytes to file 'fp'
                fwrite(free_packet+HEADER, sizeof(char), ns_length-HEADER, fp);
                //printf("NOT COMPLETE: %s", free_packet+HEADER);
                free(free_packet);
                complete = 1;
              }
              continue; // get out what we can from the buffer but continue trying to get all the bytes
            }
          }
          memcpy(header, buf+bytes_left, HEADER);
          memcpy(&ns_length, header+1, sizeof(ns_length));
          ns_length = ntohs(ns_length);

          if((in_packet = (char*)malloc(ns_length * sizeof(char))) == NULL)
          {
            error("malloc failed to allocate memory");
          }

          enqueue(intake, in_packet);
          memcpy(in_packet, buf+bytes_left+HEADER, n-HEADER-bytes_left); // adding bytes we've received to packet buffer
          memcpy(in_packet, buf+bytes_left, n-bytes_left);

          if((uint16_t)n-bytes_left < ns_length)
          {
            complete = 0;
            bytes_left = ns_length - (uint16_t)n-bytes_left;
            continue;
          }
          else
          {
            char* free_packet = dequeue(intake);
            fwrite(free_packet+HEADER, sizeof(char), ns_length-HEADER, fp);
            //printf("%s", free_packet+HEADER);
            free(free_packet);
          }

          continue;

        }

        fclose(fp);


      }

    }
    return 0;
}
