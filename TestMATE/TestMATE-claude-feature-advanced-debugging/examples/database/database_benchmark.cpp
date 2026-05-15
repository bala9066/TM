/**************************************************************************
 * File Name: database_benchmark.cpp
 * Description: Performance benchmark tool for TestMATE database backends
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Benchmarks and compares performance across different database backends:
 *   - Insert performance (single and batch)
 *   - Query performance
 *   - Transaction performance
 *   - Concurrent access performance
 *
 * Build:
 *   cmake -DTESTMATE_POSTGRESQL_SUPPORT=ON -DTESTMATE_MYSQL_SUPPORT=ON ..
 *   cmake --build .
 *
 * Usage:
 *   ./database_benchmark [options]
 *
 *   Options:
 *     --sqlite <path>        SQLite database file
 *     --postgresql <conn>    PostgreSQL connection (host/database)
 *     --mysql <conn>         MySQL connection (host/database)
 *     --records <n>          Number of records for benchmark (default: 1000)
 *     --batch-size <n>       Batch size for batch inserts (default: 100)
 *
 *   Examples:
 *     ./database_benchmark --sqlite bench.db --records 10000
 *     ./database_benchmark --sqlite bench.db --postgresql localhost/testmate --records 5000
 *     ./database_benchmark --sqlite bench.db --mysql localhost/testmate --batch-size 500
 **************************************************************************/

#include "database/DataStoreFactory.h"
#include "database/IDataStore.h"
#include "database/SqliteDataStore.h"

#ifdef TESTMATE_POSTGRESQL_SUPPORT
#include "database/PostgreSqlDataStore.h"
#endif

#ifdef TESTMATE_MYSQL_SUPPORT
#include "database/MySqlDataStore.h"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <algorithm>

using namespace TestMATE;

/**************************************************************************
 * Benchmark Configuration
 **************************************************************************/
struct SBenchmarkConfig {
    int recordCount{1000};
    int batchSize{100};
    int queryIterations{100};
};

/**************************************************************************
 * Benchmark Results
 **************************************************************************/
struct SBenchmarkResult {
    std::string backendName;

    // Insert benchmarks
    double singleInsertAvgMs{0.0};
    double batchInsertTotalMs{0.0};
    double batchInsertThroughput{0.0};  // records/sec

    // Query benchmarks
    double pointQueryAvgMs{0.0};
    double rangeQueryAvgMs{0.0};

    // Transaction benchmarks
    double transactionCommitMs{0.0};
    double transactionRollbackMs{0.0};
};

/**************************************************************************
 * Test Data Generator
 **************************************************************************/
class CTestDataGenerator {
public:
    CTestDataGenerator() : m_rng(std::random_device{}()) {}

    STestDataRecord Generate(int index) {
        STestDataRecord data;
        data.testId = "BENCH-" + std::to_string(index);
        data.sequenceId = "SEQ-" + std::to_string(m_distribution(m_rng) % 10);
        data.deviceId = "SN-" + std::to_string(100000 + index);
        data.lotId = "LOT-BENCH-" + std::to_string(m_distribution(m_rng) % 5);
        data.verdict = (m_distribution(m_rng) % 10 == 0) ?
                      ETestVerdict::kFail : ETestVerdict::kPass;
        data.durationMs = 500 + (m_distribution(m_rng) % 5000);
        data.operatorName = m_operators[m_distribution(m_rng) % m_operators.size()];
        return data;
    }

private:
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_distribution{0, 10000};
    std::vector<std::string> m_operators{"Alice", "Bob", "Charlie", "Diana"};
};

/**************************************************************************
 * Database Connection Helper
 **************************************************************************/
struct SConnectionInfo {
    EDataStoreType type;
    std::string host;
    int port;
    std::string database;
    std::string user;
    std::string password;
    std::string filePath;
};

