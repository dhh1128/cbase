/* HEADER */

/**
 * @file PluginNodeAllocate.h
 *
 * PluginNodeAllocate header file
 */

#ifndef namespace_PluginNodeAllocate_h
#define namespace_PluginNodeAllocate_h

#include <string>
#include <exception>
#include "PluginBase.h"
#include "moab.h"
#include "moab-plugin.h"


/* PluginNodeAllocate class */

class PluginNodeAllocate : public PluginBase
{
public:
  /* Constructors */
  PluginNodeAllocate(const std::string pluginKey, const std::string absFileName, bool validateOnly = false);
                                             
  /* Destructor */
  ~PluginNodeAllocate();

  int Run(mjob_t *J,
          mnl_t  *NodeList[MMAX_REQ_PER_JOB],
          int     MinTPN[],
          int     MaxTPN[],
          char    NodeMap[],
          int     NodeIndex[],
          mnl_t **BestList,
          int     TaskCount[],
          int     NodeCount[]);
 
protected:
  void            *mDataHandle;   ///< used by plugin to track state between calls
  node_allocate_t *mRunAPI;       ///< the node_allocation function supplied by plugin.
  plugin_init_t   *mInitAPI;      ///< the initialization function supplied by plugin.
  plugin_finish_t *mFiniAPI;      ///< the finish function supplied by plugin.
  std::string      mKeyName;      ///< Unique(-ish) name identifying this instance to plugin library at initialization time.
  bool             mValidateOnly; ///< used to indicate whether the plugin calls the library's functions or not.

  int CheckResults(int container_count,
                   nalloc_container_t containers[]);
};


#endif
