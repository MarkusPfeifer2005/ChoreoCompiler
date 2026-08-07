cmake_minimum_required(VERSION 3.23)
project(main LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)
FetchContent_Declare(
  podofo
  GIT_REPOSITORY https://github.com/podofo/podofo.git
  GIT_TAG        1.1.1   # pin to a real tagged release
)
set(PODOFO_BUILD_LIB_ONLY TRUE CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(podofo)

find_package(Qt6 REQUIRED COMPONENTS Widgets Pdf)

add_executable(coco main.cpp dance.cpp utils.cpp gui.cpp export.cpp)

target_include_directories(coco PRIVATE "${podofo_BINARY_DIR}/src/podofo/podofo/auxiliary")
target_link_libraries(coco PRIVATE Qt6::Widgets Qt6::Pdf nlohmann_json::nlohmann_json podofo::podofo)

install(TARGETS coco DESTINATION bin)
