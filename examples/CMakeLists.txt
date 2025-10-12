if(NOT HTTC_BUILD_EXAMPLES)
    return()
endif()

add_executable(cob_spec)
target_sources(cob_spec
    PRIVATE
        cob_spec.cpp
)
target_link_libraries(cob_spec PRIVATE httc)

add_executable(simple)
target_sources(simple
    PRIVATE
        simple.cpp
)
target_link_libraries(simple PRIVATE httc)
