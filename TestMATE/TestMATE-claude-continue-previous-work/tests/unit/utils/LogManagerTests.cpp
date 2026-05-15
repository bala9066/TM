/**************************************************************************
 * File Name: LogManagerTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for LogManager
 **************************************************************************/

#include "utils/LogManager.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace TestMATE;

//=============================================================================
// Test Fixture
//=============================================================================

class LogManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing handlers
        CLogManager::GetInstance().ClearHandlers();
    }

    void TearDown() override {
        CLogManager::GetInstance().ClearHandlers();
    }
};

//=============================================================================
// Singleton Tests
//=============================================================================

TEST_F(LogManagerTest, GetInstance_ReturnsSameInstance) {
    auto& instance1 = CLogManager::GetInstance();
    auto& instance2 = CLogManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

//=============================================================================
// Log Level Tests
//=============================================================================

TEST_F(LogManagerTest, SetGlobalLevel_ChangesLevel) {
    CLogManager::GetInstance().SetGlobalLevel(ELogLevel::kWarning);
    EXPECT_EQ(CLogManager::GetInstance().GetGlobalLevel(), ELogLevel::kWarning);

    CLogManager::GetInstance().SetGlobalLevel(ELogLevel::kDebug);
    EXPECT_EQ(CLogManager::GetInstance().GetGlobalLevel(), ELogLevel::kDebug);
}

//=============================================================================
// Handler Tests
//=============================================================================

// Custom test handler that captures log messages
class CTestLogHandler : public ILogHandler {
public:
    void Write(ELogLevel in_eLevel,
               const TString& in_strSource,
               const TString& in_strMessage,
               TTimePoint /*in_timeStamp*/) override {
        m_vecMessages.push_back({in_eLevel, in_strSource, in_strMessage});
    }

    void Flush() override {}

    struct LogEntry {
        ELogLevel level;
        TString source;
        TString message;
    };

    TVector<LogEntry> m_vecMessages;
};

TEST_F(LogManagerTest, AddHandler_ReceivesLogMessages) {
    auto pHandler = std::make_unique<CTestLogHandler>();
    auto* pRawHandler = pHandler.get();

    CLogManager::GetInstance().AddHandler(std::move(pHandler));
    CLogManager::GetInstance().SetGlobalLevel(ELogLevel::kTrace);

    CLogManager::GetInstance().Log(ELogLevel::kInfo, "TestSource", "Test message");

    ASSERT_EQ(pRawHandler->m_vecMessages.size(), 1u);
    EXPECT_EQ(pRawHandler->m_vecMessages[0].level, ELogLevel::kInfo);
    EXPECT_EQ(pRawHandler->m_vecMessages[0].source, "TestSource");
    EXPECT_EQ(pRawHandler->m_vecMessages[0].message, "Test message");
}

TEST_F(LogManagerTest, LogLevel_FiltersMessages) {
    auto pHandler = std::make_unique<CTestLogHandler>();
    auto* pRawHandler = pHandler.get();

    CLogManager::GetInstance().AddHandler(std::move(pHandler));
    CLogManager::GetInstance().SetGlobalLevel(ELogLevel::kWarning);

    // These should be filtered out
    CLogManager::GetInstance().Log(ELogLevel::kTrace, "Test", "Trace");
    CLogManager::GetInstance().Log(ELogLevel::kDebug, "Test", "Debug");
    CLogManager::GetInstance().Log(ELogLevel::kInfo, "Test", "Info");

    // These should pass through
    CLogManager::GetInstance().Log(ELogLevel::kWarning, "Test", "Warning");
    CLogManager::GetInstance().Log(ELogLevel::kError, "Test", "Error");
    CLogManager::GetInstance().Log(ELogLevel::kFatal, "Test", "Fatal");

    EXPECT_EQ(pRawHandler->m_vecMessages.size(), 3u);
}

//=============================================================================
// Convenience Method Tests
//=============================================================================

TEST_F(LogManagerTest, LogInfo_LogsAtInfoLevel) {
    auto pHandler = std::make_unique<CTestLogHandler>();
    auto* pRawHandler = pHandler.get();

    CLogManager::GetInstance().AddHandler(std::move(pHandler));
    CLogManager::GetInstance().SetGlobalLevel(ELogLevel::kTrace);

    CLogManager::GetInstance().LogInfo("Source", "Message with {} placeholder", "one");

    ASSERT_EQ(pRawHandler->m_vecMessages.size(), 1u);
    EXPECT_EQ(pRawHandler->m_vecMessages[0].level, ELogLevel::kInfo);
    EXPECT_EQ(pRawHandler->m_vecMessages[0].message, "Message with one placeholder");
}
