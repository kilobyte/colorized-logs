/*
Note: <i> is used instead of <blink> because of brain-deadness of Internet
      Exploder.  Since not everyone has a decent browser, we have to support
      deficiencies of Micro$loth products.
*/
#include <stdio.h>

#define BUFFER_SIZE 2048

char cnames[2][8][8]=
   {{"#000000","#AA0000","#00AA00","#AAAA00","#0000AA","#AA00AA","#00AAAA","#AAAAAA"},
    {"#555555","#FF5555","#55FF55","#FFFF55","#5555FF","#FF55FF","#55FFFF","#FFFFFF"}};
char bnames[8][8]=
    {"#000000","#AA0000","#00AA00","#AAAA00","#0000AA","#AA00AA","#00AAAA","#AAAAAA"};

int hasbg(char *ch)
{
    int bg=0;
    ch--;
normal:
    switch(*++ch)
    {
    case 0:
    case '\n':
        return 0;
    case 27:
        goto esc;
    }
    goto normal;
esc:
    switch(*++ch)
    {
    case '\n':
    case 0:
        return 0;
    case '[':
        goto esc;
    case '3':
        if (!*++ch || *ch=='\n')
            return 0;
    case '0':
    case '1':
    case '2':
    case '5':
    case '7':
        break;
    case '4':
        if (*++ch!='0')
            bg=1;
        if (!*ch || *ch=='\n')
            return 0;
    };
    switch(*++ch)
    {
    case '0':
    case '\n':
        return 0;
    case 'm':
        if (bg)
            return 1;
        goto normal;
    case ';':
        goto esc;
    default:
        bg=0;
    };
    goto normal;
}


int main()
{
    char line[BUFFER_SIZE],*ch;
    int table;
    int oldbl=0, bl=0;
    int oldbg=-1, bg=0;
    int oldbr=0, br=0;
    int oldcolor=7, color=7;
    int tmp;

    printf("<html>\n<body bgcolor=%s text=%s>\n<pre><tt>",bnames[0],cnames[0][7]);
    while (fgets(line,BUFFER_SIZE,stdin))
    {
        table=hasbg(line);
        if (table)
            printf("<table cellpadding=0 cellspacing=0 border=0><tr>");
        oldbl=0;
        oldbg=(table? -1 : 0);
        oldbr=0;
        oldcolor=7;
        ch=line-1;
normal:
        ch++;
        if (!*ch || *ch=='\n')
            goto end;
        if (*ch==27)
            goto esc;
        if ((bg!=oldbg)||(color!=oldcolor)||(br!=oldbr)||(bl!=oldbl))
        {
            if (oldbl)
                printf("</i>");
            if (oldbr)
                printf("</b>");
            if (oldcolor!=7)
                printf("</font>");
            if ((bg!=oldbg))
            {
                if (oldbg!=-1)
                    printf("</tt></td>");
                if (bg)
                    printf("<td bgcolor=%s><tt>",bnames[bg]);
                else
                    printf("<td><tt>");
            }
            oldcolor=color;
            oldbr=br;
            oldbg=bg;
            oldbl=bl;
            if (color!=7)
                printf("<font color=%s>",cnames[br][color]);
            if (br)
                printf("<b>");
            if (bl)
                printf("<i>");
        }
        switch(*ch)
        {
        case '<':
            printf("&lt;");
            break;
        case '>':
            printf("&gt;");
            break;
        case ' ':
            if (table)
            {
                printf("&nbsp;");
                break;
            }
        default:
            putchar(*ch);
        }
        goto normal;
esc:
        switch(*++ch)
        {
        case 0:
        case '\n':
            goto end;
        case '[':
            goto esc;
        case '0':
            color=7;
            bg=0;
            bl=0;
            br=0;
            break;
        case '1':
            br=1;
            break;
        case '2':
            br=0;
            break;
        case '3':
            if (!*++ch || *ch=='\n')
                goto end;
            color=*ch-'0';
            break;
        case '4':
            if (!*++ch || *ch=='\n')
                goto end;
            bg=*ch-'0';
            break;
        case '5':
            bl=1;
            break;
        case '7':
            tmp=color, color=bg, bg=tmp;
        };
        goto igs;
igs:
        switch(*++ch)
        {
        case 'm':
            goto normal;
        case 0:
        case '\n':
            goto end;
        case ';':
            goto esc;
        default:
            goto igs;
        };
end:
        if (oldbl)
            printf("</i>");
        if (oldbr)
            printf("</b>");
        if (oldcolor!=7)
            printf("</font>");
        if (table)
        {
            if (oldbg!=-1)
                printf("</tt></td>");
            else
                printf("<td>&nbsp;</td>"); /* empty line */
            printf("</tr></table>");
        }
        else
            printf("\n");
    };
    printf("</tt></pre>\n</body>\n</html>\n");
    return(0);
}
