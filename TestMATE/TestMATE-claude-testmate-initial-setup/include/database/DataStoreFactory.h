/**************************************************************************
 * File Name: DataStoreFactory.h
 * Description: Factory for creating database backend instances
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Provides a factory pattern for creating appropriate database backend
 *   instances based on configuration or connection string.
 *
 * Usage Example:
 *
 *   // Create SQLite database (default)
 *   auto db = CDataStoreFactory::Create(EDataStoreType::kSQLite);
 *   db->Open("test_data.db");
 *
 *   // Create PostgreSQL database
 *   auto pgDb = CDataStoreFactory::CreatePostgreSQL();
 *   SPostgreSqlConfig config;
 *   config.host = "localhost";
 *   config.database = "testmate";
 *   pgDb->Open(config);
 *
 *   // Auto-detect from connection string
 *   auto db = CDataStoreFactory::CreateFromConnectionString(
 *       "postgresql://user:pass@localhost/testmate");
 *
 **************************************************************************/

#pragma once

#include "database/IDataStore.h"
#include "utils/TestMateTypes.h"
#include <memory>
#include <map>

namespace TestMATE {

/**************************************************************************
 * Enum: EDataStoreType
 * Description: Supported database backend types
 **************************************************************************/
enum class EDataStoreType {
    kSQLite,       // SQLite (embedded, default)
    kPostgreSQL,   // PostgreSQL
    kMySQL,        // MySQL/MariaDB
    kMariaDB = kMySQL  // Alias for MySQL
};

/**************************************************************************
 * Class: CDataStoreFactory
 * Description: Factory for creating database instances
 **************************************************************************/
class CDataStoreFactory {
public:
    /**************************************************************************
     * Create database instance by type
     **************************************************************************/
    static std::unique_ptr<IDataStore> Create(EDataStoreType in_type);

    /**************************************************************************
     * Convenience methods for specific backends
     **************************************************************************/
    static std::unique_ptr<IDataStore> CreateSQLite();
    static std::unique_ptr<IDataStore> CreatePostgreSQL();
    static std::unique_ptr<IDataStore> CreateMySQL();

    /**************************************************************************
     * Auto-detect database type from connection string
     *
     * Supported formats:
     *   SQLite:     "file:///path/to/database.db"
     *              "/path/to/database.db"
     *
     *   PostgreSQL: "postgresql://user:pass@host:port/database"
     *              "postgres://user:pass@host:port/database"
     *
     *   MySQL:      "mysql://user:pass@host:port/database"
     *              "mariadb://user:pass@host:port/database"
     **************************************************************************/
    static std::unique_ptr<IDataStore> CreateFromConnectionString(
        const TString& in_strConnectionString);

    /**************************************************************************
     * Get list of available database backends
     **************************************************************************/
    static std::vector<EDataStoreType> GetAvailableBackends();

    /**************************************************************************
     * Check if a backend is available (compiled in)
     **************************************************************************/
    static bool IsBackendAvailable(EDataStoreType in_type);

    /**************************************************************************
     * Get backend name as string
     **************************************************************************/
    static TString GetBackendName(EDataStoreType in_type);

    /**************************************************************************
     * Get backend description
     **************************************************************************/
    static TString GetBackendDescription(EDataStoreType in_type);

    /**************************************************************************
     * Get recommended backend for use case
     **************************************************************************/
    enum class EUseCase {
        kEmbedded,          // Embedded/standalone applications
        kEnterpriseSmall,   // Small enterprise (< 10 concurrent users)
        kEnterpriseLarge,   // Large enterprise (> 10 concurrent users)
        kCloudDeployment,   // Cloud-based deployment
        kHighAvailability   // High availability requirement
    };

    static EDataStoreType GetRecommendedBackend(EUseCase in_useCase);

private:
    // Private constructor (static class)
    CDataStoreFactory() = delete;

    // Helper methods
    static EDataStoreType DetectTypeFromConnectionString(
        const TString& in_strConnectionString);
};

/**************************************************************************
 * Utility Functions
 **************************************************************************/

// Convert string to database type
EDataStoreType StringToDataStoreType(const TString& in_str);

// Convert database type to string
TString DataStoreTypeToString(EDataStoreType in_type);

} // namespace TestMATE
