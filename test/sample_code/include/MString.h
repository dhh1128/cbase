/* HEADER */

#ifndef __MSTRING_H__
#define __MSTRING_H__

/**
 * @file MString.h
 * 
 * MString API Interface
 */


typedef class mstring_t {

  /* internal class state information - only UNIT TESTS get an API to external (non API) get/set */

  char *CString;              /* pointer to calloc'd memory */
  int   CStringSize;          /* size of memory calloc'd */
  char *BPtr;                 /* current pointer (within CString) of next character placement */
  int   BSpace;               /* amount of space left in current buffer */

  /* Private and Public methods */

  /* private helper to allocate new memory */

  void allocInstance(int InitialSize)
    {
    if (InitialSize <= 0)
      InitialSize = MMAX_LINE;

    /* No buffer during construction, so alloc one */
    CString = (char *) calloc(1,sizeof(char) * InitialSize);
    CStringSize = InitialSize;

    /* Set the current pointer and space left */
    BPtr = CString;
    BSpace = CStringSize;
    }

  void freeInstance()
   {
   /* Only free if there is a buffer pointer present */
   if (CString != NULL)
     free(CString);
   }



public: 

  static const size_t npos;    /* static class member. Return value on errors */

  /* external helpers */
  void reinitInstance(int Size);

  /* copy constructor */

  mstring_t(const mstring_t& rhs);

  /* operator= overload:  assignment operator */

  mstring_t & operator=  (const mstring_t &rhs);
  mstring_t & operator=  (const char *s);
  mstring_t & operator=  (char c);
  mstring_t & operator+= (const mstring_t &rhs);
  mstring_t & operator+= (const char *s);
  mstring_t & operator+= (const char c);
  const mstring_t operator+  (const mstring_t &rhs) const;

  void        reserve(size_t res_arg);                  /* API .reserve method */
  size_t      find(char c, size_t pos = 0) const;       /* API .rind method */
  mstring_t&  erase(size_t pos = 0, size_t n = npos);   /* API .erase method */


  /* API .clear method */

  void clear()
    {
    /* Reset the BPtr and BSpace attributes to beginning of buffer, with full capacity space to use */
    BPtr = CString;
    BSpace = CStringSize;

    /* If there is any memory in the object, then null terminate the string */
    if (NULL != BPtr)
      *BPtr = '\0';
    }


  /* API Constructors */

  mstring_t()
    {
    allocInstance(MMAX_NAME);
    }

  mstring_t(int InitialSize)
    {
    allocInstance(InitialSize);
    }

  mstring_t(const char *s)
    {
    CString = NULL;
    CStringSize = 0;
    BPtr = NULL;
    BSpace = 0;

    /* Use the assignment operator to set it up */
    *this = s;
    }

  /* API Destructors */

  ~mstring_t()
    {
    freeInstance();

    CString = NULL;
    CStringSize = 0;
    BPtr = NULL;
    BSpace = 0;
    }

  /* accessors */ 

  char * c_str() const
    {
    return(CString);
    }

  /* size and length are the same: how big is the null terminated string  */

  size_t size() const
    {
    if ((CString != NULL ) && (BPtr != NULL))
      return(BPtr - CString);
    else
      return(0);
    }

  size_t length() const
    {
    if ((CString != NULL ) && (BPtr != NULL))
      return(BPtr - CString);
    else
      return(0);
    }

  /* current capacity of the allocated buffer */

  size_t capacity() const
    {
    return(CStringSize);
    }

  /* is the string empty method */

  bool empty() const
    {
    if ((CString == NULL) || (CStringSize == 0) || (CString[0] == '\0'))
      return(true);
    
    return(false);
    }

  /*
   * operator[] methods
   */
  const char& operator[] (size_t pos) const
    {
    return(CString[pos]);
    }

  char& operator[] (size_t pos)
    {
    return(CString[pos]);
    }

  const char& operator[] (int pos) const
    {
    return(CString[pos]);
    }
  
  char& operator[] (int pos)
    {
    return(CString[pos]);
    }


  /* NOTE: For UNITTEST USE ONLY */

  char *getBPtr()
  {  return(BPtr); }

  int getBSpace()
  {  return(BSpace); }

  void setCStringSize(int size)
  { CStringSize = size; }
   

  } mstring_t;



int MStringSet(mstring_t *,const char *);
int MStringSetF(mstring_t *,const char *,...);
int MStringAppend(mstring_t *,const char *);
int MStringAppendCount(mstring_t *,char *,int);
int MStringAppendF(mstring_t *,const char *,...);
int MStringStripFromIndex(mstring_t *,int);
int MStringInsertArg(mstring_t *,const char *,const char *,const char *,const char *);
int MStringReplaceStr(const char *,const char *,const char *,mstring_t *);

#endif  /*  __MSTRING_H__ */
