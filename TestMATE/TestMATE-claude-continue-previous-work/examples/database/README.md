# TestMATE Database Examples and Tools

This directory contains comprehensive examples and utilities for working with TestMATE's database backends.

## Table of Contents

- [Database Setup](#database-setup)
- [Usage Examples](#usage-examples)
- [Database Tools](#database-tools)
- [Supported Backends](#supported-backends)
- [Building](#building)

## Database Setup

### PostgreSQL Setup

1. Install PostgreSQL (version 12 or later recommended)
2. Run the setup script:
   ```bash
   psql -U postgres -f setup_postgresql.sql
   ```

This creates:
- Database: `testmate_demo`
- User: `testmate_user` / `testmate_pass`
- Tables: `test_results`, `test_step_results`, `measurements`, `schema_version`
- Views: `test_summary`, `operator_stats`
- Functions: `cleanup_old_tests()`

### MySQL/MariaDB Setup

1. Install MySQL or MariaDB (version 8.0 / 10.5 or later recommended)
2. Run the setup script:
   ```bash
   mysql -u root -p < setup_mysql.sql
   ```

This creates:
- Database: `testmate_demo`
- User: `testmate_user` / `testmate_pass` (localhost + remote access)
- Tables: `test_results`, `test_step_results`, `measurements`, `schema_version`
- Views: `test_summary`, `operator_stats`
- Stored procedure: `cleanup_old_tests()`
- Event scheduler: `auto_cleanup_old_tests` (disabled by default)

## Usage Examples

### PostgreSQL Example

Demonstrates PostgreSQL-specific features:

```bash
./postgresql_example
```

**Features demonstrated:**
- Basic connection setup
- Saving test data
- **Transactions with savepoints** (PostgreSQL nested transactions)
- High-performance batch operations
- Querying by lot with yield calculation
- Database maintenance (VACUUM, ANALYZE)
- Connection statistics

**Key highlights:**
```cpp
// Savepoint usage (PostgreSQL-specific)
dataStore->BeginTransaction();
dataStore->SaveTestData(test1);
dataStore->CreateSavepoint("after_test1");  // Checkpoint
dataStore->SaveTestData(test2);
dataStore->RollbackToSavepoint("after_test1");  // Undo test2 only
dataStore->CommitTransaction();  // Only test1 is saved
```

### MySQL Example

Demonstrates MySQL/MariaDB-specific features:

```bash
./mysql_example
```

**Features demonstrated:**
- Basic connection with auto-reconnect
- Saving test data
- ACID transactions with rollback
- Batch insert performance testing
- Querying and yield analysis
- **Table maintenance operations** (OPTIMIZE, ANALYZE, CHECK)
- Server version and statistics
- Connection string auto-detection

**Key highlights:**
```cpp
// Table optimization (MySQL-specific)
dataStore->OptimizeTable("test_results");  // Reclaim space, defragment
dataStore->AnalyzeTable("test_results");   // Update index statistics
dataStore->CheckTable("test_results");     // Verify integrity

// Auto-reconnect configuration
config.autoReconnect = true;  // Automatically reconnect on connection loss
```

## Database Tools

### Migration Tool

Migrate data between different database backends.

**Usage:**
```bash
./database_migration_tool <source> <destination>
```

**Examples:**
```bash
# SQLite → PostgreSQL
./database_migration_tool sqlite:test_data.db postgresql:localhost/testmate

# SQLite → MySQL
./database_migration_tool sqlite:test_data.db mysql:localhost/testmate

# PostgreSQL → MySQL
./database_migration_tool postgresql:server1/db1 mysql:server2/db2
```

**Connection String Formats:**
- SQLite: `sqlite:<path>`
- PostgreSQL: `postgresql:<host>[:<port>]/<database>`
- MySQL: `mysql:<host>[:<port>]/<database>`

**Features:**
- Batch migration with transaction support
- Progress indication
- Error reporting
- Performance metrics (records/sec)

### Backup Tool

Create backups in JSON or SQL format.

**Usage:**
```bash
./database_backup_tool <database> <output_file> [--format=json|sql]
```

**Examples:**
```bash
# Backup SQLite to JSON
./database_backup_tool sqlite:test_data.db backup.json

# Backup PostgreSQL to SQL
./database_backup_tool postgresql:localhost/testmate backup.sql --format=sql

# Backup MySQL to JSON
./database_backup_tool mysql:localhost/testmate backup_$(date +%Y%m%d).json
```

**Backup Formats:**
- **JSON**: Portable, human-readable, cross-database compatible
- **SQL**: Direct SQL INSERT statements, fast restore

**Features:**
- Progress reporting
- Metadata inclusion (timestamp, version, record count)
- Automatic format detection from file extension

### Restore Tool

Restore databases from backup files.

**Usage:**
```bash
./database_restore_tool <database> <backup_file> [--format=json|sql]
```

**Examples:**
```bash
# Restore from JSON
./database_restore_tool sqlite:restored.db backup.json

# Restore from SQL
./database_restore_tool postgresql:localhost/testmate backup.sql --format=sql

# Restore with format auto-detection
./database_restore_tool mysql:localhost/testmate backup_20250122.json
```

**Features:**
- Automatic format detection (.json, .sql)
- Safety confirmation prompt
- Transaction-based restore (atomic operation)
- Error reporting with counts

**Safety Note:**
The restore tool **adds** data to the database. Make sure the database is empty or you intend to merge data.

## Supported Backends

| Backend | Type | Use Case | Connection Pooling | Transactions | Savepoints |
|---------|------|----------|-------------------|--------------|------------|
| **SQLite** | Embedded | Standalone apps, development | No | Yes | No |
| **PostgreSQL** | Client-Server | Enterprise, high concurrency | Yes* | Yes | **Yes** |
| **MySQL/MariaDB** | Client-Server | Web apps, cloud deployment | Yes* | Yes | No |

\* Connection pooling support coming soon

## Building

### Build with PostgreSQL support:

```bash
cmake -DTESTMATE_BUILD_EXAMPLES=ON -DTESTMATE_POSTGRESQL_SUPPORT=ON ..
cmake --build .
```

### Build with MySQL support:

```bash
cmake -DTESTMATE_BUILD_EXAMPLES=ON -DTESTMATE_MYSQL_SUPPORT=ON ..
cmake --build .
```

### Build with all backends:

```bash
cmake -DTESTMATE_BUILD_EXAMPLES=ON \
      -DTESTMATE_POSTGRESQL_SUPPORT=ON \
      -DTESTMATE_MYSQL_SUPPORT=ON ..
cmake --build .
```

### Prerequisites:

**For PostgreSQL:**
- PostgreSQL client library (`libpq-dev` on Ubuntu, `postgresql-devel` on RHEL)
- CMake will automatically detect it via `find_package(PostgreSQL)`

**For MySQL:**
- MySQL or MariaDB client library
  - `libmysqlclient-dev` (Ubuntu/Debian)
  - `mysql-devel` (RHEL/CentOS)
  - `mariadb-connector-c-devel` (for MariaDB)
- CMake will automatically detect it via `find_package(MySQL)` or `find_package(MariaDB)`

## Common Workflows

### Development to Production Migration

1. **Develop with SQLite:**
   ```bash
   # Use SQLite during development
   auto db = CDataStoreFactory::CreateSQLite();
   db->Open("dev_data.db");
   ```

2. **Backup development data:**
   ```bash
   ./database_backup_tool sqlite:dev_data.db dev_backup.json
   ```

3. **Setup production database:**
   ```bash
   # PostgreSQL
   psql -U postgres -f setup_postgresql.sql
   ```

4. **Migrate to production:**
   ```bash
   ./database_migration_tool sqlite:dev_data.db postgresql:prodserver/testmate_prod
   ```

### Regular Backup Schedule

```bash
#!/bin/bash
# Daily backup script
DATE=$(date +%Y%m%d_%H%M%S)
./database_backup_tool postgresql:localhost/testmate "backups/testmate_$DATE.json"

# Keep last 30 days
find backups/ -name "testmate_*.json" -mtime +30 -delete
```

### Database Maintenance

**PostgreSQL:**
```bash
# Run weekly maintenance
psql -U testmate_user -d testmate_demo -c "VACUUM ANALYZE;"

# Or use the example code:
# See postgresql_example.cpp - DemoMaintenanceOperations()
```

**MySQL:**
```bash
# Run weekly maintenance
mysql -u testmate_user -p testmate_demo -e "OPTIMIZE TABLE test_results;"

# Or use the example code:
# See mysql_example.cpp - DemoTableMaintenance()
```

## Performance Tips

1. **Use transactions for batch operations:**
   ```cpp
   dataStore->BeginTransaction();
   for (const auto& test : tests) {
       dataStore->SaveTestData(test);
   }
   dataStore->CommitTransaction();
   ```

2. **PostgreSQL: Use savepoints for complex workflows:**
   ```cpp
   BeginTransaction();
   CreateSavepoint("checkpoint1");
   // ... operations ...
   if (error) RollbackToSavepoint("checkpoint1");
   CommitTransaction();
   ```

3. **MySQL: Enable auto-reconnect for long-running processes:**
   ```cpp
   config.autoReconnect = true;
   ```

4. **Regular maintenance:**
   - PostgreSQL: VACUUM ANALYZE (weekly)
   - MySQL: OPTIMIZE TABLE (weekly)

## Troubleshooting

### PostgreSQL Connection Issues

```bash
# Check PostgreSQL is running
sudo systemctl status postgresql

# Check user permissions
psql -U postgres -c "\du"

# Test connection
psql -U testmate_user -d testmate_demo -c "SELECT version();"
```

### MySQL Connection Issues

```bash
# Check MySQL is running
sudo systemctl status mysql

# Check user permissions
mysql -u root -p -e "SELECT User, Host FROM mysql.user WHERE User='testmate_user';"

# Test connection
mysql -u testmate_user -p testmate_demo -e "SELECT VERSION();"
```

### Build Issues

**PostgreSQL not found:**
```bash
# Ubuntu/Debian
sudo apt-get install libpq-dev

# RHEL/CentOS
sudo yum install postgresql-devel
```

**MySQL not found:**
```bash
# Ubuntu/Debian
sudo apt-get install libmysqlclient-dev

# RHEL/CentOS
sudo yum install mysql-devel
```

## Additional Resources

- [TestMATE User Manual](../../docs/USER_MANUAL.md) - Section 8: Database Configuration
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [MySQL Documentation](https://dev.mysql.com/doc/)
- [MariaDB Documentation](https://mariadb.com/kb/en/documentation/)

## License

Copyright © 2025 TestMATE Development Team. All rights reserved.
