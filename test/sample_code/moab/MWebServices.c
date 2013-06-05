

using namespace std;

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include <stdio.h>
#include <string>

#ifdef MUSEWEBSERVICES
#include <curl/curl.h>

size_t __MWSWriteData(char *,size_t,size_t,void *);
size_t __MWSReadData(void *,size_t,size_t,void *);
int __MWSSetCURLOptions(CURL **,struct curl_slist *,mstring_t *,char *);
int __MWSAddHandle(CURL **,CURLM *,struct curl_slist *,mstring_t *,char *);
int __MWSMultiPerform(CURL **,CURLM *,int,mstring_t **);
int __MWSWriteToFile(char *);


/** 
  * Saves the data returned from the server.  Return the number 
  * of bytes written 
  *  
  * @param ptr       (O) data to be saved
  * @param Size      (I) size of chunk
  * @param nmemb     (I) number of chunks 
  * @param UserData  (I) data passed in from curl
  */

size_t __MWSWriteData(

  char *ptr,
  size_t size,
  size_t nmemb,
  void *UserData)

  {
  /* We're not saving any data right now but curl still wants to have
     this function defined */

  return(size * nmemb);
  }





/**
  * Added data to curl packet.  This function gets called by 
  * libcurl as soon as it needs to read data in order to send it 
  * to web services. The data area pointed at by the pointer ptr
  * may be filled with at most size multiplied with nmemb number 
  * of bytes. The function must return the actual number of 
  * bytes that are stored in "ptr".  Since the function can be 
  * called multiple times, 0 must be returned then next time the 
  * function is called to signify the end. 
  *
  * @param ptr       (O) data to be exported
  * @param Size      (I) size of chunk
  * @param nmemb     (I) number of chunks 
  * @param Buffer    (I) data passed in from curl
  */

size_t __MWSReadData(

  void *ptr,
  size_t Size,
  size_t nmemb,
  void *Buffer)

  {
  size_t MaxSize;
  size_t len;
  char *head;
  char *tail;

  head = (char *)Buffer;

  /* return 0 if the string is empty because we've already
     written it out to curl */
  if (*head == '\0')
    {
    return(0);
    }

  MaxSize = Size * nmemb;

  len = strlen(head);

  if (len < MaxSize)
    {
    memcpy(ptr,head,len);

    /* empty the string so the callback ends the transmission
       then next time this function is called */
    *head = '\0';
    return(len);
    }
  else
    {
    memcpy(ptr,head,MaxSize);
    tail = head + MaxSize;
    
    /* shift string down */
    while (*tail != '\0')
      {
      *(head++) = *tail;
      tail++;
      }

    *head = '\0';

    return(MaxSize);
    }
  }  /* END __MWSReadData() */





/**
  * Set CURL Options
  *
  * @param Handle       (I/O) Curl Handle
  * @param HTTPHeader   (I) HTTP Header
  * @param Object       (I) Data Object
  * @param URL          (I) Optional webservice url
  */

int __MWSSetCURLOptions(

  CURL              **Handle,
  struct curl_slist *HttpHeader,
  mstring_t         *Object,
  char              *URL)

  {
  long len;

#if 0
  /* Used for POST */

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
#endif

  if ((Handle == NULL) ||
      (*Handle == NULL) ||
      (HttpHeader == NULL) ||
      (Object == FAILURE))
    return(FAILURE);

#if 0
  /* Multi POST code */

  /* specify target URL */
  if ((URL != NULL) && (URL->c_str() != NULL))
    curl_easy_setopt(*Handle,CURLOPT_URL,URL->c_str());
  else
    curl_easy_setopt(*Handle,CURLOPT_URL,MSched.WebServicesURL);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "Moab",
               CURLFORM_COPYCONTENTS, Object->c_str(),
               CURLFORM_END);
  
  /* set header */
  curl_easy_setopt(*Handle,CURLOPT_HTTPHEADER,HttpHeader);

  /* Add post info in */
  curl_easy_setopt(*Handle,CURLOPT_HTTPPOST,formpost);
