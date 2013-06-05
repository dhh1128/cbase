/* HEADER */

/**
 * @file MUBufferedFD.h
 */

#ifndef _MUBUFFERED_H__
#define _MUBUFFERED_H__

extern int MUBufferedFDInit(mbuffd_t *,int);
extern ssize_t MUBufferedFDRead(mbuffd_t *,char *,size_t);
extern ssize_t MUBufferedFDReadLine(mbuffd_t *,char *,int);
extern int MUBufferedFDMStringReadLine(mbuffd_t *,mstring_t *);

#endif  /*  _MUBUFFERED_H__ */
/* END MUBufferedFD.H */
