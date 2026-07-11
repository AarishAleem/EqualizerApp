#ifndef EQUALIZERAPP_GRAPHTESTS_H
#define EQUALIZERAPP_GRAPHTESTS_H

#include "../GraphCompiler.h"
#include "../GraphExecutor.h"
#include "../PreampModule.h"
#include <cassert>
#include <android/log.h>

#define TEST_LOG(...) __android_log_print(ANDROID_LOG_INFO, "GraphTests", __VA_ARGS__)

class GraphTests {
public:
    static void runAll() {
        TEST_LOG("Running native graph tests...");
        testProcessSpecValidation();
        testChainCompilation();
        testVariableBlockProcessing();
        testBoundsViolation();
        testResetBehavior();
        testReadOnTap();
        TEST_LOG("All native graph tests PASSED.");
    }

private:
    static void testProcessSpecValidation() {
        ProcessSpec valid{48000.0, 512, 2};
        assert(valid.isValid());

        ProcessSpec invalidRate{0.0, 512, 2};
        assert(!invalidRate.isValid());

        ProcessSpec invalidFrames{48000.0, 0, 2};
        assert(!invalidFrames.isValid());
    }

    static void testChainCompilation() {
        AudioGraph graph;
        graph.addNode(1, std::make_unique<PreampModule>());
        graph.addNode(2, std::make_unique<PreampModule>());
        graph.setLink(1, 2);

        GraphCompiler compiler;
        ProcessSpec spec{48000.0, 512, 2};
        auto result = compiler.compile(graph, spec);

        assert(result.isSuccess());
        assert(result.plan->getOperations().size() == 2);
    }

    static void testVariableBlockProcessing() {
        AudioGraph graph;
        graph.addNode(1, std::make_unique<PreampModule>());

        GraphCompiler compiler;
        ProcessSpec spec{48000.0, 512, 2};
        auto result = compiler.compile(graph, spec);

        GraphExecutor executor;
        ProcessContext context;
        float l[512], r[512];
        context.left = l;
        context.right = r;

        // Small block
        context.numFrames = 64;
        executor.execute(result.plan.get(), context);
        assert(!context.limitExceeded);

        // Max block
        context.numFrames = 512;
        executor.execute(result.plan.get(), context);
        assert(!context.limitExceeded);
    }

    static void testBoundsViolation() {
        AudioGraph graph;
        graph.addNode(1, std::make_unique<PreampModule>());
        GraphCompiler compiler;
        ProcessSpec spec{48000.0, 128, 2};
        auto result = compiler.compile(graph, spec);

        GraphExecutor executor;
        ProcessContext context;
        context.numFrames = 256; // > 128
        executor.execute(result.plan.get(), context);
        assert(context.limitExceeded);
    }

    static void testResetBehavior() {
        // Preamp reset implementation check
        auto preamp = std::make_unique<PreampModule>();
        preamp->prepare({48000.0, 512, 2});
        preamp->reset();
        // Just verify it doesn't crash
    }

    static void testReadOnTap() {
        AudioGraph graph;
        auto preamp = std::make_unique<PreampModule>();
        FrameCounterTap tap;

        // Manual assembly for test
        graph.addNode(1, std::move(preamp));
        graph.getNodes()[0].tap = &tap;

        GraphCompiler compiler;
        ProcessSpec spec{48000.0, 512, 2};
        auto result = compiler.compile(graph, spec);

        GraphExecutor executor;
        ProcessContext context;
        float l[512], r[512];
        context.left = l; context.right = r;
        context.numFrames = 100;

        executor.execute(result.plan.get(), context);
        assert(tap.getCount() == 100);
    }
};

#endif //EQUALIZERAPP_GRAPHTESTS_H
