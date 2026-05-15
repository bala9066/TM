/**************************************************************************
 * File Name: setup_postgresql.sql
 * Description: PostgreSQL database setup script for TestMATE
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Creates database, user, and initializes schema for PostgreSQL backend
 *
 * Usage:
 *   As postgres superuser:
 *     psql -U postgres -f setup_postgresql.sql
 *
 *   Or connect to PostgreSQL and run:
 *     \i setup_postgresql.sql
 **************************************************************************/

-- Create database
CREATE DATABASE testmate_demo
    WITH
    ENCODING = 'UTF8'
    LC_COLLATE = 'en_US.UTF-8'
    LC_CTYPE = 'en_US.UTF-8'
    TEMPLATE = template0;

-- Create user
CREATE USER testmate_user WITH PASSWORD 'testmate_pass';

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE testmate_demo TO testmate_user;

-- Connect to the new database
\c testmate_demo

-- Grant schema privileges
GRANT ALL ON SCHEMA public TO testmate_user;

-- Create extension for UUID generation (optional but useful)
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Create test_results table
CREATE TABLE IF NOT EXISTS test_results (
    id SERIAL PRIMARY KEY,
    test_id VARCHAR(255) NOT NULL UNIQUE,
    sequence_id VARCHAR(255),
    device_id VARCHAR(255),
    lot_id VARCHAR(255),
    verdict INTEGER NOT NULL,
    duration_ms INTEGER,
    operator_name VARCHAR(255),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    error_message TEXT,
    metadata JSONB
);

-- Create indices for common queries
CREATE INDEX idx_test_results_lot_id ON test_results(lot_id);
CREATE INDEX idx_test_results_device_id ON test_results(device_id);
CREATE INDEX idx_test_results_sequence_id ON test_results(sequence_id);
CREATE INDEX idx_test_results_timestamp ON test_results(timestamp DESC);
CREATE INDEX idx_test_results_verdict ON test_results(verdict);

-- Create test_step_results table
CREATE TABLE IF NOT EXISTS test_step_results (
    id SERIAL PRIMARY KEY,
    test_id VARCHAR(255) NOT NULL,
    step_id VARCHAR(255) NOT NULL,
    step_name VARCHAR(255),
    verdict INTEGER NOT NULL,
    duration_ms INTEGER,
    error_message TEXT,
    metadata JSONB,
    FOREIGN KEY (test_id) REFERENCES test_results(test_id) ON DELETE CASCADE
);

-- Create index for step results
CREATE INDEX idx_test_step_results_test_id ON test_step_results(test_id);

-- Create measurements table
CREATE TABLE IF NOT EXISTS measurements (
    id SERIAL PRIMARY KEY,
    test_id VARCHAR(255) NOT NULL,
    step_id VARCHAR(255),
    parameter_name VARCHAR(255) NOT NULL,
    measured_value DOUBLE PRECISION NOT NULL,
    unit VARCHAR(50),
    lower_limit DOUBLE PRECISION,
    upper_limit DOUBLE PRECISION,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (test_id) REFERENCES test_results(test_id) ON DELETE CASCADE
);

-- Create index for measurements
CREATE INDEX idx_measurements_test_id ON measurements(test_id);
CREATE INDEX idx_measurements_parameter ON measurements(parameter_name);

-- Create schema version table
CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description TEXT
);

-- Insert initial schema version
INSERT INTO schema_version (version, description) VALUES (1, 'Initial schema');

-- Grant table permissions to testmate_user
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO testmate_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO testmate_user;

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

GRANT SELECT ON test_summary TO testmate_user;

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

GRANT SELECT ON operator_stats TO testmate_user;

-- Create function to clean old test data
CREATE OR REPLACE FUNCTION cleanup_old_tests(days_old INTEGER)
RETURNS INTEGER AS $$
DECLARE
    deleted_count INTEGER;
BEGIN
    DELETE FROM test_results
    WHERE timestamp < CURRENT_TIMESTAMP - (days_old || ' days')::INTERVAL;
    GET DIAGNOSTICS deleted_count = ROW_COUNT;
    RETURN deleted_count;
END;
$$ LANGUAGE plpgsql;

-- Grant execute permission
GRANT EXECUTE ON FUNCTION cleanup_old_tests TO testmate_user;

-- Display success message
DO $$
BEGIN
    RAISE NOTICE '========================================';
    RAISE NOTICE 'PostgreSQL Database Setup Complete!';
    RAISE NOTICE '========================================';
    RAISE NOTICE 'Database: testmate_demo';
    RAISE NOTICE 'User: testmate_user';
    RAISE NOTICE 'Password: testmate_pass';
    RAISE NOTICE '';
    RAISE NOTICE 'Tables created:';
    RAISE NOTICE '  - test_results';
    RAISE NOTICE '  - test_step_results';
    RAISE NOTICE '  - measurements';
    RAISE NOTICE '  - schema_version';
    RAISE NOTICE '';
    RAISE NOTICE 'Views created:';
    RAISE NOTICE '  - test_summary';
    RAISE NOTICE '  - operator_stats';
    RAISE NOTICE '';
    RAISE NOTICE 'To connect: psql -U testmate_user -d testmate_demo';
    RAISE NOTICE '========================================';
END $$;
