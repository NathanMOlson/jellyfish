cmake_minimum_required(VERSION 3.22.1)
project(gstreamer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)

include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS} )

add_executable(jelly src/main.cpp)
target_compile_options(jelly PUBLIC -Wall -Wextra -pedantic -Werror -Wl,--fatal-warnings)
target_link_libraries(jelly gstjellyfilter)

add_library(gstjellyfilter SHARED src/gstjellyfilter.cpp src/jelly_filter.cpp)
target_compile_options(gstjellyfilter PUBLIC -Wno-missing-field-initializers -Wno-unused-parameter -Wno-pedantic)

target_link_libraries(gstjellyfilter
PkgConfig::gstreamer
PkgConfig::gstreamer-video
PkgConfig::gstreamer-app
${OpenCV_LIBS}
)
