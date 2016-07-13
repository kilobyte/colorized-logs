#include <stdio.h>

int main()
{
    int ch;
    do
    {
        ch=getchar();
        while (ch==13)
            if ((ch=getchar())!=10)
                putchar(13); /* suppress \r only when followed by \n */
        if (ch==27)
            if ((ch=getchar())=='[')
                while ((ch=getchar())==';'||(ch>='0'&&ch<='9')||ch=='?');
            else if (ch==']'&&(ch=getchar())>=0&&ch<='9')
                for (;;)
                {
                    if ((ch=getchar())==EOF||ch==7)
                        break;
                    else if (ch==27)
                        {ch=getchar(); break;}
                }
            else if (ch=='%')
                ch=getchar();
            else {}
        else if (ch!=EOF)
            putchar(ch);
    }
    while (ch!=EOF);
    return 0;
}
