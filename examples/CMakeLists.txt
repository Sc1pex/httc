add_executable(simple simple.cpp)
target_link_libraries(simple PRIVATE httc)

add_executable(cookies cookies.cpp)
target_link_libraries(cookies PRIVATE httc)

add_executable(static_server static_server/main.cpp)
target_link_libraries(static_server PRIVATE httc)