std::unique_ptr<IDataStore> OpenDatabase(const SConnectionInfo& info) {
    std::unique_ptr<IDataStore> dataStore;

    switch (info.type) {
        case EDataStoreType::kSQLite: {
            dataStore = CDataStoreFactory::CreateSQLite();
            auto result = dataStore->Open(info.filePath);
            if (!result.IsSuccess()) {
                std::cerr << "Failed to open SQLite: " << result.GetMessage() << "\n";
                return nullptr;
            }
            static_cast<CSqliteDataStore*>(dataStore.get())->InitializeSchema();
            break;
        }

#ifdef TESTMATE_POSTGRESQL_SUPPORT
        case EDataStoreType::kPostgreSQL: {
            dataStore = CDataStoreFactory::CreatePostgreSQL();
            auto pgStore = dynamic_cast<CPostgreSqlDataStore*>(dataStore.get());
            SPostgreSqlConfig config;
            config.host = info.host;
            config.port = info.port;
            config.database = info.database;
            config.user = info.user;
            config.password = info.password;
            auto result = pgStore->Open(config);
            if (!result.IsSuccess()) {
                std::cerr << "Failed to open PostgreSQL: " << result.GetMessage() << "\n";
                return nullptr;
            }
            pgStore->InitializeSchema();
            break;
        }
#endif

#ifdef TESTMATE_MYSQL_SUPPORT
        case EDataStoreType::kMySQL: {
            dataStore = CDataStoreFactory::CreateMySQL();
            auto mysqlStore = dynamic_cast<CMySqlDataStore*>(dataStore.get());
            SMySqlConfig config;
            config.host = info.host;
            config.port = info.port;
            config.database = info.database;
            config.user = info.user;
            config.password = info.password;
            auto result = mysqlStore->Open(config);
            if (!result.IsSuccess()) {
                std::cerr << "Failed to open MySQL: " << result.GetMessage() << "\n";
                return nullptr;
            }
            mysqlStore->InitializeSchema();
            break;
        }
#endif

        default:
            return nullptr;
    }

    return dataStore;
}

/**************************************************************************
 * Benchmark Functions
 **************************************************************************/

// Benchmark single insert performance
double BenchmarkSingleInsert(IDataStore* dataStore, const SBenchmarkConfig& config) {
    CTestDataGenerator generator;
    std::vector<double> times;

    // Run 100 single inserts
    for (int i = 0; i < 100; ++i) {
        auto testData = generator.Generate(i);

        auto start = std::chrono::high_resolution_clock::now();
        dataStore->SaveTestData(testData);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        times.push_back(duration / 1000.0);  // Convert to ms
    }

    // Calculate average
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    return sum / times.size();
}

// Benchmark batch insert performance
std::pair<double, double> BenchmarkBatchInsert(IDataStore* dataStore, const SBenchmarkConfig& config) {
    CTestDataGenerator generator;

    auto start = std::chrono::high_resolution_clock::now();

    // Use transaction for batch
    dataStore->BeginTransaction();

    for (int i = 0; i < config.recordCount; ++i) {
        auto testData = generator.Generate(i);
        dataStore->SaveTestData(testData);

        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << config.recordCount << "\r" << std::flush;
        }
    }

    dataStore->CommitTransaction();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << std::string(50, ' ') << "\r";  // Clear progress line

    double throughput = config.recordCount * 1000.0 / duration;  // records/sec
    return {static_cast<double>(duration), throughput};
}

// Benchmark point query performance
double BenchmarkPointQuery(IDataStore* dataStore, const SBenchmarkConfig& config) {
    std::vector<double> times;

    for (int i = 0; i < config.queryIterations; ++i) {
        std::string testId = "BENCH-" + std::to_string(i);
        STestDataRecord result;

        auto start = std::chrono::high_resolution_clock::now();
        dataStore->GetTestData(testId, result);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        times.push_back(duration / 1000.0);
    }

    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    return sum / times.size();
}

// Benchmark range query performance
double BenchmarkRangeQuery(IDataStore* dataStore, const SBenchmarkConfig& config) {
    std::vector<double> times;

    for (int i = 0; i < 10; ++i) {
        std::string lotId = "LOT-BENCH-" + std::to_string(i % 5);
        std::vector<STestDataRecord> results;

        auto start = std::chrono::high_resolution_clock::now();
        dataStore->GetTestDataByLot(lotId, results);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        times.push_back(duration / 1000.0);
    }

    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    return sum / times.size();
}

