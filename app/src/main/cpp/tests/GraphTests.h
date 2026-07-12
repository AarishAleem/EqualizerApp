#ifndef EQUALIZERAPP_GRAPHTESTS_H
#define EQUALIZERAPP_GRAPHTESTS_H

#include "../GraphCompiler.h"
#include "../GraphExecutor.h"
#include "../PreampModule.h"
#include "../ParametricEQModule.h"
#include "../AudioEngine.h"
#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <condition_variable>

#define TEST_EXPECT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << " (" << #condition << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            failCount++; \
        } \
    } while (false)

class LifetimeProbeModule : public DSPModule {
public:
    explicit LifetimeProbeModule(std::atomic<uint32_t>* counter) : counter_(counter) {}
    ~LifetimeProbeModule() override {
        if (counter_) counter_->fetch_add(1, std::memory_order_relaxed);
    }

    void prepare(const ProcessSpec&) override {}
    void process(ProcessContext&) override {}
    void reset() override {}
    void setParameter(ParameterID, ParameterValue) override {}
    std::vector<ParameterID> getOwnedParameters() const override { return {}; }
    std::string getName() const override { return "LifetimeProbe"; }

    std::unique_ptr<DSPModule> clone() const override {
        return std::make_unique<LifetimeProbeModule>(counter_);
    }

private:
    std::atomic<uint32_t>* counter_;
};

class GraphTests {
public:
    static uint32_t runAll() {
        uint32_t failCount = 0;

        testInvalidProcessSpec(failCount);
        testEmptyGraph(failCount);
        testGraphMutation(failCount);
        testCompilationValidation(failCount);
        testDeterministicOrdering(failCount);
        testNodeMaterialization(failCount);
        testParametricEQBounds(failCount);
#ifdef POWEREQ_NATIVE_TESTS
        testPlanReclamationConcurrency(failCount);
        testPlanAcquisitionCollision(failCount);
        testConcurrentPublication(failCount);
#endif

        return failCount;
    }

private:
    static void testInvalidProcessSpec(uint32_t& failCount) {
        GraphCompiler compiler;
        AudioGraph graph;
        graph.addNode(1, std::make_unique<PreampModule>());

        ProcessSpec invalidSpec{0.0, 512, 2}; // Invalid rate
        auto res = compiler.compile(graph, invalidSpec);
        TEST_EXPECT(res.code == CompilationResultCode::InvalidProcessSpec, "Reject invalid spec");
    }

    static void testEmptyGraph(uint32_t& failCount) {
        GraphCompiler compiler;
        AudioGraph graph;
        auto res = compiler.compile(graph, {48000.0, 512, 2});
        TEST_EXPECT(res.code == CompilationResultCode::MissingNodes, "Reject empty graph");
    }

    static void testGraphMutation(uint32_t& failCount) {
        AudioGraph graph;
        TEST_EXPECT(graph.addNode(1, std::make_unique<PreampModule>()) == GraphMutationResult::Success, "Add node");
        TEST_EXPECT(graph.addNode(1, std::make_unique<PreampModule>()) == GraphMutationResult::DuplicateNodeId, "Duplicate ID rejection");
        TEST_EXPECT(graph.addNode(ReservedNodes::Invalid, std::make_unique<PreampModule>()) == GraphMutationResult::InvalidNodeId, "Invalid node ID rejection");

        graph.addNode(2, std::make_unique<PreampModule>());
        TEST_EXPECT(graph.setLink(1, 2) == GraphMutationResult::Success, "Link success");
        TEST_EXPECT(graph.setLink(99, 2) == GraphMutationResult::MissingEdgeSource, "Missing edge source rejection");
    }

    static void testCompilationValidation(uint32_t& failCount) {
        GraphCompiler compiler;
        ProcessSpec spec{48000.0, 512, 2};

        // Missing destination
        {
            AudioGraph graph;
            graph.addNode(1, std::make_unique<PreampModule>());
            graph.getNodesMutable()[0].next = 99;
            auto res = compiler.compile(graph, spec);
            TEST_EXPECT(res.code == CompilationResultCode::MissingEdgeDestination, "Missing edge destination");
        }

        // Self-loop
        {
            AudioGraph graph;
            graph.addNode(1, std::make_unique<PreampModule>());
            graph.setLink(1, 1);
            auto res = compiler.compile(graph, spec);
            TEST_EXPECT(res.code == CompilationResultCode::UnsupportedSelfLoop, "Self loop detection");
        }

        // Pure Cycle
        {
            AudioGraph graph;
            graph.addNode(1, std::make_unique<PreampModule>());
            graph.addNode(2, std::make_unique<PreampModule>());
            graph.setLink(1, 2);
            graph.setLink(2, 1);
            auto res = compiler.compile(graph, spec);
            TEST_EXPECT(res.code == CompilationResultCode::CycleDetected, "Cycle detection");
        }

        // Multiple Start Nodes
        {
            AudioGraph graph;
            graph.addNode(1, std::make_unique<PreampModule>());
            graph.addNode(2, std::make_unique<PreampModule>());
            auto res = compiler.compile(graph, spec);
            TEST_EXPECT(res.code == CompilationResultCode::AmbiguousStartNode, "Ambiguous root rejection");
        }
    }

