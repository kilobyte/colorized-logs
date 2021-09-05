#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/param.h>
#ifdef __ANDROID__
  #include <sys/endian.h>
#endif
#ifdef __sun
# include <sys/isa_defs.h>
# define LITTLE_ENDIAN 1234
# define BIG_ENDIAN    4321
# ifdef _LITTLE_ENDIAN
#  define BYTE_ORDER LITTLE_ENDIAN
# else
#  define BYTE_ORDER BIG_ENDIAN
# endif
#endif

#ifndef htole32
 #if BYTE_ORDER == LITTLE_ENDIAN
  #define htole32(x) (x)
 #else
  #if BYTE_ORDER == BIG_ENDIAN
   #define htole32(x) \
       ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
        (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
  #else
   #error No endianness given.
  #endif
 #endif
#endif

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
        if (fread(&th, 1, 12, stdin)!=12)
            return 0;
        n=htole32(th.len);
        while (n>0)
        {
            s=(n>BUFFER_SIZE)?BUFFER_SIZE:n;
            if ((r=fread(buf, 1, s, stdin))<=0)
            {
                fprintf(stderr, "%s\n", r?strerror(errno):"File was truncated");
                return 1;
            }
            if ((ssize_t)fwrite(buf, 1, r, stdout)!=r)
            {
                fprintf(stderr, "Write error\n");
                return 1;
            }
            n-=r;
        }
    }

    return 0;
}
