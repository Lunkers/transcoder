cmake_minimum_required(VERSION 3.1)

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


add_executable(${PROJECT_NAME}
    src/main.cpp
    src/AV/src/transcoder.hpp
    src/AV/src/transmuxer.hpp
    src/AV/src/transcoder.cpp
    src/AV/src/transmuxer.cpp
)

target_link_libraries(${PROJECT_NAME}
    PkgConfig::LIBAV
)

target_include_directories(${PROJECT_NAME} PRIVATE src/AV)
