cmake_minimum_required(VERSION 3.15)
project(metrics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(METRICS_BUILD_EXAMPLES "Build examples" OFF)

add_library(metrics
    src/collector.cpp
    src/histogram.cpp
)

target_include_directories(metrics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/metrics>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(metrics PROPERTIES
    PUBLIC_HEADER "include/metrics/counter.hpp"
    PUBLIC_HEADER "include/metrics/gauge.hpp"
    PUBLIC_HEADER "include/metrics/histogram.hpp"
    PUBLIC_HEADER "include/metrics/info.hpp"
    PUBLIC_HEADER "include/collector.hpp"
)

install(TARGETS metrics
    EXPORT metrics-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/metrics
)

install(DIRECTORY include/metrics
    DESTINATION include
)

install(EXPORT metrics-targets
    FILE metrics-config.cmake
    NAMESPACE metrics::
    DESTINATION lib/cmake/metrics
)

if(METRICS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()