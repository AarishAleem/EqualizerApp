cmake_minimum_required(VERSION 3.22.1)

project("equalizerapp")

find_package(oboe REQUIRED CONFIG)

set(COMMON_SOURCES
    Biquad.cpp
    AudioEngine.cpp
    FrequencyResponseAnalyzer.cpp
    GraphCompiler.cpp
    ExecutionPlan.cpp
)

add_library(
    equalizerapp
    SHARED
    native-lib.cpp
    ${COMMON_SOURCES}
)

# Dedicated Native Test Build Target
add_executable(
    PowerEQNativeTests
    tests/TestRunner.cpp
    ${COMMON_SOURCES}
)

target_compile_definitions(PowerEQNativeTests PRIVATE POWEREQ_NATIVE_TESTS=1)

find_library(
    log-lib
    log
)

target_link_libraries(
    equalizerapp
    oboe::oboe
    ${log-lib}
)

target_link_libraries(
    PowerEQNativeTests
    oboe::oboe
    ${log-lib}
)