#endif

  /* PUT code */

  /* we want to use our write function */ 
  curl_easy_setopt(*Handle, CURLOPT_WRITEFUNCTION, __MWSWriteData);

  /* we want to use our read function */ 
  curl_easy_setopt(*Handle, CURLOPT_READFUNCTION, __MWSReadData);

  /* enable uploading */ 
  curl_easy_setopt(*Handle,CURLOPT_UPLOAD,1L);

  /* enable for verbose output in gdb */
  /*curl_easy_setopt(*Handle,CURLOPT_VERBOSE,1L);*/

  /* specify target URL */
  if (URL != NULL)
    curl_easy_setopt(*Handle,CURLOPT_URL,URL);
  else
    curl_easy_setopt(*Handle,CURLOPT_URL,MSched.WebServicesURL);

  /* set header */
  curl_easy_setopt(*Handle,CURLOPT_HTTPHEADER,HttpHeader);
  
  /* set data to send */
  curl_easy_setopt(*Handle,CURLOPT_READDATA,(void *)Object->c_str());

  /* provide the size of the upload */
  len = strlen(Object->c_str());
  curl_easy_setopt(*Handle,CURLOPT_INFILESIZE_LARGE,(curl_off_t)len);

  /* set timeouts */
  curl_easy_setopt(*Handle,CURLOPT_TIMEOUT,1L);
  curl_easy_setopt(*Handle,CURLOPT_CONNECTTIMEOUT,1L);

  /* fail silently if the HTTP code returned is equal to or larger than 400 */
  curl_easy_setopt(*Handle,CURLOPT_FAILONERROR,1L);

  return(SUCCESS);
  }  /* END __MWSAddHandle() */





/**
  * Create HTTP Header
  *
  * @param HTTPHeader   (I) HTTP Header
  */

int __MWSCreateHeader(

  struct curl_slist **HttpHeader)

  {

  /* Append Content-Type to the HTTP header */
  /**HttpHeader = curl_slist_append(*HttpHeader, "Content-Type: application/xml");*/
  *HttpHeader = curl_slist_append(*HttpHeader, "Expect:"); 
  
  return(SUCCESS);
  }





/**
  * Add CURL Handle to MultiHandle
  *
  * @param Handle       (I/O) Curl Handle
  * @param MultiHandle  (I) Curl MultiHandle
  * @param HttpHeader   (I) HTTP Header info
  * @param Object       (I) Data Object
  * @param URL          (I) Optional webservice destination URL
  */

int __MWSAddHandle(

  CURL              **Handle,
  CURLM             *MultiHandle,
  struct curl_slist *HttpHeader,
  mstring_t         *Object,
  char              *URL)

  {
  __MWSSetCURLOptions(Handle,HttpHeader,Object,URL);

  /* add it to the multi handle */
  curl_multi_add_handle(MultiHandle,*Handle);

  return(SUCCESS);
  }  /* END __MWSAddHandle() */





/**
  * Perform multi http puts
  *
  * @param MultiHandle    (I) The multihandle to init
  * @param HttpHeader     (I) The HTTP header
  */

int __MWSMultiPerform(

  CURL        **Handles,
  CURLM       *MultiHandle,
  int         ObjectCount,
  mstring_t   **Objects)

  {
  int StillRunning;
  int MsgsLeft;

  CURLMsg *CurlMsg;

  if ((Handles == NULL) || (*Handles == NULL) ||
      (MultiHandle == NULL) || (ObjectCount < 0) ||
      (Objects == NULL) || (*Objects == NULL))
    return FAILURE;

  /* actually start the PUT */ 
  curl_multi_perform(MultiHandle,&StillRunning);

  while (StillRunning) 
    {
    struct timeval Timeout;
    int rc; /* select() return code */ 
 
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;
 
    long CurlTimeOut = -1;
 
    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);
 
    /* set a suitable timeout to play around with */ 
    Timeout.tv_sec = 1;
    Timeout.tv_usec = 0;
 
    curl_multi_timeout(MultiHandle, &CurlTimeOut);
    if (CurlTimeOut >= 0) 
      {
      Timeout.tv_sec = CurlTimeOut / 1000;
      if (Timeout.tv_sec > 1)
        Timeout.tv_sec = 1;
      else
        Timeout.tv_usec = (CurlTimeOut % 1000) * 1000;
      }
 
    /* get file descriptors from the transfers */ 
    curl_multi_fdset(MultiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
 
    /* In a real-world program you OF COURSE check the return code of the
       function calls.  On success, the value of maxfd is guaranteed to be
       greater or equal than -1.  We call select(maxfd + 1, ...), specially in
       case of (maxfd == -1), we call select(0, ...), which is basically equal
       to sleep. */ 
 
    rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &Timeout);
 
    switch(rc) 
      {
      case -1:
        /* select error */ 
        break;
      case 0: /* timeout */
      default: /* action */ 
        curl_multi_perform(MultiHandle, &StillRunning);
        break;
      }
    }

  /* See how the transfers went */

  while ((CurlMsg = curl_multi_info_read(MultiHandle, &MsgsLeft)))
    {
    /* errnos defined in curl/curl.h */

    if (CurlMsg->msg == CURLMSG_DONE)
      {
      int idx;
      double time;

      curl_easy_getinfo(CurlMsg->easy_handle, CURLINFO_TOTAL_TIME, &time);
      MDB(8,fALL) MLog("INFO:    transfer time was %.2f seconds.\n",time);

      if (CurlMsg->data.result != CURLE_OK)
        {
        /* Find out which handle this message is about */
  
        for (idx = 0;idx < ObjectCount;idx++)
          {
          if (Handles[idx] == CurlMsg->easy_handle)
            {
            MDB(7,fALL) MLog("ERROR:    Error writing to Web Services - Errno %d.\n",CurlMsg->data.result);
    
            /* Save data to file */
            __MWSWriteToFile(Objects[idx]->c_str());
  
            break;
            }
          }
        }
      }
    }

  return(SUCCESS);
  }  /* END __MWSMultiPerform() */





