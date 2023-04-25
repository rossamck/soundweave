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

set(NETWORKING_SOURCES src/Networking/ClientManager.cpp)
set(AUDIOCAPTURE_SOURCES src/AudioCapture/AudioCapture.cpp src/AudioCapture/PingPongBuffer.cpp)
set(AUDIOWAVEFORM_SOURCES src/AudioPlayback/AudioWaveform.cpp src/AudioPlayback/RawAudioWriter.cpp)

# Check if the system is Windows
if (WIN32)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
else()
    set(CMAKE_EXE_LINKER_FLAGS "-pthread")
    find_package(SDL2 REQUIRED)
    if (NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 UNKNOWN IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION "${SDL2_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}")
    endif()
endif()

add_executable(send ${NETWORKING_SOURCES} ${AUDIOCAPTURE_SOURCES} src/holepunchrtp2.cpp)

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS system thread)

# Set target link libraries depending on the system
if (WIN32)
    target_link_libraries(send PRIVATE rtaudio Boost::system Boost::thread)
else()
    target_link_libraries(send PRIVATE SDL2::SDL2 rtaudio Boost::system Boost::thread)
endif()

target_include_directories(send PRIVATE deps/rtaudio ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_target(clean-all
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/CleanAll.cmake"
                  COMMENT "Cleaning up the build directory, executables, and CMake cache")

# Add the -g debugging command
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
