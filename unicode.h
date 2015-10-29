#define WC wchar_t
#define WCL sizeof(WC)
#define WCI wint_t
#define WCC "%lc"
#define WClen wcslen
#define WCcpy wcscpy
#define TO_WC(d,s) utf8_to_wc(d,s,-1)
#define FROM_WC(d,s,n) wc_to_utf8(d,s,n,BUFFER_SIZE*8)
#define WRAP_WC(d,s) wc_to_utf8(d,s,-1,BUFFER_SIZE)
#define OUT_WC(d,s,n) wc_to_mb(d,s,n,OUTSTATE)
#define FLATlen utf8_width
int wcwidth(WC ucs);
int wcswidth(const WC *pwcs, size_t n);
#if 0
#define WC char
#define WCL 1
#define WCI int
#define WCC "%c"
#define WClen strlen
#define WCcpy strcpy
#define TO_WC(d,s) strcpy(d,s)
#define FROM_WC(d,s,n) (memcpy(d,s,n),n)
#define WRAP_WC(d,s) strcpy(d,s)
#define OUT_WC(d,s,n) (memcpy(d,s,n),n)
#define FLATlen strlen
#define iswalnum(x) isalnum(x)
#define towupper(x) toupper(x)
#define towlower(x) tolower(x)
#endif
