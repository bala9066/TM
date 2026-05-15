/**************************************************************************
 * File Name: PluginManager.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Implementation of plugin manager
 * Requirements: REQ-PLG-021 to REQ-PLG-040
 **************************************************************************/

#include "PluginManager.h"
#include "utils/LogManager.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <filesystem>

namespace TestMATE {

CPluginManager& CPluginManager::GetInstance() {
    static CPluginManager instance;
    return instance;
}

CPluginManager::~CPluginManager() {
    UnloadAll();
}

CResult CPluginManager::LoadPlugin(const TString& in_strPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if file exists
    if (!std::filesystem::exists(in_strPath)) {
        return TESTMATE_FAILURE(EErrorCode::kFileNotFound,
                                "Plugin file not found: " + in_strPath);
    }

    // Load the library
    void* hLibrary = nullptr;
    auto result = LoadLibrary(in_strPath, hLibrary);
    if (result.IsFailure()) {
        return result;
    }

    // Get factory functions
    FPluginCreateFunc createFunc = nullptr;
    FPluginDestroyFunc destroyFunc = nullptr;

#ifdef _WIN32
    createFunc = reinterpret_cast<FPluginCreateFunc>(
        GetProcAddress(static_cast<HMODULE>(hLibrary), "CreatePlugin"));
    destroyFunc = reinterpret_cast<FPluginDestroyFunc>(
        GetProcAddress(static_cast<HMODULE>(hLibrary), "DestroyPlugin"));
#else
    createFunc = reinterpret_cast<FPluginCreateFunc>(
        dlsym(hLibrary, "CreatePlugin"));
    destroyFunc = reinterpret_cast<FPluginDestroyFunc>(
        dlsym(hLibrary, "DestroyPlugin"));
#endif

    if (!createFunc || !destroyFunc) {
        UnloadLibrary(hLibrary);
        return TESTMATE_FAILURE(EErrorCode::kPluginInvalid,
                                "Plugin missing required exports");
    }

    // Create plugin instance
    IPlugin* pPlugin = createFunc();
    if (!pPlugin) {
        UnloadLibrary(hLibrary);
        return TESTMATE_FAILURE(EErrorCode::kPluginInitFailed,
                                "Failed to create plugin instance");
    }

    // Get plugin info
    SPluginInfo info = pPlugin->GetInfo();

    // Check if already loaded
    if (m_mapPlugins.count(info.id) > 0) {
        destroyFunc(pPlugin);
        UnloadLibrary(hLibrary);
        return TESTMATE_FAILURE(EErrorCode::kAlreadyExists,
                                "Plugin already loaded: " + info.id);
    }

    // Initialize plugin
    result = pPlugin->Initialize();
    if (result.IsFailure()) {
        destroyFunc(pPlugin);
        UnloadLibrary(hLibrary);
        return result;
    }

    // Store loaded plugin
    SLoadedPlugin loaded;
    loaded.info = info;
    loaded.pPlugin = pPlugin;
    loaded.hLibrary = hLibrary;
    loaded.destroyFunc = destroyFunc;
    loaded.path = in_strPath;
    loaded.state = EPluginState::kInitialized;

    m_mapPlugins[info.id] = loaded;

    CLogManager::GetInstance().LogInfo("PluginManager",
        "Loaded plugin: {} v{}", info.name, info.version);

    return TESTMATE_SUCCESS();
}

CResult CPluginManager::UnloadPlugin(const TString& in_strPluginId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapPlugins.find(in_strPluginId);
    if (it == m_mapPlugins.end()) {
        return TESTMATE_FAILURE(EErrorCode::kNotFound,
                                "Plugin not found: " + in_strPluginId);
    }

    SLoadedPlugin& loaded = it->second;

    // Shutdown plugin
    if (loaded.pPlugin) {
        loaded.pPlugin->Shutdown();

        if (loaded.destroyFunc) {
            loaded.destroyFunc(loaded.pPlugin);
        }
    }

    // Unload library
    if (loaded.hLibrary) {
        UnloadLibrary(loaded.hLibrary);
    }

    CLogManager::GetInstance().LogInfo("PluginManager",
        "Unloaded plugin: {}", loaded.info.name);

    m_mapPlugins.erase(it);

    return TESTMATE_SUCCESS();
}

void CPluginManager::UnloadAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, loaded] : m_mapPlugins) {
        if (loaded.pPlugin) {
            loaded.pPlugin->Shutdown();
            if (loaded.destroyFunc) {
                loaded.destroyFunc(loaded.pPlugin);
            }
        }
        if (loaded.hLibrary) {
            UnloadLibrary(loaded.hLibrary);
        }
    }

    m_mapPlugins.clear();
}

