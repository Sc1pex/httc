add_executable(helloworld)
target_sources(helloworld 
    PUBLIC
        hello-world.cpp
)
target_link_libraries(helloworld PUBLIC httc)
