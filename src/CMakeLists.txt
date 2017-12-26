add_executable(brtmpserver server.cpp)
target_link_libraries(brtmpserver brpc gflags pthread)