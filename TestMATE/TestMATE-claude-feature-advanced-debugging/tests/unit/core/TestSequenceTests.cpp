/**************************************************************************
 * File Name: TestSequenceTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for Test Sequence and Data Binding
 **************************************************************************/

#include "core/test_sequence/ITestStep.h"
#include "core/test_sequence/TestSequence.h"
#include "core/data/DataBinding.h"
#include <gtest/gtest.h>

using namespace TestMATE;

//=============================================================================
// Mock Test Step for testing
//=============================================================================

class CMockTestStep : public CTestStepBase {
public:
    CMockTestStep(const TString& id, const TString& name)
        : CTestStepBase(id, name, EStepType::kAction)
    {
        SStepParameter param;
        param.name = "input";
        param.type = "string";
        param.required = true;
        AddParameter(param);
    }

    CResult Execute(SStepResult& out_result) override {
        out_result.verdict = ETestVerdict::kPass;
        out_result.message = "Executed";
        return TESTMATE_SUCCESS();
    }
};

//=============================================================================
// TestStep Tests
//=============================================================================

TEST(TestStepTest, Constructor_SetsProperties) {
    CMockTestStep step("step1", "Test Step 1");

    EXPECT_EQ(step.GetId(), "step1");
    EXPECT_EQ(step.GetName(), "Test Step 1");
    EXPECT_EQ(step.GetType(), EStepType::kAction);
}

TEST(TestStepTest, IsEnabled_DefaultTrue) {
    CMockTestStep step("step1", "Test");
    EXPECT_TRUE(step.IsEnabled());
}

TEST(TestStepTest, SetEnabled_ChangesState) {
    CMockTestStep step("step1", "Test");
    step.SetEnabled(false);
    EXPECT_FALSE(step.IsEnabled());
}

TEST(TestStepTest, SetParameter_UpdatesValue) {
    CMockTestStep step("step1", "Test");
    auto result = step.SetParameter("input", "test_value");
    EXPECT_TRUE(result.IsSuccess());

    auto value = step.GetParameter("input");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "test_value");
}

TEST(TestStepTest, SetParameter_UnknownParam_Fails) {
    CMockTestStep step("step1", "Test");
    auto result = step.SetParameter("unknown", "value");
    EXPECT_TRUE(result.IsFailure());
}

TEST(TestStepTest, Execute_ReturnsResult) {
    CMockTestStep step("step1", "Test");
    SStepResult result;

    auto execResult = step.Execute(result);

    EXPECT_TRUE(execResult.IsSuccess());
    EXPECT_EQ(result.verdict, ETestVerdict::kPass);
}

//=============================================================================
// TestSequence Tests
//=============================================================================

class TestSequenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_sequence = std::make_unique<CTestSequence>("seq1", "Test Sequence");
    }

    std::unique_ptr<CTestSequence> m_sequence;
};

TEST_F(TestSequenceTest, Constructor_SetsProperties) {
    EXPECT_EQ(m_sequence->GetId(), "seq1");
    EXPECT_EQ(m_sequence->GetName(), "Test Sequence");
    EXPECT_EQ(m_sequence->GetStepCount(), 0u);
}

TEST_F(TestSequenceTest, AddStep_IncrementsCount) {
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s1", "Step 1"));
    EXPECT_EQ(m_sequence->GetStepCount(), 1u);

    m_sequence->AddStep(std::make_unique<CMockTestStep>("s2", "Step 2"));
    EXPECT_EQ(m_sequence->GetStepCount(), 2u);
}

TEST_F(TestSequenceTest, GetStep_ReturnsCorrectStep) {
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s1", "Step 1"));
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s2", "Step 2"));

    ITestStep* step = m_sequence->GetStep(1);
    ASSERT_NE(step, nullptr);
    EXPECT_EQ(step->GetId(), "s2");
}

TEST_F(TestSequenceTest, GetStep_InvalidIndex_ReturnsNull) {
    ITestStep* step = m_sequence->GetStep(999);
    EXPECT_EQ(step, nullptr);
}

