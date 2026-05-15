/**************************************************************************
 * File Name: ThreadingTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for threading components
 **************************************************************************/

#include "core/threading/ThreadPool.h"
#include "core/threading/SyncPoint.h"
#include "core/scheduling/ResourceScheduler.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>

using namespace TestMATE;

//=============================================================================
// ThreadPool Tests
//=============================================================================

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_pool = std::make_unique<CThreadPool>(4);
    }

    void TearDown() override {
        m_pool.reset();
    }

    std::unique_ptr<CThreadPool> m_pool;
};

TEST_F(ThreadPoolTest, Constructor_CreatesSpecifiedThreads) {
    EXPECT_EQ(m_pool->GetThreadCount(), 4u);
}

TEST_F(ThreadPoolTest, Submit_ExecutesTask) {
    std::atomic<bool> executed{false};

    auto future = m_pool->Submit([&]() {
        executed = true;
        return 42;
    });

    int result = future.get();

    EXPECT_TRUE(executed);
    EXPECT_EQ(result, 42);
}

TEST_F(ThreadPoolTest, Submit_MultipleTasks_AllExecute) {
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 100; ++i) {
        futures.push_back(m_pool->Submit([&]() {
            ++counter;
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, WaitForAll_BlocksUntilComplete) {
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        m_pool->Submit([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++counter;
        });
    }

    m_pool->WaitForAll();

    EXPECT_EQ(counter.load(), 10);
}

TEST_F(ThreadPoolTest, GetPendingTaskCount_ReflectsQueueState) {
    // Initially no pending tasks
    EXPECT_EQ(m_pool->GetPendingTaskCount(), 0u);
}

TEST_F(ThreadPoolTest, IsRunning_ReturnsTrueBeforeShutdown) {
    EXPECT_TRUE(m_pool->IsRunning());

    m_pool->Shutdown();

    EXPECT_FALSE(m_pool->IsRunning());
}

//=============================================================================
// SyncPoint Tests
//=============================================================================

TEST(SyncPointTest, Constructor_SetsParticipants) {
    CSyncPoint sync(5);
    EXPECT_EQ(sync.GetParticipantCount(), 5u);
    EXPECT_EQ(sync.GetArrivedCount(), 0u);
}

TEST(SyncPointTest, Wait_ReleasesWhenAllArrive) {
    CSyncPoint sync(3);
    std::atomic<int> released{0};

    std::thread t1([&]() { sync.Wait(); ++released; });
    std::thread t2([&]() { sync.Wait(); ++released; });
    std::thread t3([&]() { sync.Wait(); ++released; });

    t1.join();
    t2.join();
    t3.join();

    EXPECT_EQ(released.load(), 3);
}

TEST(SyncPointTest, WaitFor_TimesOut) {
    CSyncPoint sync(2);

    CResult result = sync.WaitFor(50);  // 50ms timeout

    EXPECT_TRUE(result.IsFailure());
    EXPECT_EQ(result.GetCode(), EErrorCode::kSyncPointTimeout);
}

//=============================================================================
// Barrier Tests
//=============================================================================

TEST(BarrierTest, Arrive_ReleasesAllThreads) {
    CBarrier barrier(3);
    std::atomic<int> released{0};

    std::thread t1([&]() { barrier.Arrive(); ++released; });
    std::thread t2([&]() { barrier.Arrive(); ++released; });
    std::thread t3([&]() { barrier.Arrive(); ++released; });

    t1.join();
    t2.join();
    t3.join();

    EXPECT_EQ(released.load(), 3);
}

TEST(BarrierTest, Reset_AllowsReuse) {
    CBarrier barrier(2);

    std::thread t1([&]() { barrier.Arrive(); });
    std::thread t2([&]() { barrier.Arrive(); });
    t1.join();
    t2.join();

    barrier.Reset();

    std::thread t3([&]() { barrier.Arrive(); });
    std::thread t4([&]() { barrier.Arrive(); });
    t3.join();
    t4.join();

    EXPECT_EQ(barrier.GetWaiting(), 0u);
}

//=============================================================================
// ResourceScheduler Tests
//=============================================================================

class ResourceSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_scheduler = std::make_unique<CResourceScheduler>();
    }

    std::unique_ptr<CResourceScheduler> m_scheduler;
};

