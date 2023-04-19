#include "AudioCaptureMac.h"
#include <iostream>
#include <stdexcept>

AudioCaptureMac::AudioCaptureMac(std::string device_name, bool dummy_audio)
    : AudioCaptureBase(device_name, dummy_audio)
{
    if (!dummy_audio)
    {
        openDevice(device_name);
        startCapture();
    }

    if (dummy_audio)
    {
        captureThread = std::thread(&AudioCaptureMac::DummyAudioThreadFunction, this);
    }
    else
    {
        captureThread = std::thread(&AudioCaptureMac::CaptureThreadFunction, this);
    }

    buffer_.set_on_buffer_full_callback(call_callbacks);
}

AudioCaptureMac::~AudioCaptureMac()
{
    std::cout << "Closing audiocapture" << std::endl;
    closeDevice();

    quit = true;
    if (captureThread.joinable())
    {
        captureThread.join();
    }
}

void AudioCaptureMac::openDevice(const std::string &device_name)
{
    // Create an AudioComponentDescription for the desired Audio Unit
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    // Find an AudioComponent that meets the description
    AudioComponent inputComponent = AudioComponentFindNext(nullptr, &desc);

    // Create an instance of the Audio Unit
    OSStatus status = AudioComponentInstanceNew(inputComponent, &audioUnit);
    if (status != noErr)
    {
        std::cerr << "Error creating AudioUnit instance" << std::endl;
        throw std::runtime_error("Failed to create AudioUnit instance");
    }

    // Enable input on the Audio Unit
    UInt32 enableIO = 1;
    status = AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
    if (status != noErr)
    {
        std::cerr << "Error enabling input on AudioUnit" << std::endl;
        throw std::runtime_error("Failed to enable input on AudioUnit");
    }

    // Disable output on the Audio Unit
    enableIO = 0;
    status = AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
    if (status != noErr)
    {
        std::cerr << "Error disabling output on AudioUnit" << std::endl;
        throw std::runtime_error("Failed to disable output on AudioUnit");
    }
    // Set the callback function
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = MyCallback;
    callbackStruct.inputProcRefCon = this;
    status = AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &callbackStruct, sizeof(callbackStruct));
    if (status != noErr)
    {
        std::cerr << "Error setting input callback on AudioUnit" << std::endl;
        throw std::runtime_error("Failed to set input callback on AudioUnit");
    }

    // Set the audio stream format
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = 44100;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mChannelsPerFrame = 1;
    streamFormat.mBitsPerChannel = 16;
    streamFormat.mBytesPerPacket = 2;
    streamFormat.mBytesPerFrame = 2;

    status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &streamFormat, sizeof(streamFormat));
    if (status != noErr)
    {
        std::cerr << "Error setting stream format on AudioUnit" << std::endl;
        throw std::runtime_error("Failed to set stream format on AudioUnit");
    }

    // Initialize the Audio Unit
    status = AudioUnitInitialize(audioUnit);
    if (status != noErr)
    {
        std::cerr << "Error initializing AudioUnit" << std::endl;
        throw std::runtime_error("Failed to initialize AudioUnit");
    }
}

void AudioCaptureMac::closeDevice()
{
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
}

void AudioCaptureMac::startCapture()
{
    OSStatus status = AudioOutputUnitStart(audioUnit);
    if (status != noErr)
    {
        std::cerr << "Error starting AudioUnit" << std::endl;
        throw std::runtime_error("Failed to start AudioUnit");
    }
}

void AudioCaptureMac::stopCapture()
{
    AudioOutputUnitStop(audioUnit);
}

OSStatus AudioCaptureMac::MyCallback(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData)
{
    AudioCaptureMac *instance = static_cast<AudioCaptureMac *>(inRefCon);
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mDataByteSize = inNumberFrames * 2;
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mData = malloc(inNumberFrames * 2);

    OSStatus status = AudioUnitRender(instance->audioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, &bufferList);
    if (status != noErr)
    {
        std::cerr << "Error in AudioUnitRender" << std::endl;
        free(bufferList.mBuffers[0].mData);
        return status;
    }

    std::vector<short> data(inNumberFrames);
    memcpy(data.data(), bufferList.mBuffers[0].mData, bufferList.mBuffers[0].mDataByteSize);
    instance->buffer_.write(data);

    free(bufferList.mBuffers[0].mData);
    return noErr;
}

void AudioCaptureMac::CaptureThreadFunction()
{
    while (!quit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void AudioCaptureMac::DummyAudioThreadFunction()
{
    while (!quit)
    {
        std::vector<short> dummyData(4410, 0); // 100ms worth of silence
        buffer_.write(dummyData);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
