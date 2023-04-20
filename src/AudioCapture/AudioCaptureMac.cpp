#include "AudioCaptureMac.h"
#include <iostream>
#include <stdexcept>

AudioCaptureMac::AudioCaptureMac(std::string device_name, bool dummy_audio)
    : AudioCaptureBase(device_name, dummy_audio)
{
    
    if (!dummy_audio)
    {
            if (device_name.empty()) {
        device_name = prompt_device_selection();
    }
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
    instance->buffer_.add_data(data);

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
        buffer_.add_data(dummyData);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


std::vector<AudioDeviceID> AudioCaptureMac::get_audio_input_devices() {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, nullptr, &dataSize);
    if (status != noErr) {
        std::cerr << "Error getting audio input device list size." << std::endl;
        return {};
    }

    UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> devices(deviceCount);

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, nullptr, &dataSize, devices.data());
    if (status != noErr) {
        std::cerr << "Error getting audio input device list." << std::endl;
        return {};
    }

    std::vector<AudioDeviceID> inputDevices;

    for (const auto& device : devices) {
        UInt32 isInputDevice = 0;
        UInt32 dataSize = sizeof(isInputDevice);

        propertyAddress.mSelector = kAudioDevicePropertyStreams;
        propertyAddress.mScope = kAudioDevicePropertyScopeInput;

        status = AudioObjectGetPropertyDataSize(device, &propertyAddress, 0, nullptr, &dataSize);
        if (status == noErr) {
            isInputDevice = dataSize > 0;
        }

        if (isInputDevice) {
            inputDevices.push_back(device);
        }
    }

    return inputDevices;
}

std::string AudioCaptureMac::get_device_name(AudioDeviceID device) {
    UInt32 dataSize = sizeof(CFStringRef);
    CFStringRef deviceName = nullptr;

    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    OSStatus status = AudioObjectGetPropertyData(device, &propertyAddress, 0, nullptr, &dataSize, &deviceName);
    if (status != noErr) {
        std::cerr << "Error getting audio device name." << std::endl;
        return {};
    }

    char name[256];
    CFStringGetCString(deviceName, name, sizeof(name), kCFStringEncodingUTF8);
    CFRelease(deviceName);

    return std::string(name);
}


std::string AudioCaptureMac::prompt_device_selection() {
    auto inputDevices = get_audio_input_devices();

    if (inputDevices.empty()) {
        std::cerr << "No audio input devices found." << std::endl;
        return "";
    }

    std::cout << "Select an audio input device:" << std::endl;
    for (size_t i = 0; i < inputDevices.size(); ++i) {
        std::cout << (i + 1) << ". " << get_device_name(inputDevices[i]) << std::endl;
    }

    size_t selection = 0;
    std::cin >> selection;

    if (selection < 1 || selection > inputDevices.size()) {
        std::cerr << "Invalid selection. No audio device selected." << std::endl;
        return "";
    }

    std::string selectedDeviceName = get_device_name(inputDevices[selection - 1]);
    std::cout << "Selected audio input device: " << selectedDeviceName << std::endl;

    return selectedDeviceName;
}

