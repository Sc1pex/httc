add_executable(simple simple.cpp)
target_link_libraries(simple PRIVATE httc)

add_executable(cookies cookies.cpp)
target_link_libraries(cookies PRIVATE httc)
