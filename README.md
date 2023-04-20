## Introduction

This is a work-in-progress project to develop a real-time audio streaming solution, aimed particularly at musicians. 

## Features

- Real-time audio streaming 
- Cross-platform support using RTAudio
- Send/receive capability 

## Known Issues

- Packet loss on received data, can be heard when playing back the audio file
- Sub-optimal implementation of the buffer system, improvements being made
- RTAudio being used to provide cross-platform audio support, but Windows/MacOS versions not implemented 

Suggestions welcome!

## Installation Instructions

1. Clone the repo to your preferred directory
2. Initialise submodules: `git submodule init`
3. Update submoduels: `git submodule update`
4. Create build folder with `mkdir build`
5. Open the build folder with `cd build`
6. Run CMake with `cmake ..`
7. Run make `make`
8. Two executables should be generated, `./send` and `./receive`
9. The server code will need to be implemented separately

Note: both send and receive executables have a `--local` CLI parameter to send data to another machine on a local network, the IP address of said machine will have to be added manually at the moment.
