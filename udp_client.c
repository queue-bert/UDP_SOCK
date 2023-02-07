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

    // i could make the while loop stop until it receives a confirmation that it's done sending
    // before prompting the user for a new command and then i could handle the ctrl+d functionality
    // gracefully and then also just be able to download an entire file before letting a user type in
    // a new command
    // maybe i could block ctrl+d or just any other keyboard input or it might just be an OS command
    // in which case i would have to probably deal with signals
    servlen = sizeof(struct sockaddr_storage);

    // make the logic to block on this loop until the "X" mode is received
    //
    while (strcmp(cmd, "exit") != 0)
    {
      bzero(buf, BUFSIZE);
      printf("Please enter msg: ");
      fgets(buf, BUFSIZE-1, stdin); // this includes \n character adding +2 to strlen() output
      int cmd_bytes_written = sscanf(buf, "%s", cmd);
      printf("THIS IS YOUR COMMAND %s of length %d\n", cmd, cmd_bytes_written);

      // send the command to server
      if((n = sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen)) <= 0)
      {
        error("ERROR in sendto");
      }
//   char* item1 = (char*)malloc(ARRAY_SIZE * sizeof(char));

//   char* dequeuedItem1 = dequeue(queue);

      // if we sent get <filename> run loop to get all packets
      if(strcmp(cmd,"get") == 0)
      {
        int complete = 1;
        int cmp;
        uint16_t bytes_left;
        char* in_packet;
        char* ex_mode = "X";
        uint16_t ns_length;

        // if needed i should set a timeout, this is a blocking call and might not unblock
        // until it receives something which could take forever
        if((n = recvfrom(sockfd, buf, BUFSIZE, 0, p->ai_addr, &servlen)) <= 0)
          error("ERROR, COULD NOT RECEIVE FILE FROM SERVER");
        memcpy(header, buf, HEADER);
        memcpy(&ns_length, header+1, sizeof(ns_length));
        ns_length = ntohs(ns_length);

        if((in_packet = (char*)malloc(ns_length * sizeof(char))) == NULL)
        {
          error("malloc failed to allocate memory");
        }
        enqueue(intake, in_packet);
        memcpy(in_packet, buf+HEADER, n-HEADER); // adding bytes we've received to packet buffer

        if((uint16_t)n < ns_length)
        {
          complete = 0;
          bytes_left = ns_length - (uint16_t)n;
        }
        else
        {
          // dequeue and write to file
          // free memory
        }

        printf("%s", buf+HEADER);

        while((cmp = strncmp(header, ex_mode , sizeof(char))) != 0 && complete) // must use single quotes for char literals
        {
          bzero(buf, BUFSIZE);
          if((n = recvfrom(sockfd, buf, BUFSIZE, 0, p->ai_addr, &servlen)) <= 0)
            error("ERROR, COULD NOT RECEIVE PACKET FROM SERVER");

          if(!complete)
          {
            if (n > bytes_left) // edge case :: only two bytes are left after subtracting what was missing
            {
              memcpy(in_packet+(ns_length-bytes_left), buf, bytes_left); // might need to add '\0'
              char* dq = dequeue(intake);
              // write bytes to file 'fp'
              free(dq);
              complete = 1;
              if (cmp == 0)
                continue; // escape from loop otherwise continue parsing packets
            }
            else
            {
              memcpy(in_packet+(ns_length-bytes_left), buf, n);
              if((bytes_left -= n) == 0)
              {
                char* dq = dequeue(intake);
                // write bytes to file 'fp'
                free(dq);
                complete = 1;
              }
              continue; // get out what we can from the buffer but continue trying to get all the bytes
            }
            // check how many bytes are in the new recvfrom()
            // if there is more than what we're missing we can just takes those bytes
            // and append them to the end of the item we just queued and then dequeue it
            // part of the dequeue could take in a file pointer possibly to add those to a file
            // open for writing. we'll probably have a chunk of the next packet if there even is one
            // (check mode before parsing after if(!complete){} ). if our buffer does include the next
            // packet i can simply do buf+bytes_left to point to the position of the new packet in
            // the buffer
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

          if((uint16_t)n-HEADER-bytes_left < ns_length)
          {
            complete = 0;
            bytes_left = ns_length - (uint16_t)n-HEADER-bytes_left;
          }
          else
          {
            // dequeue and write to file
            // free memory
          }



        //in_packet = (char*)malloc(sz_packet * sizeof(char));
          printf("\n\nTHIS IS THE LENGTH: %u\n\n", ns_length);
          printf("%s", buf+bytes_left+HEADER);
        }


      }
      // make the logic to loop until the "X" mode is received
      // i can still make the packet size dynamic but now i just have to make sure
      // to first parse the data_size value to keep track of the packet length and
      // for however long i need to get it all completely
      // keep the 1024 buffer into circular buffer to hold two packets
    }
    return 0;
}
