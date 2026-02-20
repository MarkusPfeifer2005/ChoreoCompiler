cmake_minimum_required(VERSION 3.5)
project(main LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE Qt6::Widgets)