TEST_F(TestSequenceTest, GetStepById_ReturnsCorrectStep) {
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s1", "Step 1"));
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s2", "Step 2"));

    ITestStep* step = m_sequence->GetStepById("s2");
    ASSERT_NE(step, nullptr);
    EXPECT_EQ(step->GetName(), "Step 2");
}

TEST_F(TestSequenceTest, RemoveStep_DecreasesCount) {
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s1", "Step 1"));
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s2", "Step 2"));

    m_sequence->RemoveStep(0);

    EXPECT_EQ(m_sequence->GetStepCount(), 1u);
    EXPECT_EQ(m_sequence->GetStep(0)->GetId(), "s2");
}

TEST_F(TestSequenceTest, Clear_RemovesAllSteps) {
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s1", "Step 1"));
    m_sequence->AddStep(std::make_unique<CMockTestStep>("s2", "Step 2"));

    m_sequence->Clear();

    EXPECT_EQ(m_sequence->GetStepCount(), 0u);
}

TEST_F(TestSequenceTest, SetVariable_GetVariable_Works) {
    m_sequence->SetVariable("voltage", "3.3");

    auto value = m_sequence->GetVariable("voltage");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "3.3");
}

TEST_F(TestSequenceTest, GetEnabledStepCount_CountsCorrectly) {
    auto step1 = std::make_unique<CMockTestStep>("s1", "Step 1");
    auto step2 = std::make_unique<CMockTestStep>("s2", "Step 2");
    step2->SetEnabled(false);

    m_sequence->AddStep(std::move(step1));
    m_sequence->AddStep(std::move(step2));

    EXPECT_EQ(m_sequence->GetEnabledStepCount(), 1u);
}

//=============================================================================
// DataContext Tests
//=============================================================================

TEST(DataContextTest, SetGet_StringValue) {
    CDataContext ctx;
    ctx.SetString("name", "test");

    auto value = ctx.GetString("name");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "test");
}

TEST(DataContextTest, SetGet_IntValue) {
    CDataContext ctx;
    ctx.SetInt("count", 42);

    auto value = ctx.GetInt("count");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
}

TEST(DataContextTest, SetGet_FloatValue) {
    CDataContext ctx;
    ctx.SetFloat("voltage", 3.3);

    auto value = ctx.GetFloat("voltage");
    ASSERT_TRUE(value.has_value());
    EXPECT_DOUBLE_EQ(*value, 3.3);
}

TEST(DataContextTest, SetGet_BoolValue) {
    CDataContext ctx;
    ctx.SetBool("enabled", true);

    auto value = ctx.GetBool("enabled");
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(*value);
}

TEST(DataContextTest, HasKey_ReturnsCorrectly) {
    CDataContext ctx;
    ctx.SetString("existing", "value");

    EXPECT_TRUE(ctx.HasKey("existing"));
    EXPECT_FALSE(ctx.HasKey("nonexistent"));
}

TEST(DataContextTest, Resolve_SubstitutesVariables) {
    CDataContext ctx;
    ctx.SetString("voltage", "3.3");
    ctx.SetString("unit", "V");

    TString resolved = ctx.Resolve("Output: ${voltage}${unit}");
    EXPECT_EQ(resolved, "Output: 3.3V");
}

TEST(DataContextTest, ParentContext_FallsBack) {
    CDataContext parent;
    parent.SetString("inherited", "from_parent");

    CDataContext child;
    child.SetParent(&parent);

    auto value = child.GetString("inherited");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "from_parent");
}

//=============================================================================
// DataBinder Tests
//=============================================================================

TEST(DataBinderTest, GetInstance_ReturnsSameInstance) {
    auto& instance1 = CDataBinder::GetInstance();
    auto& instance2 = CDataBinder::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST(DataBinderTest, ResolveEnvVar_GetsEnvironmentVariable) {
    // PATH should exist on all systems
    TString path = CDataBinder::ResolveEnvVar("PATH");
    EXPECT_FALSE(path.empty());
}

TEST(DataBinderTest, ResolveTimestamp_ReturnsFormattedTime) {
    TString timestamp = CDataBinder::ResolveTimestamp("%Y");
    // Should be a 4-digit year
    EXPECT_EQ(timestamp.length(), 4u);
}

