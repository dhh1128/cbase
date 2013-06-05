/* HEADER */

/**
 * @file MFile.c
 *
 * Moab Files
 */

/* Contains:                                          *
 *                                                    *
 *  char *MFULoad(FileName,BlockSize,AccessMode,BlockCount,EMsg,SC) *
 *  char *MFULoadNoCache(FileName,BlockSize,BlockCount,EMsg,SC,BufSize) *
 *  int __MFUGetCachedFile(FileName,Buffer,BufSize)   *
 *  int __MFUCacheFile(FileName,MTime,Buffer,BufSize) *
 *                                                    */


#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"


mulong    *STime;  /* pointer to external MSched.Time */


mfcache_t  MFileCache[MMAX_FILECACHE];   
mfindex_t  MIndexState[MMAX_FILECACHE];

/* local prototypes */

int __MFUGetCachedFile(const char *,char **,int *);
int __MFUCacheFile(const char *,mulong,char *,int); 



/**
 *
 *
 * @param SysTime (I)
 */

int MFUCacheInitialize(

  mulong *SysTime) /* I */

  {
  int index;

  const char *FName = "MFUCacheInitialize";

  MDB(5,fCORE) MLog("%s(SysTime)\n",
    FName);

  STime = SysTime;

  for (index = 0;index < MMAX_FILECACHE;index++)
    {
    if (MFileCache[index].Buffer != NULL)
      free(MFileCache[index].Buffer);
    }  /* END for (index) */

  memset(MFileCache,0,sizeof(MFileCache));
  memset(MIndexState,0,sizeof(MIndexState));

  return(SUCCESS);
  }  /* END MFUCacheInitialize() */






/**
 * This function will load a file into Moab's memory and return a pointer to the
 * loaded file's buffer. Note that if this file has already been loaded before, it
 * will be retrieved from an internal cache. Because this buffer is managed in
 * a special cache, you should NOT free the allocated memory returned by this function!
 * For cacheless loading (buffers can be freed safely) use MFULoadNoCache().
 *
 * When the AccessMode is not "read" then the buffer is copied and can be safely freed
 *
 * @see MFULoadNoCache() - peer
 *
 * @param FileName (I) The abs/relative path of the file to open.
 * @param BlockSize (I) Size of each file block in bytes.
 * @param AccessMode (I) In which way we plan to access the file.
 * @param BlockCount (O,optional) How many blocks of the file to read in.
 * @param EMsg (O) Error message. [optional,minsize=MMAX_LINE]
 * @param SC (O) Status code. [optional]
 *
 * @return Returns an alloc'd pointer to a buffer containing the contents of the file. DO NOT FREE THIS
 * POINTER! Returns NULL if file is empty or error occurs!
 */

