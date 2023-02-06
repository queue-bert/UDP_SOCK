#include <stdio.h>
#include <stdlib.h>
#include "util.h"

int sendall(int s, char *buf, int *len, struct addrinfo* p)
{
    int total = 0;
    int bytesleft = *len;
    int n;

    while(total < *len)
    {
        if((n = sendto(s, buf+total, bytesleft, 0, p->ai_addr, p->ai_addrlen)) <= 0)
        {
            break;
        }
        else
        {
            total += n;
            bytesleft -= n;
        }
    }

    *len = total;

    return n==-1?-1:0;
}