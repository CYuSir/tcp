cmake_minimum_required(VERSION 3.10)

project(tcp_epoll_demo)

set(CMAKE_CXX_STANDARD 14)

# Server executable
add_executable(server
    src/server.cpp
    src/thread.cpp
    include/server.h
    include/thread.h
    include/ThreadPool.h
)

# Client executable
add_executable(client
    src/client.cpp
    include/client.h
)

# Link libraries
target_link_libraries(server PRIVATE pthread)
target_link_libraries(client PRIVATE pthread)

# Include directories
target_include_directories(server PRIVATE include)
target_include_directories(client PRIVATE include)