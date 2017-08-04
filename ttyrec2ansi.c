#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "config.h"

#ifdef WORDS_BIGENDIAN
# define to_little_endian(x) ((uint32_t) ( \
    ((uint32_t)(x) &(uint32_t)0x000000ffU) << 24 | \
    ((uint32_t)(x) &(uint32_t)0x0000ff00U) <<  8 | \
    ((uint32_t)(x) &(uint32_t)0x00ff0000U) >>  8 | \
    ((uint32_t)(x) &(uint32_t)0xff000000U) >> 24))
#else
# define to_little_endian(x) ((uint32_t)(x))
#endif
#define from_little_endian(x) to_little_endian(x)

struct ttyrec_header
{
    uint32_t sec;
    uint32_t usec;
    uint32_t len;
};

#define BUFFER_SIZE 4096


int main(void)
{
    struct ttyrec_header th;
    char buf[BUFFER_SIZE];
    int s, n, r;

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
