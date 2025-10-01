if(NOT HTTC_BUILD_TESTS)
    return()
endif()

add_executable(tests)
target_sources(tests
    PRIVATE
        headers.cpp
        percent_encoding.cpp
        request_parser.cpp
        router.cpp
        uri.cpp
)
target_link_libraries(tests PRIVATE httc::httc Catch2::Catch2WithMain)