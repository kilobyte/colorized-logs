#include <unistd.h>
#include <stdio.h>
#include "tintin.h"

int main()
{
    struct ttyrec_header th;
    char buf[BUFFER_SIZE];
    int s,n;
    
    while(1)
    {
        if (read(0, &th, 12)!=12)
            return 0;
        n=from_little_endian(th.len);
        while(n>0)
        {
            s=(n>BUFFER_SIZE)?BUFFER_SIZE:n;
            n-=s;
            if (read(0, buf, s)!=s)
            {
                fprintf(stderr, "File was truncated\n");
                return 1;
            }
            if (write(1, buf, s)!=s)
            {
                fprintf(stderr, "Write error\n");
                return 1;
            }
        }
    }
    
    return 0;
}