char *MFULoad(

  const char           *FileName,    /* I */
  int                   BlockSize,   /* I */
  enum MAccessModeEnum  AccessMode,  /* I */
  int                  *BlockCount,  /* O (optional) */
  char                 *EMsg,        /* O (optional,minsize=MMAX_LINE) */
  int                  *SC)          /* O (optional) */

  {
  char        *ptr;
  char        *buf;
  int          BufSize;

  const char *FName = "MFULoad";

  MDB(5,fCORE) MLog("%s(%s,%d,%s,BlockCount,EMsg,SC)\n",
    FName,
    (FileName != NULL) ? FileName : "NULL",
    BlockSize,
    (AccessMode == macmRead) ? "READ" : "WRITE");

  if (SC != NULL)
    *SC = mscNoError;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((FileName == NULL) || (FileName[0] == '\0'))
    {
    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  /* check if file is cached */

  if (__MFUGetCachedFile(FileName,&buf,&BufSize) == SUCCESS)
    {
    if (BlockCount != NULL)
      *BlockCount = BufSize / BlockSize;

    if (AccessMode == macmRead)
      {
      /* use cached data */

      return(buf);
      }

    /* copy cached data */

    if ((ptr = (char *)MUMalloc(BufSize + 1)) == NULL)
      {
      MOSSyslog(LOG_ERR,"cannot MUMalloc buffer for file '%s', errno: %d (%s)",
        FileName,
        errno,
        strerror(errno));

      MDB(2,fCORE) MLog("ERROR:    cannot MUMalloc file buffer for file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));

      if (SC != NULL)
        *SC = mscNoMemory;

      return(NULL);
      }

    /* fill MUMalloc'd buffer */

    memcpy(ptr,buf,BufSize + 1);

    MDB(9,fCORE) MLog("INFO:     cached file data copied from %p to %p\n",
      buf,
      ptr);

    return(ptr);
    }     /* END if (__MFUGetCachedFile() == SUCCESS) */

  /* no cached data located, reload data */

  ptr = MFULoadNoCache(FileName,BlockSize,BlockCount,EMsg,SC,&BufSize);

  if (ptr == NULL)
    {
    /* cannot load file */

    return(NULL);
    }

  if (__MFUCacheFile(FileName,0,ptr,BufSize) == FAILURE)
    {
    if (AccessMode == macmRead)
      {
      if (ptr != NULL)
        {
        free(ptr);

        ptr = NULL;
        }

      if (SC != NULL)
        *SC = mscSysFailure;

      return(NULL);
      }

    return(ptr); 
    }  /* END if (__MFUCacheFile(FileName,0,ptr,BufSize) == FAILURE) */

  if (AccessMode == macmRead)
    {
    /* use cached data */

    free(ptr);

    if (__MFUGetCachedFile(FileName,&buf,&BufSize) == SUCCESS)
      {
      if (BlockCount != NULL)
        *BlockCount = BufSize / BlockSize;

      return(buf);
      }

    if (SC != NULL)
      *SC = mscNoEnt;

    return(NULL);
    }    /* END if (AccessMode == macmRead) */

  return(ptr);
  }  /* END MFULoad() */





/**
 * Opens the given file and allocates memory for the contents of the file.
 * May read either stdin or regular file.
 *
 * @see MFULoad()
 *
 * @param FileName (I)
 * @param BlockSize (I)
 * @param BlockCount (O)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 * @param BufSize (O)
 * @return A pointer to the newly allocated memory. The pointer may be NULL if a failure
 * occurs. ANY ALLOCATED MEM MUST BE FREED BY CALLER!
 */

char *MFULoadNoCache(

  const char *FileName,
  int         BlockSize,
  int        *BlockCount,
  char       *EMsg,
  int        *SC,
  int        *BufSize)

  {
  FILE        *dfp = NULL;
  char        *ptr; /* will be the return value */
  int          count;
  int          ReadCount;
  int          FSize;
  int          tmpBufSize;

  int          rc;

  const char *FName = "MFULoadNoCache";

  MDB(5,fCORE) MLog("%s(%s,%d,BlockCount,EMsg,SC,BufSize)\n",
    FName,
    (FileName != NULL) ? FileName : "NULL",
    BlockSize);

  if (SC != NULL)
    *SC = mscNoError;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (BufSize != NULL)
    *BufSize = 0;

  if ((FileName == NULL) || (FileName[0] == '\0'))
    {
    if (SC != NULL)
      *SC = mscBadParam;

    if (EMsg != NULL)
      strcpy(EMsg,"file not specified");

    return(NULL);
    }

  /* initialize block size to legal value if not previously set */

  if (BlockSize < 1)
    BlockSize = 1;

  if (FileName != NULL)
    {
    struct stat sbuf;

    if (stat(FileName,&sbuf) == -1)
      {
      MDB(2,fCORE) MLog("INFO:     cannot stat file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));

      if (SC != NULL)
        {
        if (errno == ENOENT)
          *SC = mscNoEnt;
        else
          *SC = mscSysFailure;
        }

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot stat file '%s', errno: %d (%s)",
          FileName,
          errno,
          strerror(errno));
        }

      return(NULL);
      }  /* END if (stat(FileName,&sbuf) == -1) */

    if ((sbuf.st_mode & S_IFREG) == FALSE)
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot read '%s', object is not file",
          FileName);
        }

      return(NULL);
      }

    if ((dfp = fopen(FileName,"r")) == NULL)
      {
      MDB(2,fCORE) MLog("INFO:     cannot open file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));

      if (SC != NULL)
        *SC = mscSysFailure;

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot open file '%s', errno: %d (%s)",
          FileName,
          errno,
          strerror(errno));
        }

      return(NULL);
      }

    FSize = sbuf.st_size / BlockSize;
    }  /* END if (FileName != NULL) */
  else
    {
    dfp = stdin;

    FSize = MMAX_FBUFFER / BlockSize;
    }  /* END else (FileName != NULL) */

  tmpBufSize = FSize * BlockSize;

  MDB(5,fCORE) MLog("INFO:     new file '%s' opened (%d bytes)\n",
    FileName,
    tmpBufSize);

  if ((ptr = (char *)MUMalloc(tmpBufSize + 1)) == NULL)
    {
    MOSSyslog(LOG_ERR,"cannot malloc %d byte buffer for file '%s', errno: %d (%s)",
      tmpBufSize,
      FileName,
      errno,
      strerror(errno));

    MDB(2,fCORE) MLog("ERROR:    cannot malloc file buffer for file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    if (dfp != stdin)
      fclose(dfp);

    if (SC != NULL)
      *SC = mscNoMemory;

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot alloc buffer for file '%s', errno: %d (%s)",
        FileName,
        errno,
        strerror(errno));
      }

    return(NULL);
    }

  ReadCount = FSize;

  count = 0;
  rc    = 1;

  while (rc > 0)
    {
    rc = (int)fread(&ptr[count],ReadCount,BlockSize,dfp);

    if (rc < 0)
      {
      MOSSyslog(LOG_ERR,"cannot read file '%s', errno: %d (%s)",
        FileName,
        errno,
        strerror(errno));

      MDB(2,fCORE) MLog("ERROR:    cannot read file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));

      free(ptr);

      if (dfp != stdin)
        fclose(dfp);

      if (SC != NULL)
        *SC = mscSysFailure;

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot read file '%s', errno: %d (%s)",
          FileName,
          errno,
          strerror(errno));
        }

      return(NULL);
      }  /* END if (rc < 0) */

    count += rc;

    ReadCount -= rc;

    if (ReadCount <= 0)
      break;

    if ((rc > 0) && (FileName != NULL))
      break;

    MDB(5,fSTRUCT) MLog("INFO:     %d:%d of %d bytes read from file %s, continuing...\n",
      rc,
      count - rc,
      FSize,
      (FileName != NULL) ? FileName : "NULL");

    if (dfp == stdin)
      {
      /* sleep to slow polling on stdin */

      MUSleep(10000,TRUE);
      }
    }    /* END while (rc >= 0) */

  if (dfp != stdin)
    fclose(dfp);

  if (BlockCount != NULL)
    *BlockCount = count;

  ptr[tmpBufSize] = '\0';

  if (BufSize != NULL)
    *BufSize = tmpBufSize;

  /* SUCCESS */

  return(ptr);
  }  /* END MFULoadNoCache() */
 



/**
 *
 *
 * @param File (I)
 * @param OBuf (O) [alloc]
 * @param OBufSize (O)
 * @param EMsg (O) [minsize=MMAX_LINE]
 */

