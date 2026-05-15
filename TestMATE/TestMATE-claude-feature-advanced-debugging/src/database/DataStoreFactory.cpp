/**************************************************************************
 * File Name: DataStoreFactory.cpp
 * Description: Database factory implementation
 **************************************************************************/

#include "database/DataStoreFactory.h"
#include "database/SqliteDataStore.h"
#include "database/PostgreSqlDataStore.h"
#include "database/MySqlDataStore.h"
#include "utils/LogManager.h"
#include <algorithm>

namespace TestMATE {

/**************************************************************************
 * Create database instance by type
 **************************************************************************/

std::unique_ptr<IDataStore> CDataStoreFactory::Create(EDataStoreType in_type) {
    switch (in_type) {
        case EDataStoreType::kSqlite:
            return CreateSQLite();

        case EDataStoreType::kPostgreSQL:
            return CreatePostgreSQL();

        case EDataStoreType::kMySQL:
            return CreateMySQL();

        default:
            LOG_WARNING("DataStoreFactory", "Unknown database type, defaulting to SQLite");
            return CreateSQLite();
    }
}

/**************************************************************************
 * Convenience methods
 **************************************************************************/

std::unique_ptr<IDataStore> CDataStoreFactory::CreateSQLite() {
    LOG_INFO("DataStoreFactory", "Creating SQLite data store");
    return std::make_unique<CSqliteDataStore>();
}

std::unique_ptr<IDataStore> CDataStoreFactory::CreatePostgreSQL() {
#ifdef TESTMATE_POSTGRESQL_SUPPORT
    LOG_INFO("DataStoreFactory", "Creating PostgreSQL data store");
    return std::make_unique<CPostgreSqlDataStore>();
#else
    LOG_ERROR("DataStoreFactory", "PostgreSQL support not compiled in");
    LOG_WARNING("DataStoreFactory", "Falling back to SQLite");
    return CreateSQLite();
#endif
}

std::unique_ptr<IDataStore> CDataStoreFactory::CreateMySQL() {
#ifdef TESTMATE_MYSQL_SUPPORT
    LOG_INFO("DataStoreFactory", "Creating MySQL data store");
    return std::make_unique<CMySqlDataStore>();
#else
    LOG_ERROR("DataStoreFactory", "MySQL support not compiled in");
    LOG_WARNING("DataStoreFactory", "Falling back to SQLite");
    return CreateSQLite();
#endif
}

/**************************************************************************
 * Auto-detect from connection string
 **************************************************************************/

std::unique_ptr<IDataStore> CDataStoreFactory::CreateFromConnectionString(
    const TString& in_strConnectionString) {

    EDataStoreType type = DetectTypeFromConnectionString(in_strConnectionString);

    LOG_INFO("DataStoreFactory", "Detected database type: {}", GetBackendName(type));

    return Create(type);
}

/**************************************************************************
 * Get available backends
 **************************************************************************/

std::vector<EDataStoreType> CDataStoreFactory::GetAvailableBackends() {
    std::vector<EDataStoreType> backends;

    // SQLite is always available
    backends.push_back(EDataStoreType::kSqlite);

#ifdef TESTMATE_POSTGRESQL_SUPPORT
    backends.push_back(EDataStoreType::kPostgreSQL);
#endif

#ifdef TESTMATE_MYSQL_SUPPORT
    backends.push_back(EDataStoreType::kMySQL);
#endif

    return backends;
}

/**************************************************************************
 * Check if backend is available
 **************************************************************************/

bool CDataStoreFactory::IsBackendAvailable(EDataStoreType in_type) {
    switch (in_type) {
        case EDataStoreType::kSqlite:
            return true;  // Always available

        case EDataStoreType::kPostgreSQL:
#ifdef TESTMATE_POSTGRESQL_SUPPORT
            return true;
#else
            return false;
#endif

        case EDataStoreType::kMySQL:
#ifdef TESTMATE_MYSQL_SUPPORT
            return true;
#else
            return false;
#endif

        default:
            return false;
    }
}

/**************************************************************************
 * Get backend information
 **************************************************************************/

TString CDataStoreFactory::GetBackendName(EDataStoreType in_type) {
    switch (in_type) {
        case EDataStoreType::kSqlite:     return "SQLite";
        case EDataStoreType::kPostgreSQL: return "PostgreSQL";
        case EDataStoreType::kMySQL:      return "MySQL/MariaDB";
        default:                          return "Unknown";
    }
}

TString CDataStoreFactory::GetBackendDescription(EDataStoreType in_type) {
    switch (in_type) {
        case EDataStoreType::kSqlite:
            return "Embedded database, zero configuration, single-file storage";

        case EDataStoreType::kPostgreSQL:
            return "Enterprise-grade database with advanced features, "
                   "excellent for concurrent access and complex queries";

        case EDataStoreType::kMySQL:
            return "Popular open-source database, excellent for web applications "
                   "and high-volume operations";

        default:
            return "Unknown database type";
    }
}

/**************************************************************************
 * Get recommended backend
 **************************************************************************/

EDataStoreType CDataStoreFactory::GetRecommendedBackend(EUseCase in_useCase) {
    switch (in_useCase) {
        case EUseCase::kEmbedded:
            // SQLite is perfect for embedded/standalone apps
            return EDataStoreType::kSqlite;

        case EUseCase::kEnterpriseSmall:
            // PostgreSQL or MySQL work well for small enterprise
            // Prefer PostgreSQL if available, otherwise MySQL, fallback to SQLite
#ifdef TESTMATE_POSTGRESQL_SUPPORT
            return EDataStoreType::kPostgreSQL;
#elif defined(TESTMATE_MYSQL_SUPPORT)
            return EDataStoreType::kMySQL;
#else
            return EDataStoreType::kSqlite;
#endif

        case EUseCase::kEnterpriseLarge:
        case EUseCase::kHighAvailability:
            // PostgreSQL is better for large enterprise and HA
#ifdef TESTMATE_POSTGRESQL_SUPPORT
            return EDataStoreType::kPostgreSQL;
#elif defined(TESTMATE_MYSQL_SUPPORT)
            return EDataStoreType::kMySQL;
#else
            LOG_WARNING("DataStoreFactory", "Recommended backend not available, using SQLite");
            return EDataStoreType::kSqlite;
#endif

        case EUseCase::kCloudDeployment:
            // Both PostgreSQL and MySQL work well in cloud
#ifdef TESTMATE_POSTGRESQL_SUPPORT
            return EDataStoreType::kPostgreSQL;
#elif defined(TESTMATE_MYSQL_SUPPORT)
            return EDataStoreType::kMySQL;
#else
            return EDataStoreType::kSqlite;
#endif

        default:
            return EDataStoreType::kSqlite;
    }
}

/**************************************************************************
 * Private helper methods
 **************************************************************************/

EDataStoreType CDataStoreFactory::DetectTypeFromConnectionString(
    const TString& in_strConnectionString) {

    // Convert to lowercase for comparison
    TString lower = in_strConnectionString;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Check for PostgreSQL
    if (lower.find("postgresql://") == 0 || lower.find("postgres://") == 0) {
        return EDataStoreType::kPostgreSQL;
    }

    // Check for MySQL/MariaDB
    if (lower.find("mysql://") == 0 || lower.find("mariadb://") == 0) {
        return EDataStoreType::kMySQL;
    }

    // Check for SQLite file path
    if (lower.find("file://") == 0 ||
        lower.find(".db") != TString::npos ||
        lower.find(".sqlite") != TString::npos) {
        return EDataStoreType::kSqlite;
    }

    // If it looks like a file path (no protocol), assume SQLite
    if (lower.find("://") == TString::npos) {
        return EDataStoreType::kSqlite;
    }

    // Default to SQLite
    LOG_WARNING("DataStoreFactory", "Could not detect database type from connection string, defaulting to SQLite");
    return EDataStoreType::kSqlite;
}

/**************************************************************************
 * Utility Functions
 **************************************************************************/

EDataStoreType StringToDataStoreType(const TString& in_str) {
    TString lower = in_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "sqlite" || lower == "sqlite3") {
        return EDataStoreType::kSqlite;
    }
    else if (lower == "postgresql" || lower == "postgres" || lower == "pg" || lower == "pgsql") {
        return EDataStoreType::kPostgreSQL;
    }
    else if (lower == "mysql" || lower == "mariadb") {
        return EDataStoreType::kMySQL;
    }
    else {
        LOG_WARNING("DataStoreFactory", "Unknown database type string: {}, defaulting to SQLite", in_str);
        return EDataStoreType::kSqlite;
    }
}

TString DataStoreTypeToString(EDataStoreType in_type) {
    return CDataStoreFactory::GetBackendName(in_type);
}

} // namespace TestMATE
