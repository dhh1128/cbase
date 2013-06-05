/* HEADER */

/**
 * @file mcom-proto.h
 *
 * Moab Communication Prototypes
 */

/* socket util object */

int MSUInitialize(msocket_t *,char *,int,long);
int MSUListen(msocket_t *,char *);
int MSUConnect(msocket_t *,mbool_t,char *);
int MSUDisconnect(msocket_t *);
int MSUClose(msocket_t *);
int MSUFree(msocket_t *);
int MSUAcceptClient(msocket_t *,msocket_t *,char *);
int MSUSendData(msocket_t *,long,mbool_t,mbool_t,int *,char *);
int MSUSendPacket(int,char *,long,long,enum MStatusCodeEnum *);
int MSURecvData(msocket_t *,long,mbool_t,enum MStatusCodeEnum *,char *);
int MSURecvPacket(int,char **,long,const char *,long,enum MStatusCodeEnum *);
int MSUSelectWrite(int,unsigned long);
int MSUSelectRead(int,unsigned long);
int MSUCreate(msocket_t **);
int MSUDup(msocket_t **,msocket_t *);
int MSUAllocSBuf(char **,int,int,int *,mbool_t);

int MUISCreateFrame(msocket_t *,mbool_t,mbool_t,char *);



/* sec object */

int MSecGetChecksum(const char *,int,char *,char *,enum MChecksumAlgoEnum,char *);
int MSecGetChecksum2(char *,int,char *,int,char *,char *,enum MChecksumAlgoEnum,char *);
int MSecTestChecksum(char *);
int MSecBufTo64BitEncoding(char *,int,char *);
int MSecCompBufTo64BitEncoding(char *,int,char *);
int MSecComp64BitToBufDecoding(char *,int,char *,int *);
int MSecBufToHexEncoding(char *,int,char *);
int MSecCompress(unsigned char *,unsigned int,unsigned char *,const char *);
int MSecDecompress(unsigned char *,unsigned int,unsigned char *,unsigned int,unsigned char **,const char *);
int MSecEncryption(char *,const char *,int);



/* sss interface object */

int MS3LoadModule(mrmfunc_t *);
int MS3DoCommand(mpsi_t *,char *,char **,mxml_t **,int *,char *);
int MS3InitializeLocalQueue(mrm_t *,char *);
int MS3AddLocalJob(mrm_t *,char *);
int MS3RemoveLocalJob(mrm_t *,char *);
int MS3JobPreSubmit(mxml_t *,char *,mrm_t *,char *);


/* sss convenience functions */

int MS3AddSet(mxml_t *,const char *,const char *,mxml_t **);
int MS3AddChild(mxml_t *,const char*,const char *,const char *,mxml_t **);
int MS3AddWhere(mxml_t *,const char *,const char *,mxml_t **);
int MS3GetSet(mxml_t *,mxml_t **,int *,char *,int,char *,int);
int MS3GetSetMString(mxml_t *,mxml_t **,int *,char *,int,mstring_t *);
int MS3GetWhere(mxml_t *,mxml_t **,int *,char *,int,char *,int);
int MS3SetObject(mxml_t *,const char *,char *);
int MS3GetObject(mxml_t *,char *);
int MS3AddGet(mxml_t *,const char *,mxml_t **);
int MS3GetGet(mxml_t *,mxml_t **,int *,char *,int);
int MS3SetStatus(mxml_t *,mxml_t **,const char *,enum MSFC,const char *);
int MS3CheckStatus(mxml_t *,enum MSFC *,char *);
int MSCToMSC(enum MStatusCodeEnum,enum MSFC *);

/* sss object functions */

int MS3JobFromXML(void *,mjob_t **,char *);
int MS3JobToXML(mjob_t *,mxml_t **,mrm_t *,mbool_t ,mbitmap_t *,enum MJobAttrEnum *,enum MReqAttrEnum *,char *,char *);

int MCommGetThreadID();


/* END mcom-proto.h */

