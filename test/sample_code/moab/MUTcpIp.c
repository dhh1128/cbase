/* HEADER */

/**
 * @file MUTcpIp.c
 * 
 * Contains various functions for TCP or IP 
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Check if a string is a IPv4 Mac Address.
 *
 */

mbool_t MUStringIsMacAddress(

  char *String)

  {
  char tmpC;

  int rc;

  if (String == NULL)
    {
    return(FALSE);
    }

  rc = sscanf(String,"%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC,
    &tmpC);

  if (rc == 12)
    {
    return(TRUE);
    }

  return(FALSE);
  }  /* END MUStringIsMacAddress() */


/**
 *
 *
 * @param HostName (I)
 * @param IPAddr (O) [alloc]
 */

int MUGetIPAddr(

  char   *HostName,  /* I */
  char  **IPAddr)    /* O (alloc) */

  {
#ifdef __MIPV6
  struct sockaddr_in6 s_sockaddr;
#else
  struct sockaddr_in  s_sockaddr;
#endif

  struct addrinfo *info;
  char tmpAddr[MMAX_LINE];

  if ((IPAddr == NULL) ||
      (HostName == NULL))
    {
    return(FAILURE);
    }

  *IPAddr = NULL;

  memset(&s_sockaddr,0,sizeof(s_sockaddr));

  if (getaddrinfo(HostName,NULL,NULL,&info) != 0)
    {
    /* can't lookup hostname */

    return(FAILURE);
    }
  memcpy(&s_sockaddr,info->ai_addr,info->ai_addrlen);

  freeaddrinfo(info);

#ifdef __MIPV6
  inet_ntop(AF_INET6,&s_sockaddr.sin6_addr,tmpAddr,sizeof(tmpAddr));
#else
  inet_ntop(AF_INET,&s_sockaddr.sin_addr,tmpAddr,sizeof(tmpAddr));
#endif

  MUStrDup(IPAddr,tmpAddr);

  return(SUCCESS);
  }  /* END MUGetIPAddr() */



