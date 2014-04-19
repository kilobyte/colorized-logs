#include "tintin.h"
#include <sys/stat.h>
#include "protos/print.h"

#ifdef HAVE_GNUTLS
static gnutls_certificate_credentials_t ssl_cred=0;
static int ssl_check_cert(gnutls_session_t sslses, char *host, struct session *oldses);

gnutls_session_t ssl_negotiate(int sock, char *host, struct session *oldses)
{
    gnutls_session_t sslses;
    int ret;

    if (!ssl_cred)
    {
        gnutls_global_init();
        gnutls_certificate_allocate_credentials(&ssl_cred);
    }
    gnutls_init(&sslses, GNUTLS_CLIENT);
    gnutls_set_default_priority(sslses);
    gnutls_credentials_set(sslses, GNUTLS_CRD_CERTIFICATE, ssl_cred);
    gnutls_transport_set_ptr(sslses, (gnutls_transport_ptr_t)(intptr_t)sock);
    do
    {
        ret=gnutls_handshake(sslses);
    } while (ret==GNUTLS_E_AGAIN || ret==GNUTLS_E_INTERRUPTED);
    if (ret)
    {
        tintin_eprintf(oldses, "#SSL handshake failed: %s", gnutls_strerror(ret));
        gnutls_deinit(sslses);
        return 0;
    }
    if (!ssl_check_cert(sslses, host, oldses))
    {
        gnutls_deinit(sslses);
        return 0;
    }
    return sslses;
}


static int cert_file(char *name, char *respath)
{
    char fname[BUFFER_SIZE], *fn, *home;

    if (!*name || *name=='.')   // no valid hostname starts with a dot
        return 0;
    fn=fname;
    while(1)
    {
        if (!*name)
            break;
#ifdef WIN32
        else if (*name==':')
        {
            name++;
            *fn++='.';
        }
#endif
        else if (is7alnum(*name) || *name=='-' || *name=='.' || *name=='_')
            *fn++=*name++;
        else
            return 0;
    }
    if (*(fn-1)=='.')   // no valid hostname ends with a dot, either
        return 0;
    *fn=0;
    if (!(home=getenv("HOME")))
        home=".";
    snprintf(respath, BUFFER_SIZE, "%s/%s/%s/%s.crt", home, CONFIG_DIR, CERT_DIR, fname);
    return 1;
}


static void load_cert(gnutls_x509_crt_t *cert, char *name)
{
#   define BIGBUFSIZE 65536
    char buf[BIGBUFSIZE];
    FILE *f;
    gnutls_datum_t bptr;

    if (!cert_file(name, buf))
        return;
    if (!(f=fopen(buf, "r")))
        return;
    bptr.size=fread(buf, 1, BIGBUFSIZE, f);
    bptr.data=(unsigned char*)buf;
    fclose(f);

    gnutls_x509_crt_init(cert);
    if (gnutls_x509_crt_import(*cert, &bptr, GNUTLS_X509_FMT_PEM))
    {
        gnutls_x509_crt_deinit(*cert);
        *cert=0;
    }
}


static void save_cert(gnutls_x509_crt_t cert, char *name, int new, struct session *oldses)
{
    char *home, fname[BUFFER_SIZE], buf[BIGBUFSIZE];
    FILE *f;
    size_t len;

    len=BIGBUFSIZE;
    if (gnutls_x509_crt_export(cert, GNUTLS_X509_FMT_PEM, buf, &len))
        return;
    if (!(home=getenv("HOME")))
        home=".";
    snprintf(fname, BUFFER_SIZE, "%s/%s", home, CONFIG_DIR);
    if (mkdir(fname, 0777) && errno!=EEXIST)
    {
        tintin_eprintf(oldses, "#Cannot create config dir (%s): %s", fname, strerror(errno));
        return;
    }
    snprintf(fname, BUFFER_SIZE, "%s/%s/%s", home, CONFIG_DIR, CERT_DIR);
    mkdir(fname, 0755);
    if (mkdir(fname, 0755) && errno!=EEXIST)
    {
        tintin_eprintf(oldses, "#Cannot create certs dir (%s): %s", fname, strerror(errno));
        return;
    }
    if (!cert_file(name, fname))
        return;
    if (new)
        tintin_printf(oldses, "#It is the first time you connect to this server.");
    tintin_printf(oldses, "#Saving server certificate to %s", fname);
    if (!(f=fopen(fname, "w")))
    {
        tintin_eprintf(oldses, "#Save failed: %s", strerror(errno));
        return;
    }
    if (fwrite(buf, 1, len, f)!=len)
    {
        tintin_eprintf(oldses, "#Save failed: %s", strerror(errno));
        fclose(f);
        unlink(fname);
        return;
    }
    if (fclose(f))
        tintin_eprintf(oldses, "#Save failed: %s", strerror(errno));
}


