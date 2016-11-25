#include "tintin.h"
#include "protos/action.h"
#include "protos/glob.h"
#include "protos/globals.h"
#include "protos/ivars.h"
#include "protos/print.h"
#include "protos/parse.h"
#include "protos/utils.h"
#include "protos/variables.h"


extern struct session *if_command(const char *arg, struct session *ses);


static void addroute(struct session *ses, int a, int b, char *way, int dist, char *cond)
{
    struct routenode *r;

    r=ses->routes[a];
    while (r&&(r->dest!=b))
        r=r->next;
    if (r)
    {
        SFREE(r->path);
        r->path=mystrdup(way);
        r->distance=dist;
        SFREE(r->cond);
        r->cond=mystrdup(cond);
    }
    else
    {
        r=TALLOC(struct routenode);
        r->dest=b;
        r->path=mystrdup(way);
        r->distance=dist;
        r->cond=mystrdup(cond);
        r->next=ses->routes[a];
        ses->routes[a]=r;
    }
}

void copyroutes(struct session *ses1, struct session *ses2)
{
    for (int i=0;i<MAX_LOCATIONS;i++)
    {
        if (ses1->locations[i])
            ses2->locations[i]=mystrdup(ses1->locations[i]);
        else
            ses2->locations[i]=0;
    }
    for (int i=0;i<MAX_LOCATIONS;i++)
    {
        ses2->routes[i]=0;
        for (struct routenode *r=ses1->routes[i];r;r=r->next)
        {
            struct routenode *p=TALLOC(struct routenode);
            p->dest=r->dest;
            p->path=mystrdup(r->path);
            p->distance=r->distance;
            p->cond=mystrdup(r->cond);
            p->next=ses2->routes[i];
            ses2->routes[i]=p;
        }
    }
}

void kill_routes(struct session *ses)
{
    for (int i=0;i<MAX_LOCATIONS;i++)
    {
        SFREE(ses->locations[i]);
        ses->locations[i]=0;
        struct routenode *r=ses->routes[i];
        while (r)
        {
            struct routenode *p=r;
            r=r->next;
            SFREE(p->path);
            SFREE(p->cond);
            TFREE(p, struct routenode);
        }
        ses->routes[i]=0;
    }
}

int count_routes(struct session *ses)
{
    int num=0;

    for (int i=0;i<MAX_LOCATIONS;i++)
        for (struct routenode *r=ses->routes[i];r;r=r->next)
            num++;
    return num;
}

static void kill_unused_locations(struct session *ses)
{
    bool us[MAX_LOCATIONS];
    struct routenode *r;

    for (int i=0;i<MAX_LOCATIONS;i++)
        us[i]=false;
    for (int i=0;i<MAX_LOCATIONS;i++)
        if ((r=ses->routes[i]))
        {
            us[i]=true;
            for (;r;r=r->next)
                us[r->dest]=true;
        }
    for (int i=0;i<MAX_LOCATIONS;i++)
        if (ses->locations[i]&&!us[i])
        {
            SFREE(ses->locations[i]);
            ses->locations[i]=0;
        }
}

static void show_route(struct session *ses, int a, struct routenode *r)
{
    if (*r->cond)
        tintin_printf(ses, "~7~{%s~7~}->{%s~7~}: {%s~7~} d=%i if {%s~7~}",
            ses->locations[a],
            ses->locations[r->dest],
            r->path,
            r->distance,
            r->cond);
    else
        tintin_printf(ses, "~7~{%s~7~}->{%s~7~}: {%s~7~} d=%i",
            ses->locations[a],
            ses->locations[r->dest],
            r->path,
            r->distance);
}

