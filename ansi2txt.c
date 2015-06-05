#include <stdio.h>

int main()
{
    int ch;
    do
    {
        if ((ch=getchar())=='\n')
            putchar('\n');
        else
        if ((ch>=32)&&(ch<127))
            putchar(ch);
        else
        if (ch==27)
            while (((ch=getchar())=='[')||(ch==',')||((ch>='0')&&(ch<='9'))||(ch==';'));
    }
    while (ch!=EOF);
    return 0;
}
