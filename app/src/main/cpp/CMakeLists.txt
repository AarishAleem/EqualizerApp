cmake_minimum_required(VERSION 3.22.1)

project("equalizerapp")

find_package(oboe REQUIRED CONFIG)

add_library(
    equalizerapp
    SHARED
    native-lib.cpp
    Biquad.cpp
    AudioEngine.cpp
    DSPPipeline.cpp
    FrequencyResponseAnalyzer.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    equalizerapp
    oboe::oboe
    ${log-lib}
)