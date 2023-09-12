cmake_minimum_required(VERSION 3.15.0)

project(SDR_Channelizer
          VERSION
            1.0
          DESCRIPTION
            "TBD"
          LANGUAGES
            CXX
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable (blade_recorder.out record_iq_blade.cpp)

set_property(TARGET blade_recorder.out PROPERTY CXX_STANDARD 20)

target_include_directories(blade_recorder.out PRIVATE /usr/local/include)

target_link_libraries(blade_recorder.out PRIVATE /usr/local/lib/libbladeRF.dylib)