// Benchmark transaction commit
double BenchmarkTransactionCommit(IDataStore* dataStore) {
    CTestDataGenerator generator;

    auto start = std::chrono::high_resolution_clock::now();

    dataStore->BeginTransaction();
    for (int i = 0; i < 100; ++i) {
        auto testData = generator.Generate(10000 + i);
        dataStore->SaveTestData(testData);
    }
    dataStore->CommitTransaction();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return static_cast<double>(duration);
}

// Benchmark transaction rollback
double BenchmarkTransactionRollback(IDataStore* dataStore) {
    CTestDataGenerator generator;

    auto start = std::chrono::high_resolution_clock::now();

    dataStore->BeginTransaction();
    for (int i = 0; i < 100; ++i) {
        auto testData = generator.Generate(20000 + i);
        dataStore->SaveTestData(testData);
    }
    dataStore->RollbackTransaction();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return static_cast<double>(duration);
}

// Run complete benchmark suite
SBenchmarkResult RunBenchmark(const std::string& name,
                              std::unique_ptr<IDataStore>& dataStore,
                              const SBenchmarkConfig& config) {
    std::cout << "\nBenchmarking " << name << "...\n";
    std::cout << "=====================================\n";

    SBenchmarkResult result;
    result.backendName = name;

    std::cout << "1. Single insert performance...      ";
    result.singleInsertAvgMs = BenchmarkSingleInsert(dataStore.get(), config);
    std::cout << result.singleInsertAvgMs << " ms/record\n";

    std::cout << "2. Batch insert performance...\n";
    auto [batchTime, throughput] = BenchmarkBatchInsert(dataStore.get(), config);
    result.batchInsertTotalMs = batchTime;
    result.batchInsertThroughput = throughput;
    std::cout << "   Total time: " << batchTime << " ms\n";
    std::cout << "   Throughput: " << throughput << " records/sec\n";

    std::cout << "3. Point query performance...        ";
    result.pointQueryAvgMs = BenchmarkPointQuery(dataStore.get(), config);
    std::cout << result.pointQueryAvgMs << " ms/query\n";

    std::cout << "4. Range query performance...        ";
    result.rangeQueryAvgMs = BenchmarkRangeQuery(dataStore.get(), config);
    std::cout << result.rangeQueryAvgMs << " ms/query\n";

    std::cout << "5. Transaction commit...             ";
    result.transactionCommitMs = BenchmarkTransactionCommit(dataStore.get());
    std::cout << result.transactionCommitMs << " ms\n";

    std::cout << "6. Transaction rollback...           ";
    result.transactionRollbackMs = BenchmarkTransactionRollback(dataStore.get());
    std::cout << result.transactionRollbackMs << " ms\n";

    return result;
}

// Print comparison table
void PrintComparison(const std::vector<SBenchmarkResult>& results) {
    std::cout << "\n\n";
    std::cout << "========================================\n";
    std::cout << "         BENCHMARK COMPARISON\n";
    std::cout << "========================================\n\n";

    std::cout << std::left;
    std::cout << std::setw(25) << "Benchmark";
    for (const auto& result : results) {
        std::cout << std::setw(15) << result.backendName;
    }
    std::cout << "\n";
    std::cout << std::string(25 + results.size() * 15, '-') << "\n";

    // Single insert
    std::cout << std::setw(25) << "Single Insert (ms)";
    for (const auto& result : results) {
        std::cout << std::setw(15) << std::fixed << std::setprecision(3) << result.singleInsertAvgMs;
    }
    std::cout << "\n";

    // Batch throughput
    std::cout << std::setw(25) << "Batch Throughput (rec/s)";
    for (const auto& result : results) {
        std::cout << std::setw(15) << std::fixed << std::setprecision(0) << result.batchInsertThroughput;
    }
    std::cout << "\n";

    // Point query
    std::cout << std::setw(25) << "Point Query (ms)";
    for (const auto& result : results) {
        std::cout << std::setw(15) << std::fixed << std::setprecision(3) << result.pointQueryAvgMs;
    }
    std::cout << "\n";

    // Range query
    std::cout << std::setw(25) << "Range Query (ms)";
    for (const auto& result : results) {
        std::cout << std::setw(15) << std::fixed << std::setprecision(3) << result.rangeQueryAvgMs;
    }
    std::cout << "\n";

    // Transaction commit
    std::cout << std::setw(25) << "Transaction Commit (ms)";
    for (const auto& result : results) {
        std::cout << std::setw(15) << std::fixed << std::setprecision(1) << result.transactionCommitMs;
    }
    std::cout << "\n";

    // Transaction rollback
    std::cout << std::setw(25) << "Transaction Rollback (ms)";
    for (const auto& result : results) {
        std::cout << std::setw(15) << std::fixed << std::setprecision(1) << result.transactionRollbackMs;
    }
    std::cout << "\n";
}