/**
  * Write data to file upon error
  *
  * @param Data    (I) data to be written to file
  */

int __MWSWriteToFile(

  char *Data)  /* I */

  {
  FILE* WriteFile;

  if (Data == NULL)
    return(FAILURE);

  /* restore the greater-than sign to the xml tag that was removed
     so the callback ends the transmission */

  *Data = '<';

  /* Don't record current state, only events and stats */

  if ((strncmp("node",(Data+1),strlen("node")) == 0) ||
    (strncmp("job",(Data+1),strlen("job")) == 0) ||
    (strncmp("rsv",(Data+1),strlen("rsv")) == 0) ||
    (strncmp("trig",(Data+1),strlen("trig")) == 0) ||
    (strncmp("vc",(Data+1),strlen("vc")) == 0) ||
    (strncmp("vm",(Data+1),strlen("vm")) == 0))
    {
    return(FAILURE);
    }

  if ((WriteFile = fopen(MSched.WSFailOverFile, "a")) == NULL)
    {
    MDB(8,fALL) MLog("ERROR:    cannot open file %s to save data\n",MSched.WSFailOverFile);
    return(FAILURE);
    }

  /* write data to file */

  fprintf(WriteFile,"%s\n",Data);
  fclose(WriteFile);

  return(SUCCESS);
  }  /* END __MWSWriteToFile() */





/**
  * Write the given strings to Moab Web Services
  *
  * @param Objects  (I) the strings to write 
  * @param Count    (I) the number of objects
  * @param Info     (I) Optional webservice destination info
  */

int MWSWriteObjects(

  mstring_t    **Objects,
  int            Count,
  mwsinfo_t     *Info)

  {
  const char *FName = "MWSWriteObjects";

  MDB(7,fSTRUCT) MLog("%s(Objects,%d,%s)\n",
    FName,
    Count,
    ((Info != NULL) && (Info->URL != NULL)) ? Info->URL : "NULL");

  if ((Objects == NULL) || (*Objects == NULL) || (Count <= 0))
    {
    return(FAILURE);
    }

  /* POST */

  return(MWSWriteObjectsPOST(Objects,Count,Info));

#if 0
  /* PUT */

  return(MWSWriteObjectsMultiPUT(Objects,Count,URL));
#endif
  }  /* END MWSWriteObjects() */


/**
  * Write the given strings to Moab Web Services via POST
  *
  * @param Objects  (I) the strings to write 
  * @param Count    (I) the number of objects
  * @param Info     (I) Optional webservice destination info
  */

