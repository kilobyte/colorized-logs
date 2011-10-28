#include "tintin.h"

static const char* cdigit[] = { "零", "一", "二", "三", "四", "五", "六", "七", "八", "九",
                                "十", "百", "千", "万", "兩", "萬",
                              };
static const int nod = sizeof(cdigit)/sizeof(char*);

void ctoi(char* input)
{
    char result[BUFFER_SIZE];
    int i, j, k, num;
    int lastunit, lastdigit=0;
    char* tmp;
    j = 0;
    tmp = input;
    while (*tmp&&*(tmp+1))
    {
        for (i=0;i<nod;i++)
            if (!strncmp(tmp, cdigit[i], 3))
            {
                if (i==14)      /* synonyms */
                    i=2;
                else if (i==15)
                    i=13;
                result[j]=i;
                goto ok;
            }
        break;
    ok:
        j++;
        tmp+=3;
    }
    result[j]=0; /*just sth != 10, chitchat*/
    num = j;
    lastunit = 0;
    for (i=0, j=0;i<num;i++)
    {
        if (result[i]==0)
        {
            if (result[i+1]!=10) continue;
            else result[i]=1; /*for 一千零十, change to 一千一十, chitchat*/
        }
        if (i==0&&result[0]==10)
        {
            lastunit = 10;
            input[j] = '1';
            j++;
            lastdigit = 0;
        }
        else if (result[i]<10)
        {
            if (lastunit<=10)
            {
                input[j] = '0' + result[i];
                j++;
                lastunit = 0;
            }
            else
                lastdigit = result[i];
        }
        else if (result[i]<13)
        {
            if (lastunit>result[i]+1)
                for (k=0;k<lastunit-result[i]-1;k++)
                {
                    input[j] = '0';
                    j++;
                }
            if (lastunit)
            {
                input[j] = '0' + lastdigit;
                j++;
            }
            lastunit = result[i];
            lastdigit = 0;
        }
        else if (result[i]==13)
        {
            if (lastunit>=10)
                for (k=0;k<lastunit-9;k++)
                {
                    input[j] = '0';
                    j++;
                }
            lastunit = 13;
            lastdigit = 0;
        }
    }
    if (lastunit>=10)
    {
        for (k=0;k<lastunit-9;k++)
        {
            input[j] = '0';
            j++;
        }
        if (lastdigit) input[j-1]='0'+lastdigit;
    }
    if (j==0) input[j++]='0'; //handle #ctoi x {零}, chit, 7/10/2000
    input[j] = '\0';
}