/**************************************************************************
 * Main
 **************************************************************************/
int main(int argc, char* argv[]) {
    std::cout << "TestMATE Database Performance Benchmark\n";
    std::cout << "========================================\n";

    SBenchmarkConfig config;
    std::vector<std::pair<std::string, SConnectionInfo>> databases;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--records" && i + 1 < argc) {
            config.recordCount = std::stoi(argv[++i]);
        } else if (arg == "--batch-size" && i + 1 < argc) {
            config.batchSize = std::stoi(argv[++i]);
        } else if (arg == "--sqlite" && i + 1 < argc) {
            SConnectionInfo info;
            info.type = EDataStoreType::kSQLite;
            info.filePath = argv[++i];
            databases.push_back({"SQLite", info});
        } else if (arg == "--postgresql" && i + 1 < argc) {
            SConnectionInfo info;
            info.type = EDataStoreType::kPostgreSQL;
            std::string conn = argv[++i];
            size_t slashPos = conn.find('/');
            if (slashPos != std::string::npos) {
                info.host = conn.substr(0, slashPos);
                info.database = conn.substr(slashPos + 1);
            }
            info.port = 5432;
            std::cout << "PostgreSQL User: ";
            std::getline(std::cin, info.user);
            std::cout << "PostgreSQL Password: ";
            std::getline(std::cin, info.password);
            databases.push_back({"PostgreSQL", info});
        } else if (arg == "--mysql" && i + 1 < argc) {
            SConnectionInfo info;
            info.type = EDataStoreType::kMySQL;
            std::string conn = argv[++i];
            size_t slashPos = conn.find('/');
            if (slashPos != std::string::npos) {
                info.host = conn.substr(0, slashPos);
                info.database = conn.substr(slashPos + 1);
            }
            info.port = 3306;
            std::cout << "MySQL User: ";
            std::getline(std::cin, info.user);
            std::cout << "MySQL Password: ";
            std::getline(std::cin, info.password);
            databases.push_back({"MySQL", info});
        }
    }

    if (databases.empty()) {
        std::cout << "\nUsage: " << argv[0] << " [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --sqlite <path>        SQLite database file\n";
        std::cout << "  --postgresql <conn>    PostgreSQL connection (host/database)\n";
        std::cout << "  --mysql <conn>         MySQL connection (host/database)\n";
        std::cout << "  --records <n>          Number of records (default: 1000)\n";
        std::cout << "  --batch-size <n>       Batch size (default: 100)\n\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " --sqlite bench.db --records 10000\n";
        std::cout << "  " << argv[0] << " --sqlite bench.db --postgresql localhost/testmate\n";
        return 1;
    }

    std::cout << "\nBenchmark Configuration:\n";
    std::cout << "  Records: " << config.recordCount << "\n";
    std::cout << "  Batch size: " << config.batchSize << "\n";
    std::cout << "  Query iterations: " << config.queryIterations << "\n";

    // Run benchmarks
    std::vector<SBenchmarkResult> results;

    for (auto& [name, connInfo] : databases) {
        auto dataStore = OpenDatabase(connInfo);
        if (!dataStore) {
            std::cerr << "Skipping " << name << " (connection failed)\n";
            continue;
        }

        auto result = RunBenchmark(name, dataStore, config);
        results.push_back(result);

        dataStore->Close();
    }

    // Print comparison
    if (results.size() > 1) {
        PrintComparison(results);
    }

    std::cout << "\n\n✓ Benchmark complete!\n\n";
    return 0;
}
