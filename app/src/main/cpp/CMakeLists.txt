cmake_minimum_required(VERSION 3.22.1)

project("equalizerapp")

add_library(
    equalizerapp
    SHARED
    native-lib.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    equalizerapp
    ${log-lib}
)