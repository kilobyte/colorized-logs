#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#define BOLD         0x010000
#define DIM          0x020000
#define ITALIC       0x040000
#define UNDERLINE    0x080000
#define BLINK        0x100000
#define INVERSE      0x200000
#define STRIKE       0x400000

static bool no_header=false, white=false, contrast=false, no_wrap=false, in_span;
static bool in_hyperlink=false;
static int fg, bg, fl, frgb, brgb;
static const char *title=0;
static char *style=0;

static const char *cols[]={"BLK","RED","GRN","YEL","BLU","MAG","CYN","WHI",
                           "HIK","HIR","HIG","HIY","HIB","HIM","HIC","HIW"};

typedef unsigned char u8;


static int rgb_from_256(int i)
{
    if (i < 16)
    {   // Standard colours.
        if (white)
        {
            if (i == 3)
                return 0x806000;
            if (i == 3+8)
                return 0xcccc00;
            int c = (i&1 ? 0x010000 : 0x000000)
                  | (i&2 ? 0x000100 : 0x000000)
                  | (i&4 ? 0x000001 : 0x000000);
            return i<8 ? c*0x80 : c*0xff;
        }
        if (i == 3)
            return 0xaa5500;
        int c = (i&1 ? 0xaa0000 : 0x000000)
              | (i&2 ? 0x00aa00 : 0x000000)
              | (i&4 ? 0x0000aa : 0x000000);
        return i<8 ? c : c+0x555555;
    }
    else if (i < 232)
    {   // 6x6x6 colour cube.
        i-=16;
        int r = i / 36, g = i / 6 % 6, b = i % 6;
        return (r ? r * 0x280000 + 0x370000 : 0)
             | (g ? g * 0x002800 + 0x003700 : 0)
             | (b ? b * 0x000028 + 0x000037 : 0);
    }
    else// Grayscale ramp.
        return i*0xa0a0a-((232*10-8)*0x10101);
}


static inline int rgb_to_int(u8 r, u8 g, u8 b)
{
    return (int)r<<16|(int)g<<8|(int)b;
}


static int get_frgb(int fg)
{
    return fg==-1?white?0x000000:0xaaaaaa:rgb_from_256(fg);
}


static int get_brgb(int bg)
{
    return bg==-1?white?0xaaaaaa:0x000000:rgb_from_256(bg);
}


static void span(void)
{
    int tmp, _fg=fg, _bg=bg, _frgb=frgb, _brgb=brgb;
    char clbuf[32], *cl=clbuf;

    if (fg==-1 && bg==-1 && frgb==-1 && brgb==-1 && !fl)
        return;
    if (fl&INVERSE)
    {
        if (_fg==-1)
            _fg=white?0:7;
        if (_bg==-1)
            _bg=white?7:0;
        tmp=_fg; _fg=_bg; _bg=tmp;
        tmp=_frgb; _frgb=_brgb; _brgb=tmp;
    }
    if (fl&BLINK)
    {
        if (_frgb==-1)
            _frgb=get_frgb(_fg);
        _frgb=rgb_to_int((_frgb>>16&0xff)*3/4,
                         (_frgb>> 8&0xff)*3/4,
                         (_frgb    &0xff)*3/4)+0x606060;
        if (_brgb==-1)
            _brgb=get_brgb(_bg);
        _brgb=rgb_to_int((_brgb>>16&0xff)*3/4,
                         (_brgb>> 8&0xff)*3/4,
                         (_brgb    &0xff)*3/4)+0x606060;
    }
    if (fl&DIM)
        _fg=8;

    if (contrast && (_frgb==-1?get_frgb(_fg):_frgb)==(_brgb==-1?get_brgb(_bg):_brgb))
    {
        if (_frgb==-1)
            _frgb=get_frgb(_fg);
        _frgb^=0x808080;
    }

    if (no_header)
        goto do_span;

    printf("<b"); // Redefining <b> helps Braille and speech readers.
    if (_fg!=-1)
    {
        if (fl&BOLD)
            _fg|=8;
        cl+=sprintf(cl, " %s", cols[_fg]);
    }
    else if (fl&BOLD)
        cl+=sprintf(cl, " BOLD");

    if (_bg!=-1)
        cl+=sprintf(cl, " B%s", cols[_bg]);

    if (fl&ITALIC)
        cl+=sprintf(cl, " ITA");
    if (fl&UNDERLINE)
        cl+=sprintf(cl, (fl&STRIKE)?" UNDSTR":" UND");
    else if (fl&STRIKE)
        cl+=sprintf(cl, " STR");

    if (cl>clbuf)
    {
        *cl=0;
        if (cl>=clbuf+5)
            printf(" class=\"%s\"", clbuf+1);
        else // implies no spaces
            printf(" class=%s", clbuf+1);
    }

    if (_frgb!=-1 || _brgb!=-1)
    {
        printf(" style=\"");
        if (_frgb!=-1)
            printf("color:#%06x;", _frgb);
        if (_brgb!=-1)
            printf("background-color:#%06x", _brgb);
        printf("\"");
    }

    printf(">");
    in_span=1;
    return;

do_span:
    printf("<span style=\"");
    if (_frgb!=-1)
    {
        printf("color:#%06x", _frgb);
        if (fl&BOLD)
            printf(";font-weight:bold");
    }
    else if (_fg!=-1)
    {
        if (fl&BOLD)
            printf("color:#%c%c%c", _fg&1?'f':'5', _fg&2?'f':'5', _fg&4?'f':'5');
        else
            printf("color:#%c%c%c", _fg&1?'a':'0', _fg&2?'a':'0', _fg&4?'a':'0');
    }
    else if (fl&BOLD)
        printf("font-weight:bold");

    if (_brgb!=-1)
        printf(";background-color:#%06x", _brgb);
    else if (_bg!=-1)
        printf(";background-color:#%c%c%c", _bg&1?'a':'0', _bg&2?'a':'0', _bg&4?'a':'0');

    if (fl&ITALIC)
        printf(";font-style:italic");
    if (fl&(UNDERLINE|BLINK|STRIKE))
    {
        printf(";text-decoration:");
        if (fl&UNDERLINE)
            printf(" underline");
        if (fl&BLINK)
            printf(" blink");
        if (fl&STRIKE)
            printf(" line-through");
    }

    printf("\">");
    in_span=1;
}


static void unspan(void)
{
    if (in_span)
        printf(no_header?"</span>":"</b>");
    in_span=0;
}


static void print_string(const char *restrict str)
{
    for (; *str; str++)
        switch (*str)
        {
        case '<':
            printf("&lt;");
            break;
        case '>':
            printf("&gt;");
            break;
        case '&':
            printf("&amp;");
            break;
        default:
            putchar(*str);
        }
}


// return value is pending ch
static int osc(void)
{
    int ch=getchar();
    if (ch<'0'||ch>'9') /* not an OSC, don't try to parse */
        return ch;
    int cmd=0;
    while (ch>='0' && ch<='9')
    {
        cmd=cmd*10 + ch-'0';
        ch=getchar();
    }
    if (ch==';')
        ch=getchar();
    else
        cmd=-1; // not a well-formed OSC

    char str[4096], *ps=str;

    for (;;ch=getchar())
        switch (ch)
        {
        case 27:
            ch=getchar(); // if not ST, ESC is illegal here
            if (ch!='\\')
                goto abort;
            // fallthru
        case 7:
            ch=getchar(); // BELL is the alternate terminator
            *ps=0;
            goto ok;
        default:
            if (ps < str+sizeof(str)-1)
            {
                *ps++=ch;
                break;
            }
            // fallthru
        case EOF:
        abort:
            *ps=0;
            print_string(str);
            return ch;
        }

ok:
    if (cmd!=8) // hyperlink
        return ch;
    unspan();
    if (in_hyperlink)
        printf("</a>"), in_hyperlink=0;
    // only the first semicolon (after cmd number) is mandatory when closing
    ps=strchr(str, ';');
    if (!ps)
        return ch;
    if (*ps)
        ps++;
    if (ps[strcspn(ps, "\"<>")])
        return ch;
    if (!*ps)
        return ch;
    printf("<a href=\"%s\">", ps);
    in_hyperlink=1;
    return ch;
}


