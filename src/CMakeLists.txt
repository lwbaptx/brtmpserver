add_executable(brtmpserver main.cpp rtmpservice.cpp)
target_link_libraries(brtmpserver brpc gflags pthread)