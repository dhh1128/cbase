/* HEADER */

/**
 * @file MJobTransitionXML.h
 *
 * Declares the APIs for job functions that deal with mtransjob_t structs and xml.
 */

#ifndef __MJOBTRANSITIONXML_H__
#define __MJOBTRANSITIONXML_H__

int MJobTransitionToXML(mtransjob_t  *,mxml_t **,enum MJobAttrEnum *,enum MReqAttrEnum *,long); 
int MJobTransitionBaseToXML(mtransjob_t *,mxml_t **,mpar_t *,char *);
int MJobTransitionToExportXML(mtransjob_t *,mxml_t **,mrm_t *);
int MJobXMLToTransitionStruct(mxml_t *,char *,mtransjob_t *); 
int MJobXMLTransition(mxml_t *,char *); 

#endif /* __MJOBTRANSITIONXML_H__ */
