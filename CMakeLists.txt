cmake_minimum_required(VERSION 3.8)

project(soundweave)

set(SDL2_INCLUDE_DIR "C:/Users/rossm/Libraries/SDL2/include")
set(SDL2_LIB_DIR "C:/Users/rossm/Libraries/SDL2/lib/x64")

include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})

add_executable(windowstest
  windowstest.cpp
  src/AudioCapture/AudioCapture.cpp
)

target_include_directories(windowstest PRIVATE deps/rtaudio)
target_link_libraries(windowstest
  PRIVATE
    C:/Users/rossm/Documents/git/soundweave/deps/rtaudio/build/Debug/rtaudiod.lib
    SDL2main
    SDL2
    ws2_32
)
