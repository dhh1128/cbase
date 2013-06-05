/* HEADER */

/**
 * @file PluginBase.h
 *
 *  PluginBase header file
 */

#ifndef namespace_PluginBase_h
#define namespace_PluginBase_h

#include <string>
#include <exception>

#include "moab-plugin.h"



/* plugin exception class */

class PluginException: public std::exception
{
public:

  /**
   * PluginException constructor
   *
   * @param r exception's reason message.
   */
  PluginException(const std::string r) : reason(r)  { }

  virtual const char *what() const throw()
  {
    return reason.c_str();
  }

  ~PluginException() throw() { } 

private:
  std::string  reason;
};


/* PluginBase class */

class PluginBase
{
public:
  /* Constructors */
  PluginBase(const std::string absFilePath, 
             const std::string type);

  /* Destructor */
  virtual ~PluginBase();
  
  ///Getter for Name 
  virtual const char *Name(void)             { return mPluginName.c_str(); }

  ///Getter for File Name 
  virtual const char *FileName(void)         { return mFileName.c_str(); }
   
  ///Getter for Type 
  virtual const char *PluginType(void)       { return mPluginType.c_str(); }
   
  ///Getter for pApiVersion 
  virtual const char *PluginApiVersion(void) { return mPluginApiVersion.c_str(); }

private:
  std::string mFileName;
  std::string mPluginName;
  std::string mPluginApiVersion;   
  std::string mPluginType;

protected:
  void *libHandle; /* handle to PMI instance */
};


#endif