int MUPackFileNoCache(

  char *File,       /* I */
  char **OBuf,      /* O (alloc) */
  int   *OBufSize,  /* O */
  char  *EMsg)      /* O (minsize=MMAX_LINE) */

  {
  char *FBuffer = NULL;
  int   FBufferSize = 0;

  char *tmpBuffer = NULL;
  int   tmpBufSize = MMAX_LINE;
  int   count;

  if ((File == NULL) ||
      (OBuf == NULL) ||
      (OBufSize == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  *OBuf = NULL;
  *OBufSize = 0;

  /* verify that the file we've been given is indeed valid */
  
  if ((FBuffer = MFULoadNoCache(File,1,&count,NULL,NULL,&FBufferSize)) == NULL)
    {
    /* cannot open file */
  
    MDB(1,fCKPT) MLog("WARNING:  cannot load file '%s'\n",
      File);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"WARNING:  cannot load file '%s'\n",
        File);
      }  

    return(FAILURE);
    }
  
  tmpBuffer = (char *)MUCalloc(1,tmpBufSize * sizeof(char));
    
  /* send entire file */
  
  while (MUStringPack(FBuffer,tmpBuffer,tmpBufSize) == FAILURE)
    {
    tmpBufSize += MMAX_BUFFER;

    tmpBuffer = (char *)realloc(tmpBuffer,tmpBufSize);

    if (tmpBuffer == NULL)
      {
      MDB(1,fCKPT) MLog("ALERT:    memory realloc failure in MMPeerCopy\n");

      return(FAILURE);
      }
    }

  MUFree(&FBuffer);

  *OBuf = tmpBuffer;
  *OBufSize = tmpBufSize;

  return(SUCCESS);
  }  /* END MUPackFileNoCache() */

/**
 * Open a file and read its data into an allocated buffer
 * and return the data to the caller.
 *
 * Call MUST free the memory in the buffer
 *
 * @param    FileName     (I)  file name
 * @param    BufferP      (I/O) pointer to char * pointer to put buffer ptr 
 *                        ON SUCCESS return, caller must free this buffer
 *                        otherwise, no buffer is returned
 * @param    EMsg         (O)
 * @param    SC           (O)
 */


int MFULoadAllocReadBuffer(

  char       *FileName,
  char      **BufferP, 
  char       *EMsg,
  int        *SC)

  {
  FILE        *dfp = NULL;
  int          count;
  int          ReadCount;
  int          tmpBufSize;

  struct stat  sbuf;

  const char *FName = "MFULoadAllocReadBuffer";

  MDB(5,fCORE) MLog("%s(%s,Buffer,EMsg,SC)\n",
    FName,
    (FileName != NULL) ? FileName : "NULL");

  if (SC != NULL)
    *SC = mscNoError;

  if (EMsg != NULL)
    EMsg[0] = '\0';


  if ((FileName == NULL) || (FileName[0] == '\0') || (BufferP == NULL))
    {
    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  if (stat(FileName,&sbuf) == -1)
    {
    MDB(2,fCORE) MLog("INFO:     cannot stat file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    if (SC != NULL)
      {
      if (errno == ENOENT)
        *SC = mscNoEnt;
      else
        *SC = mscSysFailure;
      }

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot stat file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));
      }

    return(FAILURE);
    }

  if ((sbuf.st_mode & S_IFREG) == FALSE)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot read '%s', object is not file",
        FileName);
      }

    return(FAILURE);
    }

  ReadCount = sbuf.st_size;

  if (ReadCount <= 0)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"file '%s' is empty\n",
        FileName);
      }

    return(FAILURE);
    }

  if ((dfp = fopen(FileName,"r")) == NULL)
    {
    MDB(2,fCORE) MLog("INFO:     cannot open file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    if (SC != NULL)
      *SC = mscSysFailure;

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot open file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));
      }

    return(FAILURE);
    }

  /* Allocate a buffer big enough to read the data */
  *BufferP = (char*) MUCalloc(1,ReadCount + 100);

  ReadCount = sbuf.st_size;

  tmpBufSize = ReadCount;

  MDB(5,fCORE) MLog("INFO:     new file '%s' opened (%d bytes)\n",
    FileName,
    tmpBufSize);

  /* read the data into the buffer */
  count = (int)fread(*BufferP,ReadCount,1,dfp);

  fclose(dfp);

  if (count < 0)
    {
    MOSSyslog(LOG_ERR,"cannot read file '%s', errno: %d (%s)",
      FileName,
      errno,
      strerror(errno));

    MDB(2,fCORE) MLog("ERROR:    cannot read file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    fclose(dfp);

    if (SC != NULL)
      *SC = mscSysFailure;

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot read file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));
      }

    /* Free the read buffer on failure*/
    MUFree(BufferP);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MFULoadAllocReadBuffer() */





/**
 * Internal routine to local file contents from cached copy.
 *
 * @param FileName (I)
 * @param Buffer (O)
 * @param BufSize (I)
 */

