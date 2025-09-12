add_executable(tests)
target_sources(tests
    PUBLIC
        headers.cpp
        percent_encoding.cpp
        request_parser.cpp
        router.cpp
        uri.cpp
)
target_link_libraries(tests PUBLIC httc)
target_link_libraries(tests PUBLIC Catch2::Catch2WithMain)
