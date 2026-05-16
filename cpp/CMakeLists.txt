cmake_minimum_required(VERSION 3.15.0)

project(SDR_Channelizer
          VERSION
            1.0
          DESCRIPTION
            "TBD"
          LANGUAGES
            CXX
)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBLADERF REQUIRED IMPORTED_TARGET libbladeRF)

add_executable (blade_record_iq_08bit.out blade_record_iq_08bit.cpp Helper.cpp)
set_property(TARGET blade_record_iq_08bit.out PROPERTY CXX_STANDARD 20)
target_link_libraries(blade_record_iq_08bit.out PRIVATE PkgConfig::LIBBLADERF)

add_executable (blade_record_iq_12bit.out blade_record_iq_12bit.cpp Helper.cpp)
set_property(TARGET blade_record_iq_12bit.out PROPERTY CXX_STANDARD 20)
target_link_libraries(blade_record_iq_12bit.out PRIVATE PkgConfig::LIBBLADERF)

add_executable (blade_find_max_unsaturated_gain.out blade_find_max_unsaturated_gain.cpp)
set_property(TARGET blade_find_max_unsaturated_gain.out PROPERTY CXX_STANDARD 11)
target_link_libraries(blade_find_max_unsaturated_gain.out PRIVATE PkgConfig::LIBBLADERF)

add_executable (blade_record_iq_08bit_agc.out blade_record_iq_08bit_agc.cpp Helper.cpp)
set_property(TARGET blade_record_iq_08bit_agc.out PROPERTY CXX_STANDARD 20)
target_link_libraries(blade_record_iq_08bit_agc.out PRIVATE PkgConfig::LIBBLADERF)

add_executable (blade_record_iq_12bit_agc.out blade_record_iq_12bit_agc.cpp Helper.cpp)
set_property(TARGET blade_record_iq_12bit_agc.out PROPERTY CXX_STANDARD 20)
target_link_libraries(blade_record_iq_12bit_agc.out PRIVATE PkgConfig::LIBBLADERF)

find_package(UHD 4.5.0 REQUIRED)
find_package(Boost 1.65 REQUIRED)

message(UHD_INCLUDE_DIRS="${UHD_INCLUDE_DIRS}")
message(UHD_LIBRARIES="${UHD_LIBRARIES}")
message(Boost_INCLUDE_DIRS="${Boost_INCLUDE_DIRS}")

add_executable (usrp_record_iq_08bit.out usrp_record_iq_08bit.cpp Helper.cpp)
set_property(TARGET usrp_record_iq_08bit.out PROPERTY CXX_STANDARD 20)
target_include_directories(usrp_record_iq_08bit.out PRIVATE ${Boost_INCLUDE_DIRS} ${UHD_INCLUDE_DIRS})
target_link_libraries(usrp_record_iq_08bit.out ${UHD_LIBRARIES})

add_executable (usrp_record_iq_12bit.out usrp_record_iq_12bit.cpp Helper.cpp)
set_property(TARGET usrp_record_iq_12bit.out PROPERTY CXX_STANDARD 20)
target_include_directories(usrp_record_iq_12bit.out PRIVATE ${Boost_INCLUDE_DIRS} ${UHD_INCLUDE_DIRS})
target_link_libraries(usrp_record_iq_12bit.out ${UHD_LIBRARIES})

add_executable (usrp_find_max_unsaturated_gain.out usrp_find_max_unsaturated_gain.cpp)
set_property(TARGET usrp_find_max_unsaturated_gain.out PROPERTY CXX_STANDARD 17)
target_include_directories(usrp_find_max_unsaturated_gain.out PRIVATE ${Boost_INCLUDE_DIRS} ${UHD_INCLUDE_DIRS})
target_link_libraries(usrp_find_max_unsaturated_gain.out ${UHD_LIBRARIES})

add_executable (usrp_record_iq_12bit_agc.out usrp_record_iq_12bit_agc.cpp Helper.cpp)
set_property(TARGET usrp_record_iq_12bit_agc.out PROPERTY CXX_STANDARD 20)
target_include_directories(usrp_record_iq_12bit_agc.out PRIVATE ${Boost_INCLUDE_DIRS} ${UHD_INCLUDE_DIRS})
target_link_libraries(usrp_record_iq_12bit_agc.out ${UHD_LIBRARIES})

add_executable (usrp_record_iq_08bit_agc.out usrp_record_iq_08bit_agc.cpp Helper.cpp)
set_property(TARGET usrp_record_iq_08bit_agc.out PROPERTY CXX_STANDARD 20)
target_include_directories(usrp_record_iq_08bit_agc.out PRIVATE ${Boost_INCLUDE_DIRS} ${UHD_INCLUDE_DIRS})
target_link_libraries(usrp_record_iq_08bit_agc.out ${UHD_LIBRARIES})

