#include "config.h"
#ifdef HAVE_GLIB
# include <glib.h>
# define MALLOC(s)	g_slice_alloc(s)
# define TALLOC(t)	((t*)g_slice_new(t))
# define CALLOC(n,t)	((t*)g_slice_alloc0(n*sizeof(t)))
# define MFREE(x,s)	g_slice_free1(s, x)
# define TFREE(x,t)	g_slice_free(t, x)
# define CFREE(x,n,t)	g_slice_free1(n*sizeof(t), x)
# define SFREE(x)	{if (x) g_slice_free1(strlen(x)+1, x);}
# define LFREE(x)	TFREE(x, struct listnode)
#else
# define MALLOC(s)	malloc(s)
# define TALLOC(t)	((t*)malloc(sizeof(t)))
# define CALLOC(n,t)	((t*)calloc(n, sizeof(t)))
# define MFREE(x,s)	free(x)
# define TFREE(x,t)	free(x)
# define CFREE(x,n,t)	free(x)
# define SFREE(x)	free(x) /* string */
# define LFREE(x)	free(x) /* struct listnode */
#endif
