/* HEADER */

/**
 * @file PluginManager.h
 *
 * static class header that handles the loading and unloading of plugin libraries.
 */

#ifndef namespace_PluginManager_h
#define namespace_PluginManager_h

#include <string>
#include "PluginNodeAllocate.h"

class PluginManager
{
private:
  PluginManager() {} // so you can't create an instance of the class.
public:
  static void *LoadLibrary(const std::string absPath);
  static void  UnloadLibrary(void *handle);
  static void *LoadSymbol(void *handle, const std::string symbol);
  static std::string ResolveFullLibPath(const char *pluginPath);
  static PluginNodeAllocate *CreateNodeAllocationPlugin(const char *pluginKey, const char *pluginPath);
  static void ValidateNodeAllocationPlugin(const char *pluginKey, const char *pluginPath);
};

#endif
