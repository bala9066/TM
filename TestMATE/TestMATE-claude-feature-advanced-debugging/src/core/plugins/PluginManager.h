/**************************************************************************
 * File Name: PluginManager.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Plugin manager for loading and managing TestMATE plugins.
 * Requirements: REQ-PLG-021 to REQ-PLG-040
 **************************************************************************/

#pragma once

#include "IPlugin.h"
#include <map>
#include <memory>
#include <mutex>

namespace TestMATE {

/**************************************************************************
 * Struct: SLoadedPlugin
 * Description: Information about a loaded plugin
 **************************************************************************/
struct SLoadedPlugin {
    SPluginInfo info;
    IPlugin* pPlugin{nullptr};
    void* hLibrary{nullptr};
    FPluginDestroyFunc destroyFunc{nullptr};
    TString path;
    EPluginState state{EPluginState::kUnloaded};
};

/**************************************************************************
 * Class: CPluginManager
 * Description: Manages plugin lifecycle - loading, unloading, discovery
 * Requirements: REQ-PLG-021 to REQ-PLG-040
 **************************************************************************/
class CPluginManager {
public:
    /**************************************************************************
     * Function Name: GetInstance
     * Description: Returns singleton instance
     **************************************************************************/
    static CPluginManager& GetInstance();

    // Non-copyable
    CPluginManager(const CPluginManager&) = delete;
    CPluginManager& operator=(const CPluginManager&) = delete;

    //=========================================================================
    // Plugin Loading
    //=========================================================================

    /**************************************************************************
     * Function Name: LoadPlugin
     * Description: Loads a plugin from file
     * Parameters:
     *   in_strPath - Path to plugin library
     * Returns: Result containing plugin ID on success
     **************************************************************************/
    CResult LoadPlugin(const TString& in_strPath);

    /**************************************************************************
     * Function Name: UnloadPlugin
     * Description: Unloads a plugin by ID
     * Parameters:
     *   in_strPluginId - Plugin identifier
     **************************************************************************/
    CResult UnloadPlugin(const TString& in_strPluginId);

    /**************************************************************************
     * Function Name: UnloadAll
     * Description: Unloads all plugins
     **************************************************************************/
    void UnloadAll();

    //=========================================================================
    // Plugin Discovery
    //=========================================================================

    /**************************************************************************
     * Function Name: ScanDirectory
     * Description: Scans directory for plugins
     * Parameters:
     *   in_strPath - Directory to scan
     * Returns: Number of plugins found
     **************************************************************************/
    TUInt32 ScanDirectory(const TString& in_strPath);

    /**************************************************************************
     * Function Name: AddSearchPath
     * Description: Adds a directory to search for plugins
     **************************************************************************/
    void AddSearchPath(const TString& in_strPath);

    /**************************************************************************
     * Function Name: GetSearchPaths
     * Description: Returns configured search paths
     **************************************************************************/
    [[nodiscard]] TVector<TString> GetSearchPaths() const;

    //=========================================================================
    // Plugin Access
    //=========================================================================

    /**************************************************************************
     * Function Name: GetPlugin
     * Description: Gets a plugin by ID
     * Parameters:
     *   in_strPluginId - Plugin identifier
     * Returns: Plugin pointer or nullptr if not found
     **************************************************************************/
    [[nodiscard]] IPlugin* GetPlugin(const TString& in_strPluginId);

    /**************************************************************************
     * Function Name: GetPluginInfo
     * Description: Gets plugin metadata
     **************************************************************************/
    [[nodiscard]] std::optional<SPluginInfo> GetPluginInfo(const TString& in_strPluginId) const;

    /**************************************************************************
     * Function Name: GetLoadedPlugins
     * Description: Returns list of loaded plugin IDs
     **************************************************************************/
    [[nodiscard]] TVector<TString> GetLoadedPlugins() const;

    /**************************************************************************
     * Function Name: GetPluginsByType
     * Description: Returns plugins of a specific type
     **************************************************************************/
    [[nodiscard]] TVector<TString> GetPluginsByType(EPluginType in_eType) const;

    /**************************************************************************
     * Function Name: IsPluginLoaded
     * Description: Checks if plugin is loaded
     **************************************************************************/
    [[nodiscard]] bool IsPluginLoaded(const TString& in_strPluginId) const;

    /**************************************************************************
     * Function Name: GetPluginCount
     * Description: Returns number of loaded plugins
     **************************************************************************/
    [[nodiscard]] TUInt32 GetPluginCount() const;

    //=========================================================================
    // Typed Plugin Access
    //=========================================================================

    template<typename T>
    T* GetPluginAs(const TString& in_strPluginId) {
        IPlugin* pPlugin = GetPlugin(in_strPluginId);
        return dynamic_cast<T*>(pPlugin);
    }

    // Plugin ABI version this host build is compatible with. A loaded
    // library whose GetPluginApiVersion() differs is rejected.
    static constexpr const char* kHostPluginApiVersion = "1.0";

private:
    CPluginManager() = default;
    ~CPluginManager();

    CResult LoadLibrary(const TString& in_strPath, void*& out_hLibrary);
    void UnloadLibrary(void* in_hLibrary);

    // Canonicalizes the path and rejects unsafe plugin files (non-regular
    // files, symlinks, and world-writable files/directories that an
    // attacker could replace to gain in-process code execution).
    CResult ValidatePluginFile(const TString& in_strPath, TString& out_strCanonicalPath) const;

    mutable std::mutex m_mutex;
    std::map<TString, SLoadedPlugin> m_mapPlugins;
    TVector<TString> m_vecSearchPaths;
};

} // namespace TestMATE
