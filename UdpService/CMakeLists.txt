cmake_minimum_required(VERSION 3.13)
project(UdpService C)

set(CMAKE_C_STANDARD 99)

add_executable(UdpService main.c)
find_package(Threads REQUIRED)
target_link_libraries(UdpService Threads::Threads)