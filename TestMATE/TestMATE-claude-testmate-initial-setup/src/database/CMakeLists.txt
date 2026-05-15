# **************************************************************************
# File Name: CMakeLists.txt
# Author: TestMATE Development Team
# Created Date: 2025-01-XX
# Description: CMake configuration for TestMATE database layer
# **************************************************************************

set(TARGET_NAME testmate_database)

set(SOURCES
    DataStore.cpp
    SqliteDataStore.cpp
    DataStoreFactory.cpp
)

set(HEADERS
    IDataStore.h
    SqliteDataStore.h
    DataStoreFactory.h
)

# Optional PostgreSQL support
option(TESTMATE_POSTGRESQL_SUPPORT "Enable PostgreSQL database backend" OFF)
if(TESTMATE_POSTGRESQL_SUPPORT)
    find_package(PostgreSQL QUIET)
    if(PostgreSQL_FOUND)
        list(APPEND SOURCES PostgreSqlDataStore.cpp)
        list(APPEND HEADERS PostgreSqlDataStore.h)
        message(STATUS "PostgreSQL support enabled")
    else()
        message(WARNING "PostgreSQL requested but not found - disabling PostgreSQL support")
        set(TESTMATE_POSTGRESQL_SUPPORT OFF)
    endif()
endif()

# Optional MySQL/MariaDB support
option(TESTMATE_MYSQL_SUPPORT "Enable MySQL/MariaDB database backend" OFF)
if(TESTMATE_MYSQL_SUPPORT)
    find_package(MySQL QUIET)
    if(NOT MySQL_FOUND)
        # Try MariaDB
        find_package(MariaDB QUIET)
    endif()
    if(MySQL_FOUND OR MariaDB_FOUND)
        list(APPEND SOURCES MySqlDataStore.cpp)
        list(APPEND HEADERS MySqlDataStore.h)
        message(STATUS "MySQL/MariaDB support enabled")
    else()
        message(WARNING "MySQL requested but not found - disabling MySQL support")
        set(TESTMATE_MYSQL_SUPPORT OFF)
    endif()
endif()

add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    include(FetchContent)
    FetchContent_Declare(sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip
    )
    FetchContent_MakeAvailable(sqlite3)
    add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
endif()

target_link_libraries(${TARGET_NAME}
    PUBLIC
        testmate_core
    PRIVATE
        $<IF:$<TARGET_EXISTS:SQLite::SQLite3>,SQLite::SQLite3,sqlite3>
)

# Add PostgreSQL if enabled
if(TESTMATE_POSTGRESQL_SUPPORT AND PostgreSQL_FOUND)
    target_link_libraries(${TARGET_NAME} PRIVATE PostgreSQL::PostgreSQL)
    target_include_directories(${TARGET_NAME} PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    target_compile_definitions(${TARGET_NAME} PRIVATE TESTMATE_POSTGRESQL_SUPPORT)
endif()

# Add MySQL if enabled
if(TESTMATE_MYSQL_SUPPORT)
    if(MySQL_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE ${MYSQL_LIBRARIES})
        target_include_directories(${TARGET_NAME} PRIVATE ${MYSQL_INCLUDE_DIRS})
    elseif(MariaDB_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE ${MARIADB_LIBRARIES})
        target_include_directories(${TARGET_NAME} PRIVATE ${MARIADB_INCLUDE_DIRS})
    endif()
    target_compile_definitions(${TARGET_NAME} PRIVATE TESTMATE_MYSQL_SUPPORT)
endif()

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)
