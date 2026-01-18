add_executable(unit_tests)

target_sources(unit_tests PRIVATE
    async_test.hpp
    headers.cpp
    percent_encoding.cpp
    request_parser.cpp
    response.cpp
    router.cpp
    status.cpp
    uri.cpp
)

target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain httc)

include(Catch)
catch_discover_tests(unit_tests)
