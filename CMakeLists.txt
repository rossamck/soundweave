cmake_minimum_required(VERSION 3.10)

# Set project name, version, and company
project(combine
        VERSION 1.0.0
        DESCRIPTION "SoundWeave project"
        HOMEPAGE_URL "https://example.com"
        LANGUAGES CXX)

# Set company name
set(COMPANY_NAME "SoundWeave")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source files
set(NETWORKING_SOURCES src/Networking/holepunch.cpp src/Networking/RTP.cpp)
set(AUDIOCAPTURE_BASE_SOURCES src/AudioCapture/AudioCaptureBase.cpp src/AudioCapture/PingPongBuffer.cpp)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(AUDIOCAPTURE_PLATFORM_SOURCES src/AudioCapture/AudioCaptureLinux.cpp)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(AUDIOCAPTURE_PLATFORM_SOURCES src/AudioCapture/AudioCaptureMac.cpp)
endif()

set(AUDIOCAPTURE_SOURCES ${AUDIOCAPTURE_BASE_SOURCES} ${AUDIOCAPTURE_PLATFORM_SOURCES})

set(CMAKE_EXE_LINKER_FLAGS "-pthread")

# Add the 'send' executable
add_executable(send ${NETWORKING_SOURCES} ${AUDIOCAPTURE_SOURCES} src/holepunchrtp2.cpp)

# Add the 'receive' executable
add_executable(receive ${NETWORKING_SOURCES} ${AUDIOCAPTURE_SOURCES} src/holepunchwaveform.cpp)

# Link libraries
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(ALSA REQUIRED)
    find_package(SDL2 REQUIRED)
if(TARGET SDL2::SDL2)
    target_link_libraries(send PRIVATE SDL2::SDL2)
    target_link_libraries(receive PRIVATE SDL2::SDL2)
else()
    target_link_libraries(send PRIVATE ${SDL2_LIBRARIES})
    target_link_libraries(receive PRIVATE ${SDL2_LIBRARIES})
    include_directories(${SDL2_INCLUDE_DIRS})
endif()
    target_link_libraries(send PRIVATE ALSA::ALSA)
    target_link_libraries(receive PRIVATE ALSA::ALSA)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    find_library(AUDIO_TOOLBOX_LIBRARY AudioToolbox)
    find_library(CORE_AUDIO_LIBRARY CoreAudio)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(send PRIVATE ${AUDIO_TOOLBOX_LIBRARY} ${CORE_AUDIO_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    target_link_libraries(receive PRIVATE ${AUDIO_TOOLBOX_LIBRARY} ${CORE_AUDIO_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
    find_package(SDL2 REQUIRED)
    target_link_libraries(send PRIVATE ${SDL2_LIBRARIES})
    target_link_libraries(receive PRIVATE ${SDL2_LIBRARIES})
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Add clean-all custom target
add_custom_target(clean-all
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/CleanAll.cmake"
                  COMMENT "Cleaning up the build directory, executables, and CMake cache")
