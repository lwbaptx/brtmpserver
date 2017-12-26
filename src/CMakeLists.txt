INCLUDE_DIRECTORIES(/usr/local/include/brpc)
Add_EXECUTABLE(brtmpserver server.cpp)
TARGET_LINK_LIBRARIES(brtmpserver brpc gflags pthread)