int MWSWriteObjectsPOST(

  mstring_t    **Objects,
  int            Count,
  mwsinfo_t     *Info)

  {
  int index;
  CURL *curl = NULL;
  CURLcode res;
  char *DestURL = NULL;

  if ((Objects == NULL) || (*Objects == NULL) || (Count <= 0))
    {
    return(FAILURE);
    }

  if ((Info != NULL) && (Info->URL != NULL))
    DestURL = Info->URL;
  else if (MSched.WebServicesURL != NULL)
    DestURL = MSched.WebServicesURL;
  else
    {
    MDB(7,fSTRUCT) MLog("ERROR:    No destination URL found for web service call\n");

    return(FAILURE);
    }

  curl_global_init(CURL_GLOBAL_ALL);

  for (index = 0;index < Count;index++)
    {
    curl = curl_easy_init();

    if (curl)
      {
      curl_slist *HeaderList = NULL;

      /* First set the URL that is about to receive our POST. This URL can
         just as well be a https:// URL if that is what should receive the
         data. */
      curl_easy_setopt(curl, CURLOPT_URL, DestURL);

      if (Info != NULL)
        {
        string  usrpass;

        if (Info->Username != NULL)
          usrpass += Info->Username;

        usrpass += ":";

        if (Info->Password != NULL)
          usrpass += Info->Password;

        curl_easy_setopt(curl,CURLOPT_USERPWD,usrpass.c_str());

        if (Info->HTTPHeader != NULL)
          {
          HeaderList = curl_slist_append(HeaderList,Info->HTTPHeader);
          curl_easy_setopt(curl,CURLOPT_HTTPHEADER,HeaderList);
          }
        }  /* END if (Info != NULL) */

      /* Now specify the POST data */
      if ((Objects[index] != NULL) &&
          (Objects[index]->c_str() != NULL))
        {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, Objects[index]->c_str());

        /* Set read function so that the reply isn't printed out in gdb */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __MWSWriteData);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        MDB(9,fALL) MLog("INFO:    return code was %d\n",res);
        }
      else
        {
        /* Problem - bad string given */

        /* NO-OP */
        }

      /* always cleanup */
      curl_easy_cleanup(curl);

      if (HeaderList != NULL)
        curl_slist_free_all(HeaderList);
      }  /* END if (curl) */
    }  /* END for (index) */

  /* Free objects from the queue */
  for (index = 0;index < Count;index++)
    {
    if (Objects[index] != NULL)
      {
      /* call the destructor on the mstring_t instance */

      delete Objects[index];
      }
    }

  curl_global_cleanup();

  return(SUCCESS);
  }  /* END MWSWriteObjectsPOST() */ 


/**
  * Write the given strings to Moab Web Services using curl multi PUT
  *
  * @param Objects  (I) the strings to write 
  * @param Count    (I) the number of objects
  * @param Info     (I) Optional webservice destination info
  */

int MWSWriteObjectsMultiPUT(

  mstring_t    **Objects,
  int            Count,
  mwsinfo_t     *Info)

  {
  int index;

  CURL *Handles[MMAX_NODE_DB_WRITE];
  CURLM *MultiHandle;
  struct curl_slist *HttpHeader = NULL;

  if ((Objects == NULL) || (*Objects == NULL) || (Count <= 0))
    {
    return(FAILURE);
    }

  if ((Info == NULL) || (Info->URL == NULL))
    return(FAILURE);

  /* Init the multi handle */
  MultiHandle = curl_multi_init();
  curl_global_init(CURL_GLOBAL_ALL);

  __MWSCreateHeader(&HttpHeader);

  /* Iterate through all XML entries and add them to the multihandle */

  for (index = 0;index < Count;index++) 
    {
    if (Objects[index] == NULL)
      break;

    Handles[index] = curl_easy_init();

    /* Add easy handle to multihandle */
    __MWSAddHandle(&Handles[index],MultiHandle,HttpHeader,Objects[index],Info->URL);
    }

  /* do HTTP Put */
  __MWSMultiPerform(Handles,MultiHandle,Count,Objects);

  /* clean up */
  for (index = 0;index < Count;index++)
    {
    if (Handles[index] != NULL)
      {
      /* call the destructor on the mstring_t instance */
      delete Objects[index];

      curl_easy_cleanup(Handles[index]);
      }
    }

  curl_multi_cleanup(MultiHandle);
  curl_slist_free_all(HttpHeader); 
  curl_global_cleanup();

  return(SUCCESS);
  }  /* END MWSWriteObjectsMultiPUT() */


#else

int MWSWriteObjects(

  mstring_t    **Objects,
  int            Count,
  mwsinfo_t     *Info)

  {
  return(FAILURE);
  }

#endif

/**
 * Create a web services info object.
 *
 * @param InfoP [O] - the info object to create
 */

int MWSInfoCreate(

  mwsinfo_t **InfoP)

  {
  if (InfoP == NULL)
    return(FAILURE);

  *InfoP = (mwsinfo_t *)MUCalloc(1,sizeof(mwsinfo_t));

  if (*InfoP == NULL)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MWSInfoCreate() */

/**
 * Destroy a web services info object.
 *
 * @param InfoP [I] - the info object to destroy
 */

int MWSInfoFree(

  mwsinfo_t **InfoP)

  {
  mwsinfo_t *Info = NULL;

  if ((InfoP == NULL) || (*InfoP == NULL))
    return(SUCCESS);

  Info = *InfoP;

  MUFree(&Info->URL);
  MUFree(&Info->Username);
  MUFree(&Info->Password);
  MUFree(&Info->HTTPHeader);

  MUFree((char **)&Info);

  *InfoP = NULL;

  return(SUCCESS);
  }  /* END MWSInfoFree() */

/* END MWebServices.c */