int __MFUGetCachedFile(

  const char  *FileName,
  char       **Buffer,
  int         *BufSize)

  {
  int    index;

  mulong   mtime;

  const char *FName = "__MFUGetCachedFile";

  MDB(5,fSTRUCT) MLog("%s(%s,Buffer,BufSize)\n",
    FName,
    (FileName != NULL) ? FileName : "NULL");

  if (Buffer != NULL)
    *Buffer = NULL;

  if ((FileName == NULL) || (FileName[0] == '\0') || (Buffer == NULL))
    {
    return(FAILURE);
    }

  for (index = 0;index < MMAX_FILECACHE;index++)
    {
    if (MFileCache[index].FileName[0] == '\0')
      continue;

    if (strcmp(MFileCache[index].FileName,FileName))
      continue;

    /* cache entry located */

    if (MFileCache[index].ProgMTime == 
       ((STime != NULL) ? *STime : 0))
      {
      if (MFileCache[index].Buffer == NULL)
        {
        MDB(5,fSTRUCT) MLog("ALERT:    filecache buffer is empty for file '%s'\n",
          FileName);

        return(FAILURE);
        }

      MDB(5,fSTRUCT) MLog("INFO:     cached file '%s' located in slot %d (%d Bytes)\n",
        FileName,
        index,
        (int)strlen(MFileCache[index].Buffer));
      }

    if (MFUGetInfo(FileName,&mtime,NULL,NULL,NULL) == FAILURE)
      {
      MDB(5,fSTRUCT) MLog("ALERT:    cannot determine modify time for file '%s'\n",
        FileName);

      return(FAILURE);
      }

    if ((mulong)mtime > MFileCache[index].FileMTime)
      {
      MDB(5,fSTRUCT) MLog("INFO:     file '%s' has been modified\n",
        FileName);

      return(FAILURE);
      }

    /* file cache is current */

    *Buffer  = MFileCache[index].Buffer;
    *BufSize = MFileCache[index].BufSize;

    return(SUCCESS);
    }     /* END for (index) */

  /* file cache not found */

  MDB(6,fCORE) MLog("INFO:     file '%s' not cached\n",
    FileName);

  return(FAILURE);
  }  /* END __MFUGetCachedFile() */





/**
 *
 *
 * @param FileName (I)
 * @param MTime (I) [optional]
 * @param Buffer (I)
 * @param BufSize (I)
 */

