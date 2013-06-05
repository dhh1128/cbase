/* HEADER */

#ifndef __MEventLog_H__
#define __MEventLog_H__

#include <string>


/**
 *  This class represents a name-value pair.
 */

class NVPair
  {
  /* Name-Value Pair */

  public:
  NVPair(const std::string &N,const std::string &V)
    {
    Name = N;
    Value = V;
    }

  ~NVPair()
    {
    }

  std::string GetName()
    {
    return(Name);
    }

  std::string GetValue()
    {
    return(Value);
    }

  void SetName(std::string n)
    {
    Name = n;
    }

  void SetValue(std::string v)
    {
    Value = v;
    }

  private:
  std::string Name;  /* Name of the pair */
  std::string Value; /* Value of the pair */
  };  /* END class NVPair */


/**
 * A representation of a Moab object to be used by event logs.
 *  This represents the object at the time of the event.
 */

class MEventLogObject
  {
  public:

  MEventLogObject(std::string SName,enum MXMLOTypeEnum SObjType,void *Object);

  std::string             GetName();
  enum MXMLOTypeEnum      GetObjType();
  std::string             GetObject();
  std::string             GetServiceID();
  std::string             GetTopServiceID();
  mxml_t                 *GetXML(mbool_t PrimaryObject,mbool_t Serialize);

  std::string             GetVarFromVars(mhash_t *VarsH,mln_t *VarsLL,mln_t *ParentVCs,std::string Var);

  private:
  std::string             GetServiceIDFromVars(mhash_t *VarsH,mln_t *VarsLL,mln_t *ParentVCs);
  std::string             GetTopServiceIDFromVars(mhash_t *VarsH,mln_t *VarsLL,mln_t *ParentVCs);


  std::string Name;                /* Name of object */
  enum MXMLOTypeEnum ObjType; /* Type of object */
  std::string ObjectRepr;          /* XML representation of object (optional) */
  std::string ServiceID;           /* Service ID of this object (pulled from variables at event create time) */
  std::string TopServiceID;        /* Top-level Service ID of this object */
  };  /* END class MEventLogObject */


/**
 * An error message to be part of an event log.
 */

class MEventLogError
  {
  public:

  MEventLogError();

  void SetErrorCode(int SErrorCode);
  void SetErrorMessage(std::string SErrorMessage);

  mxml_t *GetXML();

  private:

  mbool_t ErrorCodeSet; /* TRUE if an error code was given */
  int     ErrorCode;    /* The error code */
  std::string  ErrorMessage; /* The error message */
  };  /* END class MEventLogError */


/**
 * Base class for events that use the web service event log interface.
 *
 * Time will automatically be set to MSched.Time (but can be overridden),
 * and source component will always be reported as "MWM".
 */

class MEventLog
  {

  public:
  MEventLog(enum MEventLogTypeEnum SType);
  ~MEventLog();

  /* Event common attrs */

  time_t *GetTime();
  void SetType(enum MEventLogTypeEnum SType);
  enum MEventLogTypeEnum GetType();
  void SetFacility(enum MEventLogFacilityEnum Facility);
  enum MEventLogFacilityEnum GetFacility();
  void SetCategory(enum MEventLogCategoryEnum Category);
  enum MEventLogCategoryEnum GetCategory();
  void SetInitiatedByUser(std::string SName);
  std::string GetInitiatedByUser();
  void SetProxyUser(std::string SName);
  std::string GetProxyUser();

  void SetStatus(std::string SStatus);
  std::string GetStatus();

  /* Errors */

  void AddError(MEventLogError *Error);

  /* Details */

  void AddDetail(std::string Name,std::string Value);

  /* Associated objects */

  void SetPrimaryObject(std::string SName,enum MXMLOTypeEnum PtrType,void *Object);
  void AddRelatedObject(std::string SName,enum MXMLOTypeEnum PtrType,void *Object);

  /* Printing */

  mxml_t *GetXML();

  private:
  time_t                     Time;     /* Time this event occured */
  enum MEventLogTypeEnum     Type;     /* Type of this event */
  enum MEventLogFacilityEnum Facility; /* What object or area of Moab this event deals with */
  enum MEventLogCategoryEnum Category; /* What category of event this is (start, modify, etc.) */

  std::string         Status;          /* Status of the event (success by default) */

  std::string         InitiatedByUser; /* User which caused the event */
  std::string         ProxyUser;       /* Proxy user which caused the event */

  MEventLogObject *PrimaryObject;      /* Primary object that this event pertains to */
  std::list<MEventLogObject *> RelatedObjects; /* Other objects which this event pertains to */

  std::list<MEventLogError *> Errors;  /* Errors associated with this event */
  std::list<NVPair *>   Details;       /* Details about this event (max is 50 in MWS) */
  };  /* END class MEventLog */

int MEventLogExportToWebServices(MEventLog *);

/* Convenience wrapper functions */
int GetCategoryAndFacility(enum MEventLogTypeEnum,enum MEventLogFacilityEnum*,enum MEventLogCategoryEnum*);
mbool_t EventLogTypeUsesSerialization(enum MEventLogTypeEnum);
int CreateAndSendEventLog(enum MEventLogTypeEnum,std::string,enum MXMLOTypeEnum,void *);
int CreateAndSendEventLogWithMsg(enum MEventLogTypeEnum,std::string,enum MXMLOTypeEnum,void *,std::string);
#endif /* __MEventLog_H__ */
