cmake_minimum_required(VERSION 3.16)
project(fm_relay_phase1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libs" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HACKRF REQUIRED libhackrf)

add_executable(fm_relay
  src/main.cpp
  src/WavWriter.cpp
  src/AudioRingBuffer.cpp
  src/HackRFDevice.cpp
  src/FMReceiver.cpp
  src/ChannelFilter.cpp
  src/FMDemodulator.cpp
  src/AudioResampler.cpp
  src/RDSDecoder.cpp
  src/dsp/FIRDecimator.cpp
)

target_include_directories(fm_relay PRIVATE include ${HACKRF_INCLUDE_DIRS})
target_link_directories(fm_relay PRIVATE ${HACKRF_LIBRARY_DIRS})
target_link_libraries(fm_relay PRIVATE ${HACKRF_LIBRARIES})

if (WIN32)
  target_compile_definitions(fm_relay PRIVATE NOMINMAX)
endif()
