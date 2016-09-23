#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

static void sl(const char* loc)
{
    if (!setlocale(LC_CTYPE, loc))
        return;
    if (strcmp(nl_langinfo(CODESET), "UTF-8"))
        return;
    printf("%s\n", loc);
    exit(0);
}

int main(void)
{
    /* try preferred values first */
    sl("C.UTF-8");
    sl("en_US.UTF-8");

    DIR* dir = opendir("/usr/share/locale");
    /* In that dir, FreeBSD and Solaris have qualified locale names.  OpenBSD
       has bare "UTF-8" which does work.  Linux has unqualified dirs whose
       names form ancient locales (except for some, like "vi"), but on the
       other hand, Linux is likely to have "C.UTF-8".
    */
    if (!dir)
    {
        fprintf(stderr, "Can't read /usr/share/locale: %s\n", strerror(errno));
        return 1;
    }
    struct dirent *d;
    while ((d = readdir(dir)))
        sl(d->d_name);
    return 1;
}
