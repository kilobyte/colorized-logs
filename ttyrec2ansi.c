#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "tintin.h"

int main()
{
    struct ttyrec_header th;
    char buf[BUFFER_SIZE];
    int s,n,r;

    while (1)
    {
        if (read(0, &th, 12)!=12)
            return 0;
        n=from_little_endian(th.len);
        while (n>0)
        {
            s=(n>BUFFER_SIZE)?BUFFER_SIZE:n;
            if ((r=read(0, buf, s))<=0)
            {
                fprintf(stderr, "%s\n", r?strerror(errno):"File was truncated");
                return 1;
            }
            if (write(1, buf, r)!=r)
            {
                fprintf(stderr, "Write error\n");
                return 1;
            }
            n-=r;
        }
    }

    return 0;
}
