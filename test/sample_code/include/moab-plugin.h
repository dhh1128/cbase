/**
 * \mainpage
 * 
 * <a href="http://www.adaptivecomputing.com/resources/docs/mwm/7-1-0/help.htm">Online Documentation</a>
 */


/**
 * @file moab-plugin.h
 *
 *  This file contains the publicly available interfaces used 
 *  for writing plugins that communicate with MOAB.
 *  This is a part of the MOAB PDK (Plugin Development Kit)
 *
 * @TODO include appropriate copyright and licensing terms in this file
 *       that lets the end-user know who owns it and how they can use it... 
 */
#ifndef __MOAB_PLUGIN_H__
#define __MOAB_PLUGIN_H__




/* Plugin function return codes */

#define     PLUGIN_RC_SUCCESS                   0
#define     PLUGIN_RC_FAILURE                   1

/* PluginType "Names" */

#define PLUGIN_TYPE_NAME_NODEALLOCATION    "NodeAllocation"

/* Supported API Versions by Header File */

#define PLUGIN_API_VERSION                 "1.0"

/* Library must export the following variables/functions,
 *   NOTE: the following 'symbols' need to be global in the plugin library
 */

extern const char *PLUGIN_NAME;    /* a string that represents the plugin's name.  This can be used in logging statements */
extern const char *PLUGIN_TYPE;    /* represents the type of policy the plugin implements. */
extern const char *PLUGIN_VERSION; /* a string that represents the version of the plugin API. */ 

/* For initialize and finish api calls */
typedef int plugin_init_t ( const char *name,
                            void **data_handle);

typedef void plugin_finish_t (void *data_handle);

extern plugin_init_t initialize;
extern plugin_finish_t finish;








/**
 *  This structure describes the name and required resources for a single request.
 *
 */
typedef struct nalloc_req {
    int  task_count; /**< resource request's task count. */
    int  memory;     /**< resource request's memory count.  Will be zero 
                       *   unless an allocating partition is a shared-memory 
                       *   partition (MB). */
} nalloc_req_t;


/**
 *  This structure describes the name and resources on an available node. 
 *
 */
typedef struct nalloc_node {
    const char *name; /**< Node's name */
    int   task_count; /**< The available taskcount for the given node. */
    int   memory;     /**< The amount of available memory for the given nodes. In MB. */
} nalloc_node_t;

 
/**
 *  This structure is used as a container to simplify making a node allocation
 *  request.  Moab will provide one container for each separate requirement.
 *  
 *  @verbatim
 *  
 *  Provided data structures:
 *  
 *  ______________
 *  | data_handle | -> Pointer to the internal data defined by the plugin.
 *  ---------------
 *  
 *  __________________
 *  | container_count |  The count of request containers.  This corresponds to
 *  -------------------  the number of separate requirements.
 *  _____________
 *  | containers |  container 0
 *  --------------
 *  |            |  container 1
 *  --------------
 *      .
 *      .
 *      .
 *  
 *  
 *  Container:
 *  _______________________ 
 *  | requirement          |
 *  | available_node_count |                      ________________   ______________
 *  | available_nodes      |-------------------->| *nalloc_node_t |->| nalloc_node |
 *  | return_node_count    |                      -----------------  ---------------
 *  | return_nodes         |------------+        | *nalloc_node_t |-+
 *  ------------------------            |         ----------------- |  ______________
 *                                      |           ...             +->| nalloc_node |  
 *                                      |                               --------------
 *                                      |
 *                                      |
 *                                      |     ________________   ______________   
 *                                      +--->| *nalloc_node_t |->| nalloc_node |  
 *                                            -----------------  ---------------  
 *                                           | *nalloc_node_t |-+                 
 *                                            ----------------- |  ______________ 
 *                                              ...             +->| nalloc_node |
 *                                                                  --------------
 *                                                                                
 *  
 *  
 *  @endverbatim
 */ 
typedef struct nalloc_container {
      nalloc_req_t    requirement;           /**< The job node allocate requirements
                                              *   structure. */
      int             available_node_count;  /**< The number of available nodes provided. */
      nalloc_node_t **available_nodes;       /**< The array of pointers to available nodes. */
      int             return_node_count;     /**< The number of returned nodes. */
      nalloc_node_t **return_nodes;          /**< The array of pointers to returned nodes. This array
                                              *   is pre-allocated to the same size as the
                                              *   available_nodes array i.e. it has available_node_count
                                              *   elements.  The plugin must take care to not attempt to
                                              *   return more nodes than the available_node_count. */
} nalloc_container_t;


/**
 *  This type definition describes the calling sequence for requesting the
 *  plugin to select nodes for allocation for a job.
 *  
 *  @param data_handle            (I) This is a pointer to the local data
 *                                    maintained by the plugin.  
 *                                
 *  @param job_name               (I) The name of the job requesting nodes.
 *                                
 *  @param container_count        (I) The number of job requirement containers
 *                                    provided.
 *  
 *  @param containers             (I/O) An array of job requirements containers.

 */
typedef int node_allocate_t  (void                *data_handle,
                              const char          *job_name,
                              int                  container_count,
                              nalloc_container_t   containers[]);

/* Library must export the following function */

extern node_allocate_t node_allocate;






#endif  /* __MOAB_PLUGIN_H__ */
