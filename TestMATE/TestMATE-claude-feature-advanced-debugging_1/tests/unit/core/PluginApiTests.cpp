/**************************************************************************
 * File Name: PluginApiTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for Plugin system and API layer
 **************************************************************************/

#include "core/plugins/PluginManager.h"
#include "api/TestMATECore.h"
#include <gtest/gtest.h>

using namespace TestMATE;

//=============================================================================
// PluginManager Tests
//=============================================================================

class PluginManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any loaded plugins
        CPluginManager::GetInstance().UnloadAll();
    }

    void TearDown() override {
        CPluginManager::GetInstance().UnloadAll();
    }
};

TEST_F(PluginManagerTest, GetInstance_ReturnsSameInstance) {
    auto& instance1 = CPluginManager::GetInstance();
    auto& instance2 = CPluginManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(PluginManagerTest, InitialState_NoPluginsLoaded) {
    EXPECT_EQ(CPluginManager::GetInstance().GetPluginCount(), 0u);
}

TEST_F(PluginManagerTest, LoadPlugin_InvalidPath_Fails) {
    auto result = CPluginManager::GetInstance().LoadPlugin("/nonexistent/path.so");
    EXPECT_TRUE(result.IsFailure());
}

TEST_F(PluginManagerTest, GetLoadedPlugins_ReturnsEmptyInitially) {
    auto plugins = CPluginManager::GetInstance().GetLoadedPlugins();
    EXPECT_TRUE(plugins.empty());
}

TEST_F(PluginManagerTest, IsPluginLoaded_ReturnsFalseForUnknown) {
    EXPECT_FALSE(CPluginManager::GetInstance().IsPluginLoaded("unknown_plugin"));
}

TEST_F(PluginManagerTest, AddSearchPath_AddsPath) {
    CPluginManager::GetInstance().AddSearchPath("/test/path");
    auto paths = CPluginManager::GetInstance().GetSearchPaths();
    EXPECT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], "/test/path");
}

TEST_F(PluginManagerTest, GetPlugin_ReturnsNullForUnknown) {
    IPlugin* plugin = CPluginManager::GetInstance().GetPlugin("unknown");
    EXPECT_EQ(plugin, nullptr);
}

TEST_F(PluginManagerTest, GetPluginInfo_ReturnsNulloptForUnknown) {
    auto info = CPluginManager::GetInstance().GetPluginInfo("unknown");
    EXPECT_FALSE(info.has_value());
}

//=============================================================================
// TestMATECore API Tests
//=============================================================================

class TestMATECoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        CTestMATECore::GetInstance().Initialize();
    }

    void TearDown() override {
        CTestMATECore::GetInstance().Shutdown();
    }
};

TEST_F(TestMATECoreTest, GetInstance_ReturnsSameInstance) {
    auto& instance1 = CTestMATECore::GetInstance();
    auto& instance2 = CTestMATECore::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(TestMATECoreTest, Initialize_Succeeds) {
    EXPECT_TRUE(CTestMATECore::GetInstance().IsInitialized());
}

TEST_F(TestMATECoreTest, GetVersion_ReturnsVersion) {
    TString version = CTestMATECore::GetInstance().GetVersion();
    EXPECT_FALSE(version.empty());
}

TEST_F(TestMATECoreTest, SetVariable_GetVariable_Works) {
    CTestMATECore::GetInstance().SetVariable("TestVar", "TestValue");
    auto value = CTestMATECore::GetInstance().GetVariable("TestVar");

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "TestValue");
}

TEST_F(TestMATECoreTest, GetVariable_Unknown_ReturnsNullopt) {
    auto value = CTestMATECore::GetInstance().GetVariable("UnknownVar");
    EXPECT_FALSE(value.has_value());
}

TEST_F(TestMATECoreTest, ClearVariables_RemovesAll) {
    CTestMATECore::GetInstance().SetVariable("Var1", "Value1");
    CTestMATECore::GetInstance().SetVariable("Var2", "Value2");

    CTestMATECore::GetInstance().ClearVariables();

    EXPECT_FALSE(CTestMATECore::GetInstance().GetVariable("Var1").has_value());
    EXPECT_FALSE(CTestMATECore::GetInstance().GetVariable("Var2").has_value());
}

TEST_F(TestMATECoreTest, GetTestStatus_ReturnsIdleInitially) {
    auto status = CTestMATECore::GetInstance().GetTestStatus();
    EXPECT_EQ(status.state, EExecutionState::kIdle);
}

TEST_F(TestMATECoreTest, Subscribe_ReturnsUniqueId) {
    TUInt64 id1 = CTestMATECore::GetInstance().Subscribe(
        EApiEvent::kTestStarted, [](EApiEvent, const TString&) {});
    TUInt64 id2 = CTestMATECore::GetInstance().Subscribe(
        EApiEvent::kTestStarted, [](EApiEvent, const TString&) {});

    EXPECT_NE(id1, id2);
}

TEST_F(TestMATECoreTest, Unsubscribe_DoesNotThrow) {
    TUInt64 id = CTestMATECore::GetInstance().Subscribe(
        EApiEvent::kTestStarted, [](EApiEvent, const TString&) {});

    EXPECT_NO_THROW(CTestMATECore::GetInstance().Unsubscribe(id));
}

TEST_F(TestMATECoreTest, StopTest_WhenNotRunning_Succeeds) {
    auto result = CTestMATECore::GetInstance().StopTest();
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(TestMATECoreTest, PauseTest_WhenNotRunning_Fails) {
    auto result = CTestMATECore::GetInstance().PauseTest();
    EXPECT_TRUE(result.IsFailure());
}

TEST_F(TestMATECoreTest, ResumeTest_WhenNotPaused_Fails) {
    auto result = CTestMATECore::GetInstance().ResumeTest();
    EXPECT_TRUE(result.IsFailure());
}

TEST_F(TestMATECoreTest, StartTest_SetsRunningState) {
    STestConfiguration config;
    config.sequencePath = "/test/sequence.xml";

    auto result = CTestMATECore::GetInstance().StartTest(config);
    EXPECT_TRUE(result.IsSuccess());

    auto status = CTestMATECore::GetInstance().GetTestStatus();
    EXPECT_EQ(status.state, EExecutionState::kRunning);
}

TEST_F(TestMATECoreTest, StartTest_WhenRunning_Fails) {
    STestConfiguration config;
    config.sequencePath = "/test/sequence.xml";

    CTestMATECore::GetInstance().StartTest(config);

    auto result = CTestMATECore::GetInstance().StartTest(config);
    EXPECT_TRUE(result.IsFailure());
}

