/*
*/

/**
 * @file moab-openssl.h
 *
 * Moab OpenSSL
 */

#include <openssl/lhash.h>
#include <openssl/bn.h>
#define USE_SOCKETS
#include "moab-apps.h"
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include "moab-s_apps.h"
 
#ifdef WINDOWS
#include <conio.h>
#endif
 
#if (defined(VMS) && __VMS_VER < 70000000)
/* FIONBIO used as a switch to enable ioctl, and that isn't in VMS < 7.0 */
#undef FIONBIO
#endif
 
#ifndef NO_RSA
static RSA MS_CALLBACK *tmp_rsa_cb(SSL *s, int is_export, int keylength);
#endif
static int sv_body(char *hostname, int s, unsigned char *context);
static int www_body(char *hostname, int s, unsigned char *context);
static void close_accept_socket(void );
static void sv_usage(void);
static int init_ssl_connection(SSL *s);
static void print_stats(BIO *bp,SSL_CTX *ctx);
#ifndef NO_DH
static DH *load_dh_param(char *dhfile);
static DH *get_dh512(void);
#endif
#ifdef MONOLITH
static void s_server_init(void);
#endif
 
#ifndef S_ISDIR
# if defined(_S_IFMT) && defined(_S_IFDIR)
#  define S_ISDIR(a)    (((a) & _S_IFMT) == _S_IFDIR)
# else
#  define S_ISDIR(a)    (((a) & S_IFMT) == S_IFDIR)
# endif
#endif 
 
#ifndef NO_DH
static unsigned char dh512_p[]={
        0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
        0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
        0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
        0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
        0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
        0x47,0x74,0xE8,0x33,
        };
static unsigned char dh512_g[]={
        0x02,
        };
 
static DH *get_dh512(void)
        {
        DH *dh=NULL;
 
        if ((dh=DH_new()) == NULL) return(NULL);
        dh->p=BN_bin2bn(dh512_p,sizeof(dh512_p),NULL);
        dh->g=BN_bin2bn(dh512_g,sizeof(dh512_g),NULL);
        if ((dh->p == NULL) || (dh->g == NULL))
                return(NULL);
        return(dh);
        }
#endif
 
/* static int load_CA(SSL_CTX *ctx, char *file);*/
 
#undef BUFSIZZ
#define BUFSIZZ 16*1024
static int bufsize=BUFSIZZ;
static int accept_socket= -1; 
 
#define TEST_CERT       "server.pem"
#undef PROG
#define PROG            s_server_main
 
extern int verify_depth;
 
static char *cipher=NULL;
static int s_server_verify=SSL_VERIFY_NONE;
static int s_server_session_id_context = 1; /* anything will do */
static char *s_cert_file=TEST_CERT,*s_key_file=NULL;
static char *s_dcert_file=NULL,*s_dkey_file=NULL;
#ifdef FIONBIO
static int s_nbio=0;
#endif
static int s_nbio_test=0;
int s_crlf=0;
static SSL_CTX *ctx=NULL;
static int www=0;
 
static BIO *bio_s_out=NULL;
static int s_debug=0;
static int s_quiet=0;
 
static int hack=0;
 
#ifdef MONOLITH
static void s_server_init(void)
        {
        accept_socket=-1;
        cipher=NULL;
        s_server_verify=SSL_VERIFY_NONE;
        s_dcert_file=NULL;
        s_dkey_file=NULL;
        s_cert_file=TEST_CERT;
        s_key_file=NULL;
#ifdef FIONBIO 
        s_nbio=0;
#endif
        s_nbio_test=0;
        ctx=NULL;
        www=0;
 
        bio_s_out=NULL;
        s_debug=0;
        s_quiet=0;
        hack=0;
        }
#endif

