cmake_minimum_required(VERSION 3.31)
project(httc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(PROJECT_HOMEPAGE_URL "https://github.com/Sc1pex/httc")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(asio CONFIG REQUIRED)

option(HTTC_BUILD_TESTS "Build tests")
option(HTTC_BUILD_EXAMPLES "Build examples")

if(HTTC_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
endif()

add_library(httc)
add_library(httc::httc ALIAS httc) 

target_include_directories(httc 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_sources(httc 
    PRIVATE
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
        src/utils/mime.cpp
)

target_link_libraries(httc PUBLIC asio::asio)

set_target_properties(httc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME httc
)

if(HTTC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(HTTC_BUILD_TESTS)
    add_subdirectory(test)
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install the library
install(TARGETS httc
    EXPORT httcTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Generate and install config files
set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(EXPORT httcTargets
    FILE httcTargets.cmake
    NAMESPACE httc::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/httcConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/httcConfig.cmake"
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/httcConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/httcConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/httcConfigVersion.cmake"
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# Export targets for build tree usage
export(EXPORT httcTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/httcTargets.cmake"
    NAMESPACE httc::
)
