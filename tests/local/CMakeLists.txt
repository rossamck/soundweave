cmake_minimum_required(VERSION 3.15)
project(receive)

# Set the path to the vcpkg toolchain file
set(VCPKG_TOOLCHAIN "C:/Users/rossm/Documents/git/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Use the vcpkg toolchain file to locate RTAudio
find_package(RtAudio REQUIRED)

# Specify the location of the Boost root directory and libraries
set(BOOST_ROOT "C:/Boost")
set(BOOST_LIBRARYDIR "C:/Boost/lib")
find_package(Boost COMPONENTS system thread REQUIRED)

# Include the Boost and RTAudio header files
include_directories(${RTAUDIO_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# Add the executable
add_executable(receive receive.cpp)

# Link the Boost and RTAudio libraries
target_link_libraries(receive PRIVATE ${RTAUDIO_LIBRARIES} ${Boost_LIBRARIES})