TUInt32 CPluginManager::ScanDirectory(const TString& in_strPath) {
    if (!std::filesystem::exists(in_strPath)) {
        return 0;
    }

    TUInt32 count = 0;

#ifdef _WIN32
    const TString extension = ".dll";
#elif defined(__APPLE__)
    const TString extension = ".dylib";
#else
    const TString extension = ".so";
#endif

    for (const auto& entry : std::filesystem::directory_iterator(in_strPath)) {
        if (entry.is_regular_file() &&
            entry.path().extension() == extension) {

            auto result = LoadPlugin(entry.path().string());
            if (result.IsSuccess()) {
                ++count;
            }
        }
    }

    return count;
}

void CPluginManager::AddSearchPath(const TString& in_strPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_vecSearchPaths.push_back(in_strPath);
}

TVector<TString> CPluginManager::GetSearchPaths() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_vecSearchPaths;
}

IPlugin* CPluginManager::GetPlugin(const TString& in_strPluginId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapPlugins.find(in_strPluginId);
    if (it != m_mapPlugins.end()) {
        return it->second.pPlugin;
    }
    return nullptr;
}

std::optional<SPluginInfo> CPluginManager::GetPluginInfo(const TString& in_strPluginId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapPlugins.find(in_strPluginId);
    if (it != m_mapPlugins.end()) {
        return it->second.info;
    }
    return std::nullopt;
}

TVector<TString> CPluginManager::GetLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<TString> result;
    for (const auto& [id, loaded] : m_mapPlugins) {
        result.push_back(id);
    }
    return result;
}

TVector<TString> CPluginManager::GetPluginsByType(EPluginType in_eType) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TVector<TString> result;
    for (const auto& [id, loaded] : m_mapPlugins) {
        if (loaded.info.type == in_eType) {
            result.push_back(id);
        }
    }
    return result;
}

bool CPluginManager::IsPluginLoaded(const TString& in_strPluginId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mapPlugins.count(in_strPluginId) > 0;
}

TUInt32 CPluginManager::GetPluginCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<TUInt32>(m_mapPlugins.size());
}

CResult CPluginManager::LoadLibrary(const TString& in_strPath, void*& out_hLibrary) {
#ifdef _WIN32
    out_hLibrary = LoadLibraryA(in_strPath.c_str());
    if (!out_hLibrary) {
        return TESTMATE_FAILURE(EErrorCode::kPluginLoadFailed,
                                "Failed to load library: " + in_strPath);
    }
#else
    out_hLibrary = dlopen(in_strPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!out_hLibrary) {
        TString error = dlerror() ? dlerror() : "Unknown error";
        return TESTMATE_FAILURE(EErrorCode::kPluginLoadFailed,
                                "Failed to load library: " + error);
    }
#endif

    return TESTMATE_SUCCESS();
}

void CPluginManager::UnloadLibrary(void* in_hLibrary) {
    if (!in_hLibrary) {
        return;
    }

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(in_hLibrary));
#else
    dlclose(in_hLibrary);
#endif
}

} // namespace TestMATE