/***********************/
/* the #route command  */
/***********************/
void route_command(const char *arg, struct session *ses)
{
    char a[BUFFER_SIZE], b[BUFFER_SIZE], way[BUFFER_SIZE], dist[BUFFER_SIZE], cond[BUFFER_SIZE];
    int j, d;

    arg=get_arg(arg, a, 0, ses);
    arg=get_arg(arg, b, 0, ses);
    arg=get_arg_in_braces(arg, way, 0);
    arg=get_arg(arg, dist, 0, ses);
    arg=get_arg_in_braces(arg, cond, 1);
    if (!*a)
    {
        tintin_printf(ses, "#THESE ROUTES HAVE BEEN DEFINED:");
        for (int i=0;i<MAX_LOCATIONS;i++)
            for (struct routenode *r=ses->routes[i];r;r=r->next)
                show_route(ses, i, r);
        return;
    }
    if (!*way)
    {
        bool first=true;
        if (!*b)
            strcpy(b, "*");
        for (int i=0;i<MAX_LOCATIONS;i++)
            if (ses->locations[i]&&match(a, ses->locations[i]))
                for (struct routenode *r=ses->routes[i];r;r=r->next)
                    if (ses->locations[i]&&
                          match(b, ses->locations[r->dest]))
                    {
                        if (first)
                        {
                            tintin_printf(ses, "#THESE ROUTES HAVE BEEN DEFINED:");
                            first=false;
                        }
                        show_route(ses, i, r);
                    }
        if (first)
            tintin_printf(ses, "#THAT ROUTE (%s) IS NOT DEFINED.", b);
        return;
    }
    int i;
    for (i=0;i<MAX_LOCATIONS;i++)
        if (ses->locations[i]&&!strcmp(ses->locations[i], a))
            goto found_i;
    if (i==MAX_LOCATIONS)
    {
        for (i=0;i<MAX_LOCATIONS;i++)
            if (!ses->locations[i])
            {
                ses->locations[i]=mystrdup(a);
                goto found_i;
            }
        tintin_eprintf(ses, "#TOO MANY LOCATIONS!");
        return;
    }
found_i:
    for (j=0;j<MAX_LOCATIONS;j++)
        if (ses->locations[j]&&!strcmp(ses->locations[j], b))
            goto found_j;
    if (j==MAX_LOCATIONS)
    {
        for (j=0;j<MAX_LOCATIONS;j++)
            if (!ses->locations[j])
            {
                ses->locations[j]=mystrdup(b);
                goto found_j;
            }
        tintin_eprintf(ses, "#TOO MANY LOCATIONS!");
        kill_unused_locations(ses);
        return;
    }
found_j:
    if (*dist)
    {
        char *err;
        d=strtol(dist, &err, 0);
        if (*err)
        {
            tintin_eprintf(ses, "#Hey! Route length has to be a number! Got {%s}.", arg);
            kill_unused_locations(ses);
            return;
        }
        if ((d<0)&&(ses->mesvar[MSG_ROUTE]||ses->mesvar[MSG_ERROR]))
            tintin_eprintf(ses, "#Error: distance cannot be negative!");
    }
    else
        d=DEFAULT_ROUTE_DISTANCE;
    addroute(ses, i, j, way, d, cond);
    if (ses->mesvar[MSG_ROUTE])
    {
        if (*cond)
            tintin_printf(ses, "#Ok. Way from {%s} to {%s} is now set to {%s} (distance=%i) condition:{%s}",
                ses->locations[i],
                ses->locations[j],
                way,
                d,
                cond);
        else
            tintin_printf(ses, "#Ok. Way from {%s} to {%s} is now set to {%s} (distance=%i)",
                ses->locations[i],
                ses->locations[j],
                way,
                d);
    }
    routnum++;
}

/*************************/
/* the #unroute command  */
/*************************/
void unroute_command(const char *arg, struct session *ses)
{
    char a[BUFFER_SIZE], b[BUFFER_SIZE];
    bool found=false;

    arg=get_arg(arg, a, 0, ses);
    arg=get_arg(arg, b, 1, ses);

    if ((!*a)||(!*b))
    {
        tintin_eprintf(ses, "#SYNTAX: #unroute <from> <to>");
        return;
    }

    for (int i=0;i<MAX_LOCATIONS;i++)
        if (ses->locations[i]&&match(a, ses->locations[i]))
            for (struct routenode**r=&ses->routes[i];*r;)
            {
                if (match(b, ses->locations[(*r)->dest]))
                {
                    struct routenode *p=*r;
                    if (ses->mesvar[MSG_ROUTE])
                    {
                        tintin_printf(ses, "#Ok. There is no longer a route from {%s~-1~} to {%s~-1~}.",
                            ses->locations[i],
                            ses->locations[p->dest]);
                    }
                    found=true;
                    *r=(*r)->next;
                    SFREE(p->path);
                    SFREE(p->cond);
                    TFREE(p, struct routenode);
                }
                else
                    r=&((*r)->next);
            }
    if (found)
        kill_unused_locations(ses);
    else if (ses->mesvar[MSG_ROUTE])
        tintin_printf(ses, "#THAT ROUTE (%s) IS NOT DEFINED.", b);
}

