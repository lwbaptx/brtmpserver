cmake_minimum_required(VERSION 2.8)
project(brtmpserver)

find_library(GFLAGS_LIBRARY NAMES gflags)
if(GFLAGS_LIBRARY MATCHES GFLAGS_LIBRARY-NOTFOUND)
    message(FATAL_ERROR "gflags library not found")
endif()

find_library(BRPC_LIBRARY NAMES brpc)
if(BRPC_LIBRARY MATCHES BRPC_LIBRARY-NOTFOUND)
    message(FATAL_ERROR "brpc library not found")
endif()

add_subdirectory(src bin)