int main(int argc, char **argv)
{
    while (1)
    {
        static const struct option long_options[] =
        {
            {"no-header",       0, 0, 'n'},
            {"white",           0, 0, 'w'},
            {"contrast",        0, 0, 'c'},
            {"title",           1, 0, 't'},
            {"no-wrap",         0, 0, 'l'},
            {"style",           1, 0, -257},
            {0,                 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-nwt:lc", long_options, 0);
        if (c == -1)
            break;

        switch (c)
        {
        case 'n':
            no_header=true;
            break;
        case 'w':
            white=true;
            break;
        case 'c':
            contrast=true;
            break;
        case 't':
            if (title)
                return fprintf(stderr, "%s: title was already given.\n", argv[0]), 1;
            title=optarg;
            break;
        case 'l':
            no_wrap=true;
            break;
        case -257:
            if (style)
                return fprintf(stderr, "%s: style was already given.\n", argv[0]), 1;
            style=optarg;
            break;
        case '?':
            return 1;
        case 1:
            return fprintf(stderr, "%s: this program works as a filter, please "
                           "pipe the input in instead.\n", argv[0]), 1;
        }
    }

    if (no_header)
    {
        if (title || style)
        {
            return fprintf(stderr, "%s: --no-header forbids --title and --style.\n",
                           argv[0]), 1;
        }
        printf(
"<pre style=\"color:#%s%s\">",
                white?"000":"bbb",
                no_wrap?"":";white-space:pre-wrap;word-wrap:break-word;overflow-wrap:break-word");
    }
    else
    {
        printf(
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n");
        if (title)
        {
            printf("<title>");
            print_string(title);
            printf("</title>\n");
        }
        printf(
"<style type=\"text/css\">\n"
"body {background-color: %s;}\n"
"pre {\n"
"\tfont-weight: normal;\n"
"\tcolor: #%s;\n"
"%s"
"}\n"
"b {font-weight: normal}\n"
"b.BOLD {color: #%s}\n"
"b.ITA {font-style: italic}\n"
"b.UND {text-decoration: underline}\n"
"b.STR {text-decoration: line-through}\n"
"b.UNDSTR {text-decoration: underline line-through}\n",
                white?"white":"black",
                white?"000":"bbb",
                no_wrap?"":
"\twhite-space: -moz-pre-wrap;\n"
"\twhite-space: -o-pre-wrap;\n"
"\twhite-space: -pre-wrap;\n"
"\twhite-space: pre-wrap;\n"
"\tword-wrap: break-word;\n"
"\toverflow-wrap: break-word;\n",
                white?"000;font-weight:bold":"fff");
        for (int i=0; i<16; i++)
            printf("b.%s {color: #%06x}\n", cols[i], rgb_from_256(i));
        for (int i=0; i<8; i++)
            printf("b.B%s {background-color: #%06x}\n", cols[i], rgb_from_256(i));
        if (style)
            printf("\n%s\n", style);
        printf(
"</style>\n"
"</head>\n"
"<body>\n"
"<pre>");
    }
    fg=bg=-1;
    fl=0;
    in_span=false;
    frgb=brgb=-1;
    int ch=getchar();
    unsigned int ntok, tok[16];
normal:
    switch (ch)
    {
    case EOF:
        unspan();
        printf("</pre>\n");
        if (!no_header)
            printf("</body>\n</html>\n");
        return 0;
    case 0:  case 1:  case 2:  case 3:  case 4:  case 5:  case 6:
                               case 11:                   case 14: case 15:
    case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 24: case 25: case 26:          case 28: case 29: case 30: case 31:
        printf("&#x24%02X;", ch);
        ch=getchar();
        goto normal;
    case 7:
        printf("&#x266A;");     // bell
        ch=getchar();
        goto normal;
    case 8:
        printf("&#x232B;");     // backspace
        ch=getchar();
        goto normal;
    case 12:                    // form feed
    formfeed:
        ch=getchar();
        unspan();
        printf("\n<hr>\n");
        goto normal;
    case 13:
        ch=getchar();
        unspan();
        if (ch!=10)
            printf("&crarr;\n");
        goto normal;
    case 27:                    // ESC
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
    case 127:
        printf("&#x2326;");     // delete
        ch=getchar();
        goto normal;
    case 10:                    // newline
        if (in_hyperlink)
        {
            unspan();
            printf("</a>");
            in_hyperlink=0;
        }
        // fallthru
    default:
        putchar(ch);
        ch=getchar();
        goto normal;
    }
/****************************************************************************/
esc:
    switch (ch)
    {
    case '[':
        break;
    case ']':
        ch=osc();
        goto normal;
    case '%':
    case '(':
    case ')':
        ch=getchar();
        // fallthru
    default:
        ch=getchar();
        goto normal;
    }
    /* [ */
    ch=getchar();
    ntok=0;
    tok[0]=0;
/****************************************************************************/
csi:
    switch (ch)
    {
    case '?':
        ch=getchar();
        goto csiopt;
    case ';':
        if (++ntok>=sizeof(tok)/sizeof(tok[0]))
            goto normal;        // too many tokens, something is fishy
        tok[ntok]=0;
        ch=getchar();
        goto csi;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        tok[ntok]=tok[ntok]*10+ch-'0';
        ch=getchar();
        goto csi;
    case 'm':
        for (unsigned int i=0;i<=ntok;i++)
            switch (tok[i])
            {
            case 0:
                fg=bg=-1;
                fl=0;
                frgb=brgb=-1;
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
            case 9:
                fl|=STRIKE;
                break;
            case 21:
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
            case 29:
                fl&=~STRIKE;
                break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                fg=tok[i]-30;
                frgb=-1;
                break;
            case 38:
                i++;
                if (i>ntok)
                    break;
                if (tok[i]==5 && i<ntok)
                {   // 256 colours
                    i++;
                    frgb=rgb_from_256(tok[i]);
                }
                else if (tok[i]==2 && i+3<=ntok)
                {   // 24 bit
                    frgb=rgb_to_int(tok[i+1], tok[i+2], tok[i+3]);
                    i+=3;
                }
                // Subcommands 3 (CMY) and 4 (CMYK) are so insane
                // there's no point in supporting them.
                break;
            case 39:
                fg=-1;
                frgb=-1;
                break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                bg=tok[i]-40;
                brgb=-1;
                break;
            case 48:
                i++;
                if (i>ntok)
                    break;
                if (tok[i]==5 && i<ntok)
                {   // 256 colours
                    i++;
                    brgb=rgb_from_256(tok[i]);
                }
                else if (tok[i]==2 && i+3<=ntok)
                {   // 24 bit
                    brgb=rgb_to_int(tok[i+1], tok[i+2], tok[i+3]);
                    i+=3;
                }
                break;
            case 49:
                bg=-1;
                brgb=-1;
                break;
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                frgb=rgb_from_256(tok[i]-82);
                break;
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                brgb=rgb_from_256(tok[i]-92);
                break;
            }
        unspan();
        span();
        ch=getchar();
        goto normal;
    case 'C':
        ntok=tok[0];
        if (ntok<=0)
            ntok=1;
        else if (ntok>512) // sanity
            ntok=512;
        for (unsigned int i=0;i<ntok;++i)
            printf(" ");
        ch=getchar();
        goto normal;
    case 'J': // screen clear
        goto formfeed;
    default:
        ch=getchar();           // invalid/unimplemented code, ignore
    case EOF:
        goto normal;
    }
/****************************************************************************/
csiopt:
    if (ch==';'||(ch>='0'&&ch<='9'))
    {
        ch=getchar();
        goto csiopt;
    }
    ch=getchar();
    goto normal;
}
