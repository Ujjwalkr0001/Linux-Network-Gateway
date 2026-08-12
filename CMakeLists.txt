cmake_minimum_required(VERSION 3.15)
project(LinuxNetworkGateway CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include project headers directory
include_directories(${CMAKE_SOURCE_DIR}/include)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror=return-type
    -g
)

# Executable Targets
add_executable(env_check tools/env_check.cpp)
add_executable(phase1_tcp_server src/phase1_tcp_server.cpp)
add_executable(phase2_tcp_echo_server src/phase2_tcp_echo_server.cpp)
add_executable(phase3_multithreaded_server src/phase3_multithreaded_server.cpp)
add_executable(phase4_threadpool_server src/phase4_threadpool_server.cpp)
add_executable(phase5_binary_protocol src/phase5_binary_protocol.cpp)
add_executable(phase6_non_blocking_server src/phase6_non_blocking_server.cpp)
add_executable(phase7_epoll_demo src/phase7_epoll_demo.cpp)
add_executable(phase8_reactor_server src/phase8_reactor_server.cpp)
add_executable(phase9_connection_manager_server src/phase9_connection_manager_server.cpp)

find_package(Threads REQUIRED)
target_link_libraries(env_check PRIVATE Threads::Threads)
target_link_libraries(phase1_tcp_server PRIVATE Threads::Threads)
target_link_libraries(phase2_tcp_echo_server PRIVATE Threads::Threads)
target_link_libraries(phase3_multithreaded_server PRIVATE Threads::Threads)
target_link_libraries(phase4_threadpool_server PRIVATE Threads::Threads)
target_link_libraries(phase5_binary_protocol PRIVATE Threads::Threads)
target_link_libraries(phase6_non_blocking_server PRIVATE Threads::Threads)
target_link_libraries(phase7_epoll_demo PRIVATE Threads::Threads)
target_link_libraries(phase8_reactor_server PRIVATE Threads::Threads)
target_link_libraries(phase9_connection_manager_server PRIVATE Threads::Threads)
