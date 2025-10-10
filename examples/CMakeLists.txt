if(NOT HTTC_BUILD_EXAMPLES)
    return()
endif()

add_executable(cob_spec)
target_sources(cob_spec
    PRIVATE
        cob_spec.cpp
)
target_link_libraries(cob_spec PRIVATE httc)
