cmake_minimum_required(VERSION 3.25)
project(order)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_executable(agg agg_trade.cpp)
add_executable(sort sort.cpp)
add_executable(footprint footprint.cpp json.hpp footprint.h)
target_link_libraries(agg pthread)
target_link_libraries(sort pthread)
target_link_libraries(footprint pthread)