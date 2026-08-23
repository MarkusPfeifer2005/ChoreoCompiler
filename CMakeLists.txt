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
set(PODOFO_BUILD_STATIC TRUE CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(podofo)

find_package(Qt6 REQUIRED COMPONENTS Widgets Pdf)

add_executable(coco MACOSX_BUNDLE main.cpp dance.cpp utils.cpp gui.cpp export.cpp)

if(APPLE)
    set_target_properties(coco PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "CoCo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "at.markus.coco"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    )
endif()

target_include_directories(coco PRIVATE "${podofo_BINARY_DIR}/src/podofo/podofo/auxiliary")
target_link_libraries(coco PRIVATE Qt6::Widgets Qt6::Pdf nlohmann_json::nlohmann_json podofo::podofo)

install(TARGETS coco DESTINATION bin)

set(CPACK_PACKAGE_NAME "coco")
set(CPACK_PACKAGE_VERSION "1.0.0-alpha")
set(CPACK_PACKAGE_CONTACT "CoCo Project <https://github.com/MarkusPfeifer2005/ChoreoCompiler>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Choreography editing and compilation tool")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6gui6, libqt6widgets6, libqt6pdf6")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

include(CPack)