TEST_F(ResourceSchedulerTest, RegisterResource_ReturnsUniqueId) {
    TResourceId id1 = m_scheduler->RegisterResource("Resource1");
    TResourceId id2 = m_scheduler->RegisterResource("Resource2");

    EXPECT_NE(id1, id2);
    EXPECT_EQ(m_scheduler->GetResourceCount(), 2u);
}

TEST_F(ResourceSchedulerTest, TryAcquire_SucceedsWhenAvailable) {
    TResourceId id = m_scheduler->RegisterResource("TestResource");

    bool acquired = m_scheduler->TryAcquire(id, 1);

    EXPECT_TRUE(acquired);
    EXPECT_EQ(m_scheduler->GetOwner(id), 1u);
}

TEST_F(ResourceSchedulerTest, TryAcquire_FailsWhenOwned) {
    TResourceId id = m_scheduler->RegisterResource("TestResource");

    m_scheduler->TryAcquire(id, 1);
    bool secondAcquire = m_scheduler->TryAcquire(id, 2);

    EXPECT_FALSE(secondAcquire);
}

TEST_F(ResourceSchedulerTest, Release_MakesResourceAvailable) {
    TResourceId id = m_scheduler->RegisterResource("TestResource");

    m_scheduler->TryAcquire(id, 1);
    CResult result = m_scheduler->Release(id, 1);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_scheduler->GetOwner(id), kInvalidSocketId);
}

TEST_F(ResourceSchedulerTest, Release_FailsIfNotOwner) {
    TResourceId id = m_scheduler->RegisterResource("TestResource");

    m_scheduler->TryAcquire(id, 1);
    CResult result = m_scheduler->Release(id, 2);  // Wrong owner

    EXPECT_TRUE(result.IsFailure());
}

TEST_F(ResourceSchedulerTest, ReleaseAll_ReleasesAllHeldResources) {
    TResourceId id1 = m_scheduler->RegisterResource("Resource1");
    TResourceId id2 = m_scheduler->RegisterResource("Resource2");

    m_scheduler->TryAcquire(id1, 1);
    m_scheduler->TryAcquire(id2, 1);

    m_scheduler->ReleaseAll(1);

    EXPECT_EQ(m_scheduler->GetOwner(id1), kInvalidSocketId);
    EXPECT_EQ(m_scheduler->GetOwner(id2), kInvalidSocketId);
}

TEST_F(ResourceSchedulerTest, IsAvailable_ReflectsState) {
    TResourceId id = m_scheduler->RegisterResource("TestResource");

    EXPECT_TRUE(m_scheduler->IsAvailable(id, 1));

    m_scheduler->TryAcquire(id, 1);

    EXPECT_TRUE(m_scheduler->IsAvailable(id, 1));   // Same owner
    EXPECT_FALSE(m_scheduler->IsAvailable(id, 2));  // Different requester
}

TEST_F(ResourceSchedulerTest, AcquireSet_AcquiresMultipleResources) {
    TResourceId id1 = m_scheduler->RegisterResource("Resource1");
    TResourceId id2 = m_scheduler->RegisterResource("Resource2");

    TVector<TResourceId> ids = {id1, id2};
    CResult result = m_scheduler->AcquireSet(ids, 1, 100);

    EXPECT_TRUE(result.IsSuccess());
    EXPECT_EQ(m_scheduler->GetOwner(id1), 1u);
    EXPECT_EQ(m_scheduler->GetOwner(id2), 1u);
}

TEST_F(ResourceSchedulerTest, SharedResource_AllowsMultipleUsers) {
    TResourceId id = m_scheduler->RegisterResource("SharedResource",
                                                    EResourceAccessType::kShared,
                                                    3);

    bool acq1 = m_scheduler->TryAcquire(id, 1);
    bool acq2 = m_scheduler->TryAcquire(id, 2);
    bool acq3 = m_scheduler->TryAcquire(id, 3);

    EXPECT_TRUE(acq1);
    EXPECT_TRUE(acq2);
    EXPECT_TRUE(acq3);
}
