cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(ffmpeg-experiments)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

# include 
FetchContent_Declare(
		yaml-cpp
		GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
		GIT_SHALLOW	ON
		GIT_TAG yaml-cpp-0.6.3
)

FetchContent_GetProperties(yaml-cpp)
if(NOT yaml-cpp_POPULATED)
    message(STATUS "populating yaml-cpp")
    FetchContent_Populate(yaml-cpp)
    set(YAML_CPP_BUILD_TESTS OFF)
    add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})
    message(STATUS "done.")
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/AV/src/transcoder.hpp
    src/AV/src/transmuxer.hpp
    src/AV/src/transcoder.cpp
    src/AV/src/transmuxer.cpp
)

target_link_libraries(${PROJECT_NAME}
    PkgConfig::LIBAV
    yaml-cpp
)

