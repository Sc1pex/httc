add_executable(unit_tests)

target_sources(unit_tests PRIVATE
    main.cpp
    async_test.hpp
    headers.cpp
    percent_encoding.cpp
    request_parser.cpp
    response.cpp
    router.cpp
    status.cpp
    uri.cpp
)

target_link_libraries(unit_tests PRIVATE doctest::doctest httc httc_warnings)

list(APPEND CMAKE_MODULE_PATH "${doctest_SOURCE_DIR}/scripts/cmake")
include(doctest)
doctest_discover_tests(unit_tests)