    static void testDeterministicOrdering(uint32_t& failCount) {
        GraphCompiler compiler;
        AudioGraph graph;
        graph.addNode(2, std::make_unique<PreampModule>());
        graph.addNode(1, std::make_unique<PreampModule>());
        graph.setLink(1, 2);

        auto res = compiler.compile(graph, {48000.0, 512, 2});
        TEST_EXPECT(res.isSuccess(), "Topological order 1->2");
    }

    static void testNodeMaterialization(uint32_t& failCount) {
        AudioGraph graph;
        graph.addNode(1, std::make_unique<PreampModule>());
        GraphCompiler compiler;
        auto r1 = compiler.compile(graph, {48000.0, 512, 2});
        auto r2 = compiler.compile(graph, {48000.0, 512, 2});
        TEST_EXPECT(r1.plan->getOperations()[0].module != r2.plan->getOperations()[0].module, "Unique runtime instances");
    }

    static void testParametricEQBounds(uint32_t& failCount) {
        ParametricEQModule eq20(20);
        TEST_EXPECT(eq20.getOwnedParameters().size() == 12 * 4, "Clamped to 12");

        ParametricEQModule eq0(0);
        TEST_EXPECT(eq0.getOwnedParameters().size() == 1 * 4, "Sanitized to 1");
    }

#ifdef POWEREQ_NATIVE_TESTS
    static void testPlanReclamationConcurrency(uint32_t& failCount) {
        std::atomic<uint32_t> destructionCount{0};

        {
            AudioEngine engine;

            // 1. Install Plan A
            AudioGraph gA;
            gA.addNode(1, std::make_unique<LifetimeProbeModule>(&destructionCount));
            engine.installGraph(gA);
            destructionCount = 0;

            // 2. reader starts
            engine.testPauseAcquisition_ = true;
            engine.testResumeAcquisition_ = false;
            engine.testAcquisitionReachedPause_ = false;

            std::thread reader([&]() {
                ExecutionPlan* p = engine.acquirePlanForAudioThread();
                if (p) {
                    engine.releasePlanForAudioThread();
                }
            });

            while (!engine.testAcquisitionReachedPause_) std::this_thread::yield();

            // 3. Control: Publish Plan B
            AudioGraph gB;
            gB.addNode(1, std::make_unique<PreampModule>());
            engine.installGraph(gB);

            // 4. Verify Plan A not destroyed
            engine.reclaimRetiredPlans();
            TEST_EXPECT(destructionCount == 0, "Plan A held by active reader");

            // 5. release reader
            engine.testResumeAcquisition_ = true;
            reader.join();

            // 6. reclaim
            engine.reclaimRetiredPlans();
            TEST_EXPECT(destructionCount >= 1, "Plan A destroyed after reader release");
        }
    }

    static void testPlanAcquisitionCollision(uint32_t& failCount) {
        AudioEngine engine;
        AudioGraph g; g.addNode(1, std::make_unique<PreampModule>());
        engine.installGraph(g);

        // Make sequence odd manually to simulate mid-publication
        engine.publicationSequence_.fetch_add(1);

        ExecutionPlan* p = engine.acquirePlanForAudioThread();
        TEST_EXPECT(p == nullptr, "Must return nullptr if publication in progress");

        engine.publicationSequence_.fetch_add(1); // Back to even
        p = engine.acquirePlanForAudioThread();
        TEST_EXPECT(p != nullptr, "Must succeed when stable");
        engine.releasePlanForAudioThread();
    }

    static void testConcurrentPublication(uint32_t& failCount) {
        AudioEngine engine;
        std::atomic<int> successCount{0};

        auto worker = [&]() {
            for (int i = 0; i < 10; ++i) {
                AudioGraph g; g.addNode(i + 1, std::make_unique<PreampModule>());
                auto res = engine.installGraph(g);
                if (res.isSuccess()) successCount++;
            }
        };

        std::thread t1(worker);
        std::thread t2(worker);
        t1.join();
        t2.join();

        TEST_EXPECT(successCount == 20, "All concurrent publications should succeed via mutex");
        TEST_EXPECT(engine.publicationSequence_.load() % 2 == 0, "Sequence must be even");
    }
#endif
};

#endif //EQUALIZERAPP_GRAPHTESTS_H