#define INF 0x7FFFFFFF

/**********************/
/* the #goto command  */
/**********************/
void goto_command(const char *arg, struct session *ses)
{
    char A[BUFFER_SIZE], B[BUFFER_SIZE], tmp[BUFFER_SIZE], cond[BUFFER_SIZE];
    int a, b, i, j, s;
    int d[MAX_LOCATIONS], ok[MAX_LOCATIONS], way[MAX_LOCATIONS];
    char *path[MAX_LOCATIONS], *locs[MAX_LOCATIONS];

    arg=get_arg(arg, A, 0, ses);
    arg=get_arg(arg, B, 1, ses);

    if ((!*A)||(!*B))
    {
        tintin_eprintf(ses, "#SYNTAX: #goto <from> <to>");
        return;
    }

    for (a=0;a<MAX_LOCATIONS;a++)
        if (ses->locations[a]&&!strcmp(ses->locations[a], A))
            break;
    if (a==MAX_LOCATIONS)
    {
        tintin_eprintf(ses, "#Location not found: [%s]", A);
        return;
    }
    for (b=0;b<MAX_LOCATIONS;b++)
        if (ses->locations[b]&&!strcmp(ses->locations[b], B))
            break;
    if (b==MAX_LOCATIONS)
    {
        tintin_eprintf(ses, "#Location not found: [%s]", B);
        return;
    }
    for (i=0;i<MAX_LOCATIONS;i++)
    {
        d[i]=INF;
        ok[i]=0;
    }
    d[a]=0;
    do
    {
        s=INF;
        for (j=0;j<MAX_LOCATIONS;j++)
            if (!ok[j]&&(d[j]<s))
                s=d[i=j];
        if (s==INF)
        {
            tintin_eprintf(ses, "#No route from %s to %s!", A, B);
            return;
        }
        ok[i]=1;
        for (struct routenode *r=ses->routes[i];r;r=r->next)
            if (d[r->dest]>s+r->distance)
            {
                if (!*(r->cond))
                    goto good;
                substitute_vars(r->cond, tmp);
                substitute_myvars(tmp, cond, ses);
                if (eval_expression(cond, ses))
                {
                good:
                    d[r->dest]=s+r->distance;
                    way[r->dest]=i;
                }
            }
    } while (!ok[b]);
    j=0;
    for (i=b;i!=a;i=way[i])
        d[j++]=i;
    for (d[i=j]=a;i>0;i--)
    {
        locs[i]=mystrdup(ses->locations[d[i]]);
        for (struct routenode *r=ses->routes[d[i]];r;r=r->next)
            if (r->dest==d[i-1])
                path[i]=mystrdup(r->path);
    }

    /*
       we need to copy all used route data (paths and location names)
       because of ugly bad users who can use #unroute in the middle
       of a #go command
    */
    locs[0]=mystrdup(ses->locations[b]);
    for (i=j;i>0;i--)
    {
        if (ses->mesvar[MSG_GOTO])
        {
            tintin_printf(ses, "#going from %s to %s",
                locs[i],
                locs[i-1]);
        }
        parse_input(path[i], true, ses);
    }
    for (i=j;i>=0;i--)
        SFREE(locs[i]);
    for (i=j;i>0;i--)
        SFREE(path[i]);
    set_variable("loc", B, ses);
}