int __MFUCacheFile(

  const char *FileName,
  mulong      MTime,
  char       *Buffer,
  int         BufSize)

  {
  int index;

  time_t now;

  const char *FName = "__MFUCacheFile";

  MDB(5,fSTRUCT) MLog("%s(%s,Buffer,%d)\n",
    FName,
    FileName,
    BufSize);

  /* look for existing cache entry */

  for (index = 0;index < MMAX_FILECACHE;index++)
    {
    if (MFileCache[index].FileName[0] == '\0')
      continue;

    if (strcmp(MFileCache[index].FileName,FileName))
      continue;

    if (MFileCache[index].Buffer != NULL)
      {
      free(MFileCache[index].Buffer);

      MFileCache[index].Buffer = NULL;
      }

    if ((MFileCache[index].Buffer = (char *)MUMalloc(BufSize + 1)) == NULL)
      {
      MOSSyslog(LOG_ERR,"cannot MUMalloc cache buffer for file '%s', errno: %d (%s)",
        FileName,
        errno,
        strerror(errno));

      MDB(2,fSTRUCT) MLog("ERROR:    cannot MUMalloc file buffer for file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));

      return(FAILURE);
      }

    memcpy(MFileCache[index].Buffer,Buffer,BufSize);

    MFileCache[index].Buffer[BufSize] = '\0';

    MFileCache[index].BufSize         = BufSize;

    MFileCache[index].ProgMTime       = (STime != NULL) ? *STime : 0;

    if (MTime > 0)
      {
      MFileCache[index].FileMTime = MTime;
      }
    else
      {
      time(&now);

      MFileCache[index].FileMTime = now;
      }

    MDB(5,fSTRUCT) MLog("INFO:     file '%s' cached in slot %d (%d bytes)\n",
      FileName,
      index,
      BufSize);

    return(SUCCESS);
    }  /* END for (index) */

  /* create new cache entry */

  for (index = 0;index < MMAX_FILECACHE;index++)
    {
    if (MFileCache[index].FileName[0] == '\0')
      {
      /* MUMalloc space for buffer */

      if ((MFileCache[index].Buffer = (char *)MUMalloc(BufSize + 1)) == NULL)
        {
        MOSSyslog(LOG_ERR,"cannot MUMalloc cache buffer for file '%s', errno: %d (%s)",
          FileName,
          errno,
          strerror(errno));

        MDB(2,fSTRUCT) MLog("ERROR:    cannot MUMalloc cache buffer for file '%s', errno: %d (%s)\n",
          FileName,
          errno,
          strerror(errno));

        return(FAILURE);
        }

      strcpy(MFileCache[index].FileName,FileName);

      memcpy(MFileCache[index].Buffer,Buffer,BufSize);

      MFileCache[index].Buffer[BufSize] = '\0';

      MFileCache[index].BufSize   = BufSize;

      MFileCache[index].ProgMTime = (STime != NULL) ? *STime : 0;

      time(&now);

      MFileCache[index].FileMTime = now;

      MDB(5,fSTRUCT) MLog("INFO:     new file '%s' cached in slot %d (%d bytes)\n",
        FileName,
        index,
        BufSize);

      return(SUCCESS);
      }
    }    /* END for index */

  MDB(5,fSTRUCT) MLog("INFO:  file cache overflow while attempting to cache file '%s'\n",
    FileName);

  return(FAILURE);
  }  /* END __MFUCacheFile() */





/**
 * This function removes the given filename.
 *
 * @param FileName (I)
 */

int MFURemove(

  char *FileName)  /* I */

  {
  if ((FileName == NULL) || (FileName[0] == '\0'))
    {
    return(FAILURE);
    }

  if (remove(FileName) != 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MFURemove() */





/**
 * Get attributes for specified file/directory.
 *
 * @see MFUGetAttributes()
 *
 * @param FileName (I)
 * @param ModifyTime (O) [optional]
 * @param FileSize (O) [optional]
 * @param IsExe (O) [optional]
 * @param IsDir (O) [optional]
 */

int MFUGetInfo(

  const char *FileName,
  mulong     *ModifyTime,
  long       *FileSize,
  mbool_t    *IsExe,
  mbool_t    *IsDir)

  {
  int   rc;
  char *ptr;

  struct stat sbuf;

  if (IsExe != NULL)
    *IsExe = FALSE;

  if (ModifyTime != NULL)
    *ModifyTime = 0;

  if (FileSize != NULL)
    *FileSize = 0;

  if (IsDir != NULL)
    *IsDir = FALSE;

  if ((FileName == NULL) || (FileName[0] == '\0'))
    {
    return(FAILURE);
    }

  /* FORMAT:  <FILENAME>[ <ARG>]... */

  /* NOTE:  mask off, then restore possible args */

  ptr = (char *)strchr(FileName,' ');

  if (ptr != NULL)
    *ptr = '\0';

  rc = stat(FileName,&sbuf);

  if (ptr != NULL)
    *ptr = ' ';

  if (rc == -1)
    {
    MDB(4,fCORE) MLog("INFO:     cannot stat file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    return(FAILURE);
    }

  if (ModifyTime != NULL)
    {
    *ModifyTime = (mulong)sbuf.st_mtime; 

    /* handle out-of-sync clocks between localhost and fileserver */

    if ((STime != NULL) && (*STime > 0))
      *ModifyTime = MIN(*STime,*ModifyTime); 
    }

  if (FileSize != NULL)
    *FileSize = (long)sbuf.st_size;

  if (IsExe != NULL)
    {
    if (sbuf.st_mode & S_IXUSR)
      *IsExe = TRUE;
    else
      *IsExe = FALSE;
    }

  if (IsDir != NULL)
    {
    if (sbuf.st_mode & S_IFDIR)
      *IsDir = TRUE;
    else
      *IsDir = FALSE;
    }

  return(SUCCESS);
  }  /* END MFUGetInfo() */




/**
 *
 *
 * @param FileName (I)
 */

int MFUCacheInvalidate(

  char *FileName) /* I */

  {
  int index;

  const char *FName = "MFUCacheInvalidate";

  MDB(7,fCORE) MLog("%s(%s)\n",
    FName,
    (FileName != NULL) ? FileName : "NULL");

  if ((FileName == NULL) || (FileName[0] == '\0'))
    {
    return(SUCCESS);
    }

  for (index = 0;index < MMAX_FILECACHE;index++)
    {
    if (MFileCache[index].FileName[0] == '\0')
      continue;

    if (strcmp(MFileCache[index].FileName,FileName))
      continue;

    /* located cache entry */

    /* reset timestamp */

    MFileCache[index].ProgMTime = 0;
    MFileCache[index].FileMTime = 0;

    break;
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MFUCacheInvalidate() */






/**
 * Load extended file attributes.
 * 
 * @see MFUGetInfo()
 *
 * @param PathName (I)
 * @param Perm (O) [optional]
 * @param MTime (O) [optional]
 * @param Size (O) [optional]
 * @param UID (O) [optional]
 * @param GID (O) [optional]
 * @param IsPrivate (O) [optional]
 * @param IsExe (O) [optional]
 */

int MFUGetAttributes(

  char    *PathName,  /* I */
  int     *Perm,      /* O (optional) */
  mulong  *MTime,     /* O (optional) */
  long    *Size,      /* O (optional) */
  int     *UID,       /* O (optional) */
  int     *GID,       /* O (optional) */
  mbool_t *IsPrivate, /* O (optional) */
  mbool_t *IsExe)     /* O (optional) */

  {
  struct stat S;

  if (Perm != NULL)
    *Perm = 0;

  if (MTime != NULL)
    *MTime = 0;

  if (Size != NULL) 
    *Size = 0;

  if (UID != NULL)
    *UID = 0;

  if (IsPrivate != NULL)
    *IsPrivate = MBNOTSET;

  if (IsExe != NULL)
    *IsExe = MBNOTSET;

  if (PathName == NULL)
    {
    return(FAILURE);
    }

  if (stat(PathName,&S) == -1)
    {
    return(FAILURE);
    }

  if (MTime != NULL)
    {
    *MTime = (long)S.st_mtime;

    /* handle out-of-sync clocks between localhost and fileserver */

    if ((STime != NULL) && (*STime > 0))
      *MTime = MIN(*MTime,*STime);
    }

  if (Size != NULL)
    *Size = (unsigned long)S.st_size;

  if (Perm != NULL)
    *Perm = (int)S.st_mode;

  if (UID != NULL)
    *UID = (int)S.st_uid;

  if (GID != NULL)
    *GID = (int)S.st_gid;

  if (IsPrivate != NULL)
    {
    if (S.st_mode & (S_IRWXG|S_IRWXO))
      *IsPrivate = FALSE;
    else
      *IsPrivate = TRUE;
    }

  if (IsExe != NULL)
    {
    if (S.st_mode & S_IXUSR)
      *IsExe = TRUE;
    else
      *IsExe = FALSE;
    }

  return(SUCCESS);
  }  /* END MFUGetAttributes() */





/**
 * Create a file with specified permissions. If the file already exists,
 * permissions will only be changed--the contents of the file will be preserved.
 *
 * @see MOMkDir() - create directory with specified name and permissions
 * @see MFUOpen() 
 *
 * @param FileName (I/O) - populate with temp file if empty
 * @param SpoolDir (I) [optional]
 * @param Buf      (I) [optional]
 * @param BufSize  (I)
 * @param Mode     (I) [optional,-1 = not set]
 * @param UID      (I) [optional,-1 = not set]
 * @param GID      (I) [optional,-1 = not set]
 * @param DoAppend (I)
 * @param SC       (O) [optional]
 */

int MFUCreate(

  char   *FileName,
  char   *SpoolDir,
  void   *Buf,
  int     BufSize,
  int     Mode,
  int     UID,
  int     GID,
  mbool_t DoAppend,
  int    *SC)
  
  {
  FILE *fp;
  int   fd;

  const char *FName = "MFUCreate";

  MDB(5,fCORE) MLog("%s(%s,%s,%d,%d,%d,%d,SC)\n",
    FName,
    (SpoolDir != NULL) ? SpoolDir : "NULL",
    (Buf != NULL) ? "Buf" : "NULL",
    BufSize,
    Mode,
    UID,
    GID);

  if (SC != NULL)
    *SC = 0;

  if (FileName == NULL)
    {
    return(FAILURE);
    }

  if (FileName[0] == '\0')
    {
    int fd;

    sprintf(FileName,"%s/tmpfile.XXXXXX",
      (SpoolDir != NULL) ? SpoolDir : "/tmp");

    if ((fd = mkstemp(FileName)) == -1)
      {
      if (SC != NULL)
        *SC = errno;

      return(FAILURE);
      }

    close(fd);
    }  /* END if (FileName[0] == '\0') */

  if ((fp = fopen(FileName,(DoAppend == TRUE) ? "a+" : "w+")) == NULL)
    {
    if (SC != NULL)
      *SC = errno;

    MDB(2,fCORE) MLog("WARNING:  cannot open file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    return(FAILURE);
    }

  if (Mode != -1)
    {
    if (chmod(FileName,Mode) == -1)
      {
      if (SC != NULL)
        *SC = errno;

      MDB(1,fCORE) MLog("WARNING:  cannot chmod file '%s' to %o.  errno: %d (%s)\n",
        FileName,
        Mode,
        errno,
        strerror(errno));
      }
    }

  if ((Buf != NULL) &&
      (fwrite(Buf,BufSize,1,fp) < 1))
    {
    if (SC != NULL)
      *SC = errno;

    MDB(2,fCORE) MLog("WARNING:  cannot write data to file '%s', errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));

    fclose(fp);

    return(FAILURE);
    }

  fflush(fp);

  fd = fileno(fp);

  if (Buf != NULL)
    {
    MDB(7,fCORE) MLog("INFO:     successfully wrote following to file '%s' ---\n%s\n---\n",
      FileName,
      (char *)Buf);
    }

  if ((UID != -1) && (GID != -1))
    {
    /* currently are not checking return status of chown() change??? */

    if (fchown(fd,UID,GID) == -1)
      {
      MDB(2,fCORE) MLog("WARNING:  cannot chown file '%s', errno: %d (%s)\n",
        FileName,
        errno,
        strerror(errno));
      }
    }

  fclose(fp);

  /* clear possible cached copy */

  MFUCacheInvalidate(FileName);

  return(SUCCESS);
  }  /* END MFUCreate() */





/**
 * Search for specified file in PathList.
 *
 * @param SPath (I) specified file
 * @param PathList (I)
 * @param SpecPath (I) [optional] - additional context-sensitive search path
 * @param CWD (I) [optional]
 * @param ReqExe (I)
 * @param FPath (O)
 * @param FPathSize (I)
 */

int MFUGetFullPath(

  const char  *SPath,
  char       **PathList,
  char        *SpecPath,
  char        *CWD,
  mbool_t      ReqExe,
  char        *FPath,
  int          FPathSize)

  {
  int pindex;

  char tmpPath[MMAX_LINE];

  mbool_t IsExe;
  mbool_t IsDir;

  /* NOTE:  should add 'ign-dir' and 'ign-noexe' */

  if (FPath != NULL)
    FPath[0] = '\0';

  if ((SPath == NULL) || (PathList == NULL) || (FPath == NULL))
    {
    return(FAILURE);
    }

  /* Check if SpecifiedPath (SPath) is an absolute path */
  if (SPath[0] == '/')
    {
    /* absolute path specified, verify file exists */

    /* Test this path instance exists */
    if (MFUGetInfo(SPath,NULL,NULL,&IsExe,&IsDir) == FAILURE)
      {
      return(FAILURE);
      }

    if ((ReqExe == TRUE) && (IsExe == FALSE) && (IsDir == FALSE))
      {
      return(FAILURE);
      }

    MUStrCpy(FPath,SPath,FPathSize);

    return(SUCCESS);
    }

  /* Check for SPath in CWD if CDW is specified */
  if ((CWD != NULL) && (CWD[0] != '\0'))
    {
    if (CWD[strlen(CWD) - 1] == '/')
      {
      sprintf(tmpPath,"%s%s",
        CWD,
        SPath);
      }
    else
      {
      sprintf(tmpPath,"%s/%s",
        CWD,
        SPath);
      }

    /* Test this path instance exists */
    if ((MFUGetInfo(tmpPath,NULL,NULL,&IsExe,&IsDir) == SUCCESS) &&
        ((ReqExe == FALSE) || 
         ((IsDir == FALSE) && (IsExe == TRUE))))
      {
      /* match located */

      MUStrCpy(FPath,tmpPath,FPathSize);

      return(SUCCESS);
      }
    }  /* END if ((CWD != NULL) && (CWD[0] != '\0')) */

  /* Nothing found yet, so search the array of possible places */
  for (pindex = 0;pindex < MMAX_PATH;pindex++)
    {
    if (PathList[pindex] == NULL)
      break;

    if (PathList[pindex][strlen(PathList[pindex]) - 1] == '/')
      {
      sprintf(tmpPath,"%s%s",
        PathList[pindex],
        SPath);
      }
    else
      {
      sprintf(tmpPath,"%s/%s",
        PathList[pindex],
        SPath);
      }

    /* Test this path instance exists */
    if (MFUGetInfo(tmpPath,NULL,NULL,&IsExe,NULL) == FAILURE)
      {
      continue;
      }

    if ((ReqExe == TRUE) && (IsExe == FALSE))
      {
      continue;
      }

    MUStrCpy(FPath,tmpPath,FPathSize);

    return(SUCCESS);
    }  /* END for (pindex) */

  /* Nothing yet, so check some 'special paths' for possible hits */
  if ((SpecPath != NULL) && (SpecPath[0] != '\0'))
    {
    if (SpecPath[strlen(SpecPath) - 1] == '/')
      {
      sprintf(tmpPath,"%s%s",
        SpecPath,
        SPath);
      }
    else
      {
      sprintf(tmpPath,"%s/%s",
        SpecPath,
        SPath);
      }

    /* Test this path instance exists */
    if (MFUGetInfo(tmpPath,NULL,NULL,&IsExe,NULL) == SUCCESS)
      {
      if ((ReqExe != TRUE) || (IsExe != FALSE))
        {
        MUStrCpy(FPath,tmpPath,FPathSize);

        return(SUCCESS);
        }
      }
    }  /* END if (SpecPath != NULL) */

  /* Could NOT find a full path */

  return(FAILURE);
  }  /* END MFUGetFullPath() */





int MFUChmod(

  char *FName,  /* I */
  int   Mode)   /* I */

  {
  if (chmod(FName,Mode) == -1)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MFUChmod() */





/** 
 * Creates the path for a given buffer to be written to file before passing to MFUCreate.
 *
 * Abstracts writing job submission scripts and for MSched.StoreSubmission.
 *
 * @see MFUCreate() - child
 *
 * @param J (I)
 * @param Buffer (I) If no buffer is specified, only the unique (if set) file will be created. [optional] 
 * @param BasePath (I) BasePath of the file.
 * @param ExtPath (I) Extension to the BasePath. [optional]
 * @param Header (I) If specified, the header will be used as the file name insetead. If not set, the job name will be used. [optional]
 * @param Unique (I) If set, the file will be made unique by appending a unique number to the file name.
 * @param CmdFile (I) If set, will set (strdup) J->E.Cmd to the path of the file.
 * @param EMsg (O) [optional]
 */

int MFUWriteFile(

  mjob_t     *J,
  char       *Buffer,
  char       *BasePath,
  const char *ExtPath,
  char       *Header,
  mbool_t     Unique,
  mbool_t     CmdFile,
  char       *EMsg)
  
  {
  int fd = -1;

  char FileDir[MMAX_PATH_LEN];
  char SpecFile[MMAX_PATH_LEN];
  const char *ExtPtr;

  if ((J == NULL) || (BasePath == NULL))
    {
    return(FAILURE);
    }

  strcpy(FileDir,BasePath);

  if (FileDir[strlen(FileDir) - 1] != '/')
    strcat(FileDir,"/");

  if (ExtPath != NULL)
    {
    ExtPtr = ExtPath;
    
    if (ExtPtr[0] == '/')
      ExtPtr++;
    
    strcat(FileDir,ExtPtr);

    if (FileDir[strlen(FileDir) - 1] != '/')
      strcat(FileDir,"/");
    }

  errno = 0;

  /* Generate a random number after the filename in case there are
    * duplicate jobids */

  snprintf(SpecFile,sizeof(SpecFile),"%s/%s%s",
    FileDir,
    (Header != NULL) ? Header : J->Name,
    (Unique) ? ".XXXXXX" : "");

  if ((Unique == TRUE) && ((fd = mkstemp(SpecFile)) < 0))
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"cannot create file in directory '%s' (errno: %d, '%s')",
        FileDir,
        errno,
        strerror(errno));
      }

    return(FAILURE);
    }

  if (fd >= 0)
    close(fd);

  if (CmdFile)
    MUStrDup(&J->Env.Cmd,SpecFile);

  /* write file to spool directory */

  if ((Buffer != NULL) &&
      (MFUCreate(
        SpecFile,
        NULL,
        Buffer,
        strlen(Buffer),
        S_IXUSR|S_IRUSR,
        -1, 
        -1,
        TRUE,
        NULL) == FAILURE))
    {
    MFURemove(SpecFile);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot create command file");

    return(FAILURE);
    }

  return(SUCCESS);
  } /* END int MFUWriteFile() */






/**
 * Try to acquire a lock on the given file.
 *
 * @param LockFile (I) The name of the file to lock.
 * @param LockFD (I/O) File descriptor for the lock file.
 * @param FileType (I) For logging (type of file lock)
 *
 * @return SUCCESS if the lock is acquired, FAILURE if another process already has the lock.
 */

int MFUAcquireLock(

  char        *LockFile,
  int         *LockFD,
  const char  *FileType)

  {
  struct flock flock;
  int fds;

  if ((LockFile == NULL) ||
      (LockFD == NULL) ||
      (FileType == NULL))
    {
    return(FAILURE);
    }

  if (LockFile[0] == '\0')
    {
    MDB(0,fSTRUCT) MLog("ALERT:   empty %s lock filename\n",
      FileType);

    return(FAILURE);
    }

  fds = open(LockFile,O_CREAT|O_RDWR,0600);

  if (fds < 0)
    {
    /* could not open lock file */

    MDB(0,fSTRUCT) MLog("ALERT:   could not open %s lock file '%s' (errno: %d:%s)\n",
      FileType,
      LockFile,
      errno,
      strerror(errno));

    return(FAILURE);
    }

  flock.l_type   = F_WRLCK;  /* F_WRLCK  or  F_UNLCK */
  flock.l_whence = SEEK_SET;
  flock.l_start  = 0;
  flock.l_len    = 0;
  flock.l_pid    = getpid();

  if (fcntl(fds,F_SETLK,&flock) != 0)
    {
    close(fds);

    MDB(0,fSTRUCT) MLog("ALERT:   could not create lock on file '%s' (errno: %d:%s)\n",
      LockFile,
      errno,
      strerror(errno));

    return(FAILURE);
    }

  /* don't close file or we'll lose the lock! */

  *LockFD = fds;

  return(SUCCESS);
  }  /* END MFUAcquireLock() */




/**
 * Try to release a lock on the given file.
 *
 * @param LockFile (I) Name of the file to unlock
 * @param LockFD (I) File descriptor to unlock [modified]
 * @param FileType (I) Type of lock
 *
 * @return SUCCESS if the lock is released, FAILURE otherwise.
 */
int MFUReleaseLock(

  char *LockFile,
  int  *LockFD,
  char *FileType)

  {
  struct flock flock;
  int fds;

  if ((LockFile == NULL) ||
      (LockFD == NULL) ||
      (FileType == NULL))
    {
    return(FAILURE);
    }

  if (LockFile[0] == '\0')
    {
    return(FAILURE);
    }

  if (*LockFD > 0)
    {
    fds = *LockFD;
    }
  else
    {
    fds = open(LockFile,O_CREAT|O_TRUNC|O_WRONLY,0600);
    }

  if (fds < 0)
    {
    /* could not open lock file */

    return(FAILURE);
    }

  flock.l_type   = F_UNLCK;
  flock.l_whence = SEEK_SET;
  flock.l_start  = 0;
  flock.l_len    = 0;

  if (fcntl(fds,F_SETLK,&flock) != 0)
    {
    close(fds);

    return(FAILURE);
    }

  close(fds);

  *LockFD = 0;

  return(SUCCESS);
  }  /* END MFUReleaseLock() */



/**
 * This function removes files from the given path if they were last modified after ExpireTime.
 *
 * Note that this function will skip over special filenames like '.' and '..'.
 *
 * @param DirPath (I)
 * @param ExpireTime (I) how old the file must be to be removed
 * @param DoFork (I) fork in order to keep Moab from blocking
 * @param EMsg (I) optional,minsize=MMAX_LINE
 */

int MURemoveOldFilesFromDir(

  char   *DirPath,     /* I */
  mulong  ExpireTime,  /* I (how old the file must be to be removed) */
  mbool_t DoFork,      /* I */
  char   *EMsg)        /* I (optional,minsize=MMAX_LINE) */

  {
  char tmpPath[MMAX_PATH_LEN];

  DIR *DirHandle;
  struct dirent *FileHandle;
  mulong FMTime;

  mbool_t IsDir = FALSE;

  pid_t pid;

  const char *FName = "MURemoveOldFilesFromDir";

  MDB(7,fSTRUCT) MLog("%s(%s,%ld,EMsg)\n",
    FName,
    (DirPath != NULL) ? DirPath : "NULL",
    ExpireTime);

  if ((DirPath == NULL) || (DirPath[0] == '\0'))
    {
    return(FAILURE);
    }

  pid = 0;  /* default behavior is to perform child funcionality */

  /* fork before checking old files for removal */

  if (DoFork == TRUE)
    {
    if ((pid = MUFork()) == -1)
      {
      /* Fork failed, resume in main process */

      DoFork = FALSE;
      }
    else if (pid > 0)
      {
      /* parent */

      return(SUCCESS);
      }
    }

  /* open directory for reading */
      
  DirHandle = opendir(DirPath);
  
  if (DirHandle == NULL)
    {
    MDB(1,fALL) MLog("ALERT:    cannot open directory '%s' for removal of old files\n",
      DirPath);

    if (EMsg != NULL)
      {
      snprintf(EMsg,sizeof(EMsg),"ALERT:    cannot open directory '%s' for removal of old files\n",
        DirPath);
      }

    if (DoFork == TRUE)
      exit(1);
    else
      return(FAILURE);
    }
    
  FileHandle = readdir(DirHandle);
  
  while (FileHandle != NULL)
    {    
    /* attempt to delete old files */

    if (!strcmp(FileHandle->d_name,".") ||
        !strcmp(FileHandle->d_name,".."))
      {
      /* skip special file names */

      FileHandle = readdir(DirHandle);

      continue;
      }

    if (!MUStrCmpReverse(".cp",FileHandle->d_name))
      {
      /* do not delete checkpoint files! */

      FileHandle = readdir(DirHandle);

      continue;
      }

    snprintf(tmpPath,sizeof(tmpPath),"%s/%s",
      DirPath,
      FileHandle->d_name);

    if (MFUGetInfo(tmpPath,&FMTime,NULL,NULL,&IsDir) == FAILURE)
      {
      MDB(3,fALL) MLog("ALERT:    cannot determine modify time for file '%s'\n",
        tmpPath);

      if (EMsg != NULL)
        {
        snprintf(EMsg,sizeof(EMsg),"ALERT:    cannot determine modify time for file '%s'\n",
          tmpPath);
        }

      FMTime = 0;

      FileHandle = readdir(DirHandle);

      continue;
      }

    if ((IsDir == FALSE) &&
        (MSched.Time - FMTime) > ExpireTime)
      {
      /* file is too old - delete it */


      if (remove(tmpPath))
        {
        MDB(3,fALL) MLog("cannot remove file  '%s' - errno=%d %s\n",
          tmpPath,
          errno,
          strerror(errno));
        }
      else
        {
        MDB(7,fALL) MLog("successfully removed file  '%s'\n",
          tmpPath);
        }
      }

    FileHandle = readdir(DirHandle);
    }

  closedir(DirHandle);

  if (DoFork == TRUE)
    {
    exit(0);
    }

  return(SUCCESS);
  }  /* END MURemoveOldFiles() */




/* END MFile.c */
