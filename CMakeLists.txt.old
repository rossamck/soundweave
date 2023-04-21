cmake_minimum_required(VERSION 3.10)

project(combine
        VERSION 1.0.0
        DESCRIPTION "SoundWeave project"
        HOMEPAGE_URL "https://example.com"
        LANGUAGES CXX)

set(COMPANY_NAME "SoundWeave")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add RtAudio as a dependency
add_subdirectory(deps/rtaudio)

set(NETWORKING_SOURCES src/Networking/holepunch.cpp src/Networking/RTP.cpp)
set(AUDIOCAPTURE_SOURCES src/AudioCapture/AudioCapture.cpp src/AudioCapture/PingPongBuffer.cpp)
set(CMAKE_EXE_LINKER_FLAGS "-pthread")

add_executable(send ${NETWORKING_SOURCES} ${AUDIOCAPTURE_SOURCES} src/holepunchrtp2.cpp)
add_executable(receive ${NETWORKING_SOURCES} ${AUDIOCAPTURE_SOURCES} src/holepunchwaveform.cpp)

find_package(ALSA REQUIRED)

find_package(SDL2 REQUIRED)
if (NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}")
endif()

target_link_libraries(send PRIVATE ALSA::ALSA SDL2::SDL2 rtaudio)
target_link_libraries(receive PRIVATE ALSA::ALSA SDL2::SDL2 rtaudio)

target_include_directories(send PRIVATE deps/rtaudio)
target_include_directories(receive PRIVATE deps/rtaudio)

add_custom_target(clean-all
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/CleanAll.cmake"
                  COMMENT "Cleaning up the build directory, executables, and CMake cache")