/************************/
/* the #dogoto command  */
/************************/
void dogoto_command(const char *arg, struct session *ses)
{
    char A[BUFFER_SIZE], B[BUFFER_SIZE],
        distvar[BUFFER_SIZE], locvar[BUFFER_SIZE], pathvar[BUFFER_SIZE];
    char left[BUFFER_SIZE], right[BUFFER_SIZE], tmp[BUFFER_SIZE], cond[BUFFER_SIZE];
    int a, b, i, j, s;
    int d[MAX_LOCATIONS], ok[MAX_LOCATIONS], way[MAX_LOCATIONS];
    char path[BUFFER_SIZE], *pptr;

    arg=get_arg(arg, A, 0, ses);
    arg=get_arg(arg, B, 0, ses);
    arg=get_arg(arg, distvar, 0, ses);
    arg=get_arg(arg, locvar, 0, ses);
    arg=get_arg(arg, pathvar, 0, ses);

    if ((!*A)||(!*B))
    {
        tintin_eprintf(ses, "#SYNTAX: #dogoto <from> <to> [<distvar> [<locvar> [<pathvar>]]] [#else ...]");
        return;
    }
    bool flag=*distvar||*locvar||*pathvar;

    for (a=0;a<MAX_LOCATIONS;a++)
        if (ses->locations[a]&&!strcmp(ses->locations[a], A))
            break;
    if (a==MAX_LOCATIONS)
        goto not_found;
    for (b=0;b<MAX_LOCATIONS;b++)
        if (ses->locations[b]&&!strcmp(ses->locations[b], B))
            break;
    if (b==MAX_LOCATIONS)
        goto not_found;
    for (i=0;i<MAX_LOCATIONS;i++)
    {
        d[i]=INF;
        ok[i]=0;
    }
    d[a]=0;
    do
    {
        s=INF;
        for (j=0;j<MAX_LOCATIONS;j++)
            if (!ok[j]&&(d[j]<s))
                s=d[i=j];
        if (s==INF)
            goto not_found;
        ok[i]=1;
        for (struct routenode *r=ses->routes[i];r;r=r->next)
            if (d[r->dest]>s+r->distance)
            {
                if (!*(r->cond))
                    goto good;
                substitute_vars(r->cond, tmp);
                substitute_myvars(tmp, cond, ses);
                if (eval_expression(cond, ses))
                {
                good:
                    d[r->dest]=s+r->distance;
                    way[r->dest]=i;
                }
            }
    } while (!ok[b]);
    sprintf(tmp, "%d", d[b]);
    if (*distvar)
        set_variable(distvar, tmp, ses);
    j=0;
    for (i=b;i!=a;i=way[i])
        d[j++]=i;
    d[j]=a;
    pptr=path;
    for (i=j;i>=0;i--)
        pptr+=snprintf(pptr, path-pptr+BUFFER_SIZE, " %s", ses->locations[d[i]]);
    pptr=path+(pptr!=path);
    if (*locvar)
        set_variable(locvar, pptr, ses);
    pptr=path;
    for (i=j;i>0;i--)
    {
        for (struct routenode *r=ses->routes[d[i]];r;r=r->next)
            if (r->dest==d[i-1])
            {
                if (flag)
                    pptr+=snprintf(pptr,
                        path-pptr+BUFFER_SIZE,
                        " {%s}",
                        r->path);
                else
                {
                    tintin_printf(ses, "%-10s>%-10s {%s}",
                        ses->locations[d[i]],
                        ses->locations[d[i-1]],
                        r->path);
                }
            }
    }
    pptr=path+(pptr!=path);
    if (*pathvar)
        set_variable(pathvar, pptr, ses);
    return;

not_found:
    arg=get_arg_in_braces(arg, left, 0);
    if (*left == tintin_char)
    {
        if (is_abrev(left + 1, "else"))
        {
            get_arg_in_braces(arg, right, 1);
            parse_input(right, true, ses);
            return;
        }
        if (is_abrev(left + 1, "elif"))
        {
            if_command(arg, ses);
            return;
        }
    }
    if (*left)
        tintin_eprintf(ses, "#ERROR: cruft after #dogoto: {%s}", left);
    if (!flag)
        tintin_printf(ses, "No paths from %s to %s found.", A, B);
}
