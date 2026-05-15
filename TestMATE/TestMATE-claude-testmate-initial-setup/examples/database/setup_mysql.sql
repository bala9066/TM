/**************************************************************************
 * File Name: setup_mysql.sql
 * Description: MySQL/MariaDB database setup script for TestMATE
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Creates database, user, and initializes schema for MySQL/MariaDB backend
 *
 * Usage:
 *   As MySQL root user:
 *     mysql -u root -p < setup_mysql.sql
 *
 *   Or connect to MySQL and run:
 *     source setup_mysql.sql;
 **************************************************************************/

-- Create database with UTF8MB4 encoding (full Unicode support)
CREATE DATABASE IF NOT EXISTS testmate_demo
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

-- Create user and grant privileges
CREATE USER IF NOT EXISTS 'testmate_user'@'localhost' IDENTIFIED BY 'testmate_pass';
GRANT ALL PRIVILEGES ON testmate_demo.* TO 'testmate_user'@'localhost';

-- Also allow connections from any host (optional - for network deployment)
CREATE USER IF NOT EXISTS 'testmate_user'@'%' IDENTIFIED BY 'testmate_pass';
GRANT ALL PRIVILEGES ON testmate_demo.* TO 'testmate_user'@'%';

FLUSH PRIVILEGES;

-- Switch to the new database
USE testmate_demo;

-- Create test_results table
CREATE TABLE IF NOT EXISTS test_results (
    id INT AUTO_INCREMENT PRIMARY KEY,
    test_id VARCHAR(255) NOT NULL UNIQUE,
    sequence_id VARCHAR(255),
    device_id VARCHAR(255),
    lot_id VARCHAR(255),
    verdict INT NOT NULL,
    duration_ms INT,
    operator_name VARCHAR(255),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    error_message TEXT,
    metadata JSON,
    INDEX idx_lot_id (lot_id),
    INDEX idx_device_id (device_id),
    INDEX idx_sequence_id (sequence_id),
    INDEX idx_timestamp (timestamp DESC),
    INDEX idx_verdict (verdict)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Create test_step_results table
CREATE TABLE IF NOT EXISTS test_step_results (
    id INT AUTO_INCREMENT PRIMARY KEY,
    test_id VARCHAR(255) NOT NULL,
    step_id VARCHAR(255) NOT NULL,
    step_name VARCHAR(255),
    verdict INT NOT NULL,
    duration_ms INT,
    error_message TEXT,
    metadata JSON,
    INDEX idx_test_id (test_id),
    FOREIGN KEY (test_id) REFERENCES test_results(test_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Create measurements table
CREATE TABLE IF NOT EXISTS measurements (
    id INT AUTO_INCREMENT PRIMARY KEY,
    test_id VARCHAR(255) NOT NULL,
    step_id VARCHAR(255),
    parameter_name VARCHAR(255) NOT NULL,
    measured_value DOUBLE NOT NULL,
    unit VARCHAR(50),
    lower_limit DOUBLE,
    upper_limit DOUBLE,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_test_id (test_id),
    INDEX idx_parameter (parameter_name),
    FOREIGN KEY (test_id) REFERENCES test_results(test_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Create schema version table
CREATE TABLE IF NOT EXISTS schema_version (
    version INT PRIMARY KEY,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description TEXT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Insert initial schema version
INSERT INTO schema_version (version, description) VALUES (1, 'Initial schema')
ON DUPLICATE KEY UPDATE version = version;

-- Create view for test summary statistics
CREATE OR REPLACE VIEW test_summary AS
SELECT
    lot_id,
    COUNT(*) as total_tests,
    SUM(CASE WHEN verdict = 1 THEN 1 ELSE 0 END) as passed,
    SUM(CASE WHEN verdict = 2 THEN 1 ELSE 0 END) as failed,
    ROUND(100.0 * SUM(CASE WHEN verdict = 1 THEN 1 ELSE 0 END) / COUNT(*), 2) as yield_percent,
    AVG(duration_ms) as avg_duration_ms,
    MIN(timestamp) as first_test,
    MAX(timestamp) as last_test
FROM test_results
WHERE lot_id IS NOT NULL
GROUP BY lot_id;

-- Create view for operator statistics
CREATE OR REPLACE VIEW operator_stats AS
SELECT
    operator_name,
    COUNT(*) as total_tests,
    SUM(CASE WHEN verdict = 1 THEN 1 ELSE 0 END) as passed,
    SUM(CASE WHEN verdict = 2 THEN 1 ELSE 0 END) as failed,
    ROUND(100.0 * SUM(CASE WHEN verdict = 1 THEN 1 ELSE 0 END) / COUNT(*), 2) as pass_rate,
    AVG(duration_ms) as avg_duration_ms
FROM test_results
WHERE operator_name IS NOT NULL
GROUP BY operator_name;

-- Create stored procedure to clean old test data
DELIMITER //

CREATE PROCEDURE IF NOT EXISTS cleanup_old_tests(IN days_old INT)
BEGIN
    DECLARE deleted_count INT;

    DELETE FROM test_results
    WHERE timestamp < DATE_SUB(NOW(), INTERVAL days_old DAY);

    SELECT ROW_COUNT() INTO deleted_count;

    SELECT CONCAT('Deleted ', deleted_count, ' test records older than ', days_old, ' days') AS result;
END //

DELIMITER ;

-- Grant permissions to testmate_user
GRANT SELECT ON test_summary TO 'testmate_user'@'localhost';
GRANT SELECT ON operator_stats TO 'testmate_user'@'localhost';
GRANT EXECUTE ON PROCEDURE cleanup_old_tests TO 'testmate_user'@'localhost';

GRANT SELECT ON test_summary TO 'testmate_user'@'%';
GRANT SELECT ON operator_stats TO 'testmate_user'@'%';
GRANT EXECUTE ON PROCEDURE cleanup_old_tests TO 'testmate_user'@'%';

-- Create event scheduler job for automatic cleanup (optional, disabled by default)
-- To enable: SET GLOBAL event_scheduler = ON;
CREATE EVENT IF NOT EXISTS auto_cleanup_old_tests
ON SCHEDULE EVERY 1 WEEK
DO
    CALL cleanup_old_tests(90);  -- Clean tests older than 90 days

-- Disable the event by default
ALTER EVENT auto_cleanup_old_tests DISABLE;

-- Display success message
SELECT '========================================'  AS '';
SELECT 'MySQL Database Setup Complete!'         AS '';
SELECT '========================================'  AS '';
SELECT 'Database: testmate_demo'                 AS '';
SELECT 'User: testmate_user'                     AS '';
SELECT 'Password: testmate_pass'                 AS '';
SELECT ''                                        AS '';
SELECT 'Tables created:'                         AS '';
SELECT '  - test_results'                        AS '';
SELECT '  - test_step_results'                   AS '';
SELECT '  - measurements'                        AS '';
SELECT '  - schema_version'                      AS '';
SELECT ''                                        AS '';
SELECT 'Views created:'                          AS '';
SELECT '  - test_summary'                        AS '';
SELECT '  - operator_stats'                      AS '';
SELECT ''                                        AS '';
SELECT 'To connect: mysql -u testmate_user -p testmate_demo' AS '';
SELECT '========================================'  AS '';
