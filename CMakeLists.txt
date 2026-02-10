cmake_minimum_required(VERSION 3.28)

project(
    httc
    VERSION 0.1.0
    LANGUAGES CXX
)

include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

option(HTTC_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(HTTC_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(HTTC_BUILD_BENCHMARKS "Build benchmarks" OFF)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(asio INTERFACE ASIO_HAS_IO_URING)
endif()

add_subdirectory(src)

if(HTTC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(HTTC_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(HTTC_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.12.0
    )
    FetchContent_MakeAvailable(Catch2)
    
    enable_testing()
    add_subdirectory(test)
endif()
