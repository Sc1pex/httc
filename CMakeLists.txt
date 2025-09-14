cmake_minimum_required(VERSION 3.31)
project(Mnist)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(uvw CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

add_library(httc STATIC)
target_include_directories(httc PUBLIC include)
target_sources(httc 
    PUBLIC
        src/headers.cpp
        src/percent_encoding.cpp
        src/response.cpp
        src/request.cpp
        src/request_parser.cpp
        src/router.cpp
        src/server.cpp
        src/status.cpp
        src/uri.cpp
        src/utils/file_server.cpp
)
target_link_libraries(httc PUBLIC uvw::uvw)

add_subdirectory(bin)
add_subdirectory(test)
