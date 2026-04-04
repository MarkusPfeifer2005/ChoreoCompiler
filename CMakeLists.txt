cmake_minimum_required(VERSION 3.5)
project(main LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

find_package(Qt6 REQUIRED COMPONENTS Widgets Pdf)

add_executable(coco main.cpp)

target_link_libraries(coco PRIVATE Qt6::Widgets Qt6::Pdf nlohmann_json::nlohmann_json)

install(TARGETS coco DESTINATION bin)
