cmake_minimum_required(VERSION 3.8)
project(rtaudiotest)

# Set the C++ standard to use
set(CMAKE_CXX_STANDARD 14)

# Add the RtAudio source directory
add_subdirectory(rtaudio)

# Add the executable target for your main.cpp file
add_executable(test main.cpp AudioCapture/AudioCapture.cpp)


# Link the RtAudio library to your executable
target_link_libraries(test rtaudio)
