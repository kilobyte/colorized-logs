#include <stdio.h>

#define BOLD         0x010000
#define DIM          0x020000
#define ITALIC       0x040000
#define UNDERLINE    0x080000
#define BLINK        0x100000
#define INVERSE      0x200000

static int fg,bg,fl,b,cl;

static char *cols[]={"BLK","RED","GRN","YEL","BLU","MAG","CYN","WHI",
                     "HIK","HIR","HIG","HIY","HIB","HIM","HIC","HIW"};

static int ntok, tok[10];
static int ch;

static void class()
{
    if (!cl)
        printf(" class=\"");
    else
        printf(" ");
    cl=1;
}

static void span()
{
    int tmp, _fg=fg, _bg=bg;

    if (fg==-1 && bg==-1 && !fl)
        return;
    printf("<b");
    cl=0;
    if (fl&INVERSE)
    {
        if (_fg==-1)
            _fg=7;
        if (_bg==-1)
            _bg=0;
        tmp=_fg; _fg=_bg; _bg=tmp;
    }
    if (fl&DIM)
        fg=8;
    if (_fg!=-1)
    {
        if (fl&BOLD)
            _fg|=8;
        class();printf("%s", cols[_fg]);
    }
    else if (fl&BOLD)
        class(),printf("BOLD");

    if (_bg!=-1)
        class(),printf("B%s", cols[_bg]);

    if (fl&ITALIC)
        class(),printf("ITA");
    if (fl&UNDERLINE)
        class(),printf((fl&BLINK)?"UNDBLI":"UND");
    else if (fl&BLINK)
        class(),printf("BLI");

    if (cl)
        printf("\"");
    printf(">");
    b=1;
}


static void unspan()
{
    if (b)
        printf("</b>");
    b=0;
}


int main()
{
    int i;

    printf(
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
"\t\"http://www.w3.org/TR/html4/strict.dtd\">\n"
"<html>\n"
"<head>\n"
"<!--<title></title>-->\n"
"<style type=\"text/css\">\n"
"body {background-color: black;}\n"
"pre {\n"
"\tfont-weight: normal;\n"
"\tcolor: #bbb;\n"
"\twhite-space: -moz-pre-wrap;\n"
"\twhite-space: -o-pre-wrap;\n"
"\twhite-space: -pre-wrap;\n"
"\twhite-space: pre-wrap;\n"
"\tword-wrap: break-word;\n"
"}\n"
"b {font-weight: normal}\n"
"b.BLK {color: #000}\n"
"b.BLU {color: #00a}\n"
"b.GRN {color: #0a0}\n"
"b.CYN {color: #0aa}\n"
"b.RED {color: #a00}\n"
"b.MAG {color: #a0a}\n"
"b.YEL {color: #aa0}\n"
"b.WHI {color: #aaa}\n"
"b.HIK {color: #555}\n"
"b.HIB {color: #55f}\n"
"b.HIG {color: #5f5}\n"
"b.HIC {color: #5ff}\n"
"b.HIR {color: #f55}\n"
"b.HIM {color: #f5f}\n"
"b.HIY {color: #ff5}\n"
"b.HIW {color: #fff}\n"
"b.BOLD {color: #fff}\n"
"b.BBLK {background-color: #000}\n"
"b.BBLU {background-color: #00a}\n"
"b.BGRN {background-color: #0a0}\n"
"b.BCYN {background-color: #0aa}\n"
"b.BRED {background-color: #a00}\n"
"b.BMAG {background-color: #a0a}\n"
"b.BYEL {background-color: #aa0}\n"
"b.BWHI {background-color: #aaa}\n"
"b.ITA {font-style: italic}\n"
"b.UND {text-decoration: underline}\n"
"b.BLI {text-decoration: blink}\n"
"b.UNDBLI {text-decoration: underline blink}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<pre>");
    fg=bg=-1;
    fl=0;
    b=0;
    ch=getchar();
normal:
    switch (ch)
    {
    case EOF:
        unspan();
        printf("</pre>\n</body>\n</html>\n");
        return 0;
    case 0:  case 1:  case 2:  case 3:  case 4:  case 5:  case 6:
    case 8:                    case 11:                   case 14: case 15:
    case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 24: case 25: case 26:          case 28: case 29: case 30: case 31:
        ch=getchar();
        goto normal;
    case 7:
        printf("&iexcl;");      /* bell */
        ch=getchar();
        goto normal;
    case 12:                    /* form feed */
    formfeed:
        ch=getchar();
        unspan();
        printf("\n<hr>\n");
        goto normal;
    case 27:                    /* ESC */
        ch=getchar();
        goto esc;
    case '<':
        printf("&lt;");
        ch=getchar();
        goto normal;
    case '>':
        printf("&gt;");
        ch=getchar();
        goto normal;
    case '&':
        printf("&amp;");
        ch=getchar();
        goto normal;
    default:
        putchar(ch);
        ch=getchar();
        goto normal;
    }
/****************************************************************************/
esc:
    if (ch!='[')
        goto normal;
    ch=getchar();
    ntok=0;
    tok[0]=0;
/****************************************************************************/
csi:
    switch (ch)
    {
    case ';':
        if (++ntok>=10)
            goto normal;        /* too many tokens, something is fishy */
        tok[ntok]=0;
        ch=getchar();
        goto csi;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        tok[ntok]=tok[ntok]*10+ch-'0';
        ch=getchar();
        goto csi;
    case 'm':
        for (i=0;i<=ntok;i++)
            switch (tok[i])
            {
            case 0:
                fg=bg=-1;
                fl=0;
                break;
            case 1:
                fl|=BOLD;
                fl&=~DIM;
                break;
            case 2:
                fl|=DIM;
                fl&=~BOLD;
                break;
            case 3:
                fl|=ITALIC;
                break;
            case 4:
                fl|=UNDERLINE;
                break;
            case 5:
                fl|=BLINK;
                break;
            case 7:
                fl|=INVERSE;
                break;
            case 21:
                fl&=~(BOLD|DIM);
                break;
            case 22:
                fl&=~(BOLD|DIM);
                break;
            case 23:
                fl&=~ITALIC;
                break;
            case 24:
                fl&=~UNDERLINE;
                break;
            case 25:
                fl&=~BLINK;
                break;
            case 27:
                fl&=~INVERSE;
                break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                fg=tok[i]-30;
                break;
            case 39:
                fg=-1;
                break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                bg=tok[i]-40;
                break;
            case 49:
                bg=-1;
            }
        unspan();
        span();
        ch=getchar();
        goto normal;
    case 'J':
        goto formfeed;
    default:
        ch=getchar();           /* invalid/unimplemented code, ignore */
    case EOF:
        goto normal;
    }
}