static int diff_certs(gnutls_x509_crt_t c1, gnutls_x509_crt_t c2)
{
    char buf1[BIGBUFSIZE], buf2[BIGBUFSIZE];
    size_t len1, len2;

    len1=len2=BIGBUFSIZE;
    if (gnutls_x509_crt_export(c1, GNUTLS_X509_FMT_DER, buf1, &len1))
        return 1;
    if (gnutls_x509_crt_export(c2, GNUTLS_X509_FMT_DER, buf2, &len2))
        return 1;
    if (len1!=len2)
        return 1;
    return memcmp(buf1, buf2, len1);
}


static int ssl_check_cert(gnutls_session_t sslses, char *host, struct session *oldses)
{
    char fname[BUFFER_SIZE], buf2[BUFFER_SIZE], *bptr;
    time_t t;
    gnutls_x509_crt_t cert, oldcert;
    const gnutls_datum_t *cert_list;
    unsigned int cert_list_size;
    char *err=0;

    oldcert=0;
    load_cert(&oldcert, host);

    if (gnutls_certificate_type_get(sslses)!=GNUTLS_CRT_X509)
    {
        err="server doesn't use x509 -> no key retention.";
        goto nocert;
    }

    if (!(cert_list = gnutls_certificate_get_peers(sslses, &cert_list_size)))
    {
        err="server has no x509 certificate -> no key retention.";
        goto nocert;
    }

    gnutls_x509_crt_init(&cert);
    if (gnutls_x509_crt_import(cert, &cert_list[0], GNUTLS_X509_FMT_DER)<0)
    {
        err="server's certificate is invalid.";
        goto badcert;
    }

    t=time(0);
    if (gnutls_x509_crt_get_activation_time(cert)>t)
    {
        snprintf(buf2, BUFFER_SIZE, "%s", ctime(&t));
        if ((bptr=strchr(buf2, '\n')))
            *bptr=0;
        snprintf(fname, BUFFER_SIZE, "certificate activation time is in the future (%s).",
            buf2);
        err=fname;
    }

    if (gnutls_x509_crt_get_expiration_time(cert)<t)
    {
        snprintf(buf2, BUFFER_SIZE, "%s", ctime(&t));
        if ((bptr=strchr(buf2, '\n')))
            *bptr=0;
        snprintf(fname, BUFFER_SIZE, "certificate has expired (on %s).",
            buf2);
        err=fname;
    }

    if (!oldcert)
        save_cert(cert, host, 1, oldses);
    else if (diff_certs(cert, oldcert))
    {
        t-=gnutls_x509_crt_get_expiration_time(oldcert);
        if (err)
        {
            snprintf(buf2, BUFFER_SIZE, "certificate mismatch, and new %s",
                     err);
            err=buf2;
        }
        else if (t<-7*24*3600)
            err = "the server certificate is different from the saved one.";
        else
        {
            tintin_printf(oldses, (t>0)?
                "#SSL notice: server certificate has changed, but the old one was expired.":
                "#SSL notice: server certificate has changed, but the old one was about to expire.");
            /* Replace the old cert */
            save_cert(cert, host, 0, oldses);
            gnutls_x509_crt_deinit(oldcert);
            oldcert=0;
        }
    }
    else
    {
        /* All ok */
        gnutls_x509_crt_deinit(oldcert);
        oldcert=0;
    }


badcert:
    gnutls_x509_crt_deinit(cert);

nocert:
    if (oldcert)
        gnutls_x509_crt_deinit(oldcert);
    if (!err)
        return 1;
    if (oldcert)
    {
        tintin_eprintf(oldses, "#SSL error: %s", err);
        tintin_eprintf(oldses, "############################################################");
        tintin_eprintf(oldses, "##################### SECURITY ALERT #######################");
        tintin_eprintf(oldses, "############################################################");
        tintin_eprintf(oldses, "# It's likely you're under a Man-in-the-Middle attack.     #");
        tintin_eprintf(oldses, "# Someone may be trying to intercept your connection.      #");
        tintin_eprintf(oldses, "#                                                          #");
        tintin_eprintf(oldses, "# Another explanation is that the server's settings were   #");
        tintin_eprintf(oldses, "# changed in an unexpected way.  You can choose to avoid   #");
        tintin_eprintf(oldses, "# connecting, investigate this or blindly trust that the   #");
        tintin_eprintf(oldses, "# server is kosher.  To continue, please delete the file:  #");
        if (cert_file(host, fname))
            tintin_eprintf(oldses, "# %-57s#", fname);
        else
            ; /* can't happen */
        tintin_eprintf(oldses, "############################################################");
        tintin_eprintf(oldses, "#Aborting connection!");
        return 0;
    }
    else
    {
        tintin_printf(oldses, "#SSL warning: %s", err);
        tintin_printf(oldses, "#You may be vulnerable to Man-in-the-Middle attacks.");
        return 2;
    }
}

#endif
