add_executable(cob_spec)
target_sources(cob_spec
    PUBLIC
        cob_spec.cpp
)
target_link_libraries(cob_spec PUBLIC httc)
