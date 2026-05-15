# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-22
# Description: CMake configuration for database examples
# **************************************************************************

# PostgreSQL example (optional)
if(TESTMATE_POSTGRESQL_SUPPORT)
    add_executable(postgresql_example postgresql_example.cpp)
    target_link_libraries(postgresql_example
        PRIVATE
            testmate_database
            testmate_core
    )
    target_include_directories(postgresql_example
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )

    # Install example
    install(TARGETS postgresql_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
    )

    message(STATUS "PostgreSQL example enabled")
endif()

# MySQL example (optional)
if(TESTMATE_MYSQL_SUPPORT)
    add_executable(mysql_example mysql_example.cpp)
    target_link_libraries(mysql_example
        PRIVATE
            testmate_database
            testmate_core
    )
    target_include_directories(mysql_example
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )

    # Install example
    install(TARGETS mysql_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
    )

    message(STATUS "MySQL example enabled")
endif()

# Database migration tool (requires at least one backend besides SQLite)
if(TESTMATE_POSTGRESQL_SUPPORT OR TESTMATE_MYSQL_SUPPORT)
    add_executable(database_migration_tool database_migration_tool.cpp)
    target_link_libraries(database_migration_tool
        PRIVATE
            testmate_database
            testmate_core
    )
    target_include_directories(database_migration_tool
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )

    # Install migration tool
    install(TARGETS database_migration_tool
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
    )

    message(STATUS "Database migration tool enabled")
endif()

# Database backup tool (works with all backends including SQLite)
add_executable(database_backup_tool database_backup_tool.cpp)
target_link_libraries(database_backup_tool
    PRIVATE
        testmate_database
        testmate_core
)
target_include_directories(database_backup_tool
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

install(TARGETS database_backup_tool
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
)

message(STATUS "Database backup tool enabled")

# Database restore tool (works with all backends including SQLite)
add_executable(database_restore_tool database_restore_tool.cpp)
target_link_libraries(database_restore_tool
    PRIVATE
        testmate_database
        testmate_core
)
target_include_directories(database_restore_tool
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

install(TARGETS database_restore_tool
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
)

message(STATUS "Database restore tool enabled")

# Database benchmark tool (works with all backends)
add_executable(database_benchmark database_benchmark.cpp)
target_link_libraries(database_benchmark
    PRIVATE
        testmate_database
        testmate_core
)
target_include_directories(database_benchmark
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

install(TARGETS database_benchmark
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
)

message(STATUS "Database benchmark tool enabled")

# Install SQL setup scripts and README
install(FILES
    setup_postgresql.sql
    setup_mysql.sql
    README.md
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/database
)
