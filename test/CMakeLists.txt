add_executable(tests)
target_sources(tests
    PUBLIC
        request_parser.cpp
        headers.cpp
)
target_link_libraries(tests PUBLIC httc)
target_link_libraries(tests PUBLIC Catch2::Catch2WithMain)
