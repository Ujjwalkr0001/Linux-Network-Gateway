cmake_minimum_required(VERSION 3.15)
project(LinuxNetworkGateway
    VERSION 1.0.0
    DESCRIPTION "High-Performance C++ Linux Network Gateway (25-Phase Educational Project)"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type: Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug, Release, RelWithDebInfo)" FORCE)
endif()
message(STATUS "[LinuxNetworkGateway] Build type: ${CMAKE_BUILD_TYPE}")

# Include project headers directory
include_directories(${CMAKE_SOURCE_DIR}/include)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror=return-type
)

# Per-build-type compiler flags
set(CMAKE_CXX_FLAGS_DEBUG          "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")


# Compiler Sanitizer Options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

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
add_executable(phase10_routing_engine src/phase10_routing_engine.cpp)
add_executable(phase11_udp_server src/phase11_udp_server.cpp)
add_executable(phase12_rate_limiter src/phase12_rate_limiter.cpp)
add_executable(phase13_auth_system src/phase13_auth_system.cpp)
add_executable(phase14_async_logger src/phase14_async_logger.cpp)
add_executable(phase15_telemetry_dashboard src/phase15_telemetry_dashboard.cpp)
add_executable(phase16_benchmarking_suite src/phase16_benchmarking_suite.cpp)
add_executable(phase17_stress_fuzzer src/phase17_stress_fuzzer.cpp)
add_executable(phase18_resilience_audit src/phase18_resilience_audit.cpp)
add_executable(phase19_sanitizers_demo src/phase19_sanitizers_demo.cpp)
add_executable(phase20_unit_tests src/phase20_unit_tests.cpp)
add_executable(phase21_config_subsystem src/phase21_config_subsystem.cpp)
add_executable(phase22_security_audit src/phase22_security_audit.cpp)
add_executable(phase23_kernel_module_demo src/phase23_kernel_module_demo.cpp)
add_executable(phase24_packet_capture src/phase24_packet_capture.cpp)

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
target_link_libraries(phase10_routing_engine PRIVATE Threads::Threads)
target_link_libraries(phase11_udp_server PRIVATE Threads::Threads)
target_link_libraries(phase12_rate_limiter PRIVATE Threads::Threads)
target_link_libraries(phase13_auth_system PRIVATE Threads::Threads)
target_link_libraries(phase14_async_logger PRIVATE Threads::Threads)
target_link_libraries(phase15_telemetry_dashboard PRIVATE Threads::Threads)
target_link_libraries(phase16_benchmarking_suite PRIVATE Threads::Threads)
target_link_libraries(phase17_stress_fuzzer PRIVATE Threads::Threads)
target_link_libraries(phase18_resilience_audit PRIVATE Threads::Threads)
target_link_libraries(phase19_sanitizers_demo PRIVATE Threads::Threads)
target_link_libraries(phase20_unit_tests PRIVATE Threads::Threads)
target_link_libraries(phase21_config_subsystem PRIVATE Threads::Threads)
target_link_libraries(phase22_security_audit PRIVATE Threads::Threads)
target_link_libraries(phase23_kernel_module_demo PRIVATE Threads::Threads)
target_link_libraries(phase24_packet_capture PRIVATE Threads::Threads)

# CTest Integration
enable_testing()




add_test(NAME GatewayUnitTests COMMAND phase20_unit_tests)





