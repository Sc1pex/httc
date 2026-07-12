cmake_minimum_required(VERSION 3.28)

project(
    httc
    VERSION 0.3.0
    LANGUAGES CXX
)

include(FetchContent)

find_program(MOLD_PATH mold)
if(MOLD_PATH)
    add_link_options(-fuse-ld=mold)
endif()

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
target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED)

# Compiler warnings configuration
add_library(httc_warnings INTERFACE)
add_library(httc::warnings ALIAS httc_warnings)

option(HTTC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ${PROJECT_IS_TOP_LEVEL})

set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wpedantic
    -Wconversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
)

set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    -Wsuggest-override
)
# GCC's optimizer triggers false-positive -Wnull-dereference warnings
# when inlining Asio's executor comparison and scheduler logic (Asio Issue #1616).
list(REMOVE_ITEM GCC_WARNINGS -Wnull-dereference)

# GCC (specifically with UBSan and -O3) triggers false-positive -Wmismatched-new-delete warnings
# when tracing Asio's custom coroutine frame aligned allocators back to std::free.
list(APPEND GCC_WARNINGS -Wno-mismatched-new-delete)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(httc_warnings INTERFACE ${CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(httc_warnings INTERFACE ${GCC_WARNINGS})
endif()

if(HTTC_WARNINGS_AS_ERRORS)
    target_compile_options(httc_warnings INTERFACE -Werror)
endif()


add_subdirectory(src)

add_library(httc::httc ALIAS httc)

if(HTTC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(HTTC_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(HTTC_BUILD_TESTS)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.5.3
        SYSTEM
    )
    FetchContent_MakeAvailable(doctest)
    
    enable_testing()
    add_subdirectory(test)
endif()
