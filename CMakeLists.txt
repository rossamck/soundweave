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

# Add the executable
add_executable(${PROJECT_NAME} src/combine.cpp)

# Link libraries
find_package(ALSA REQUIRED)
find_package(SDL2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ALSA::ALSA SDL2::SDL2)

# Add clean-all custom target
add_custom_target(clean-all
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/CleanAll.cmake"
                  COMMENT "Cleaning up the build directory, executables, and CMake cache")

