#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <sndfile.h>
#include <portaudio.h>

struct AudioData {
    std::vector<float> buffer;
    int sampleRate;
    int channels;
    long totalFrames;
};

// --- Input Validation Functions (Matching Python Logic) ---

float getValidPitchOffset() {
    float pitchOffset;
    while (true) {
        std::cout << "Enter the pitch offset (1 to 12): ";
        if (std::cin >> pitchOffset && pitchOffset >= 1.0f && pitchOffset <= 12.0f) {
            return pitchOffset;
        }
        std::cout << "Invalid pitch offset. Please enter a value between 1 and 12." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int getValidNumVoices() {
    int numVoices;
    while (true) {
        std::cout << "Enter the number of voices (1 to 12): ";
        if (std::cin >> numVoices && numVoices >= 1 && numVoices <= 12) {
            return numVoices;
        }
        std::cout << "Invalid number of voices. Please enter a value between 1 and 12." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

float getValidMaxDelay() {
    float maxDelayMs;
    while (true) {
        std::cout << "Enter the maximum delay (0 to 1000 milliseconds): ";
        if (std::cin >> maxDelayMs && maxDelayMs >= 0.0f && maxDelayMs <= 1000.0f) {
            return maxDelayMs / 1000.0f; // Convert to seconds
        }
        std::cout << "Invalid delay. Please enter a value between 0 and 1000 milliseconds." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

// --- Audio Processing Logic ---

std::vector<float> generatePitchArray(int numVoices, float pitchOffset) {
    std::vector<float> pitchArray;
    float start;
    if (numVoices % 2 == 0) {
        start = -((numVoices / 2) - 1) * pitchOffset;
    }
    else {
        start = -((numVoices - 1) / 2.0f) * pitchOffset;
    }

    for (int i = 0; i < numVoices; ++i) {
        pitchArray.push_back(start + i * pitchOffset);
    }
    return pitchArray;
}

void processChoir(const AudioData& input, AudioData& output, int numVoices, float pitchOffset, float maxDelaySec) {
    auto pitchShifts = generatePitchArray(numVoices, pitchOffset);
    int maxDelaySamples = static_cast<int>(input.sampleRate * maxDelaySec);

    output.channels = 2;
    output.buffer.assign(input.totalFrames * 2, 0.0f);

    std::default_random_engine generator;

    for (int i = 0; i < numVoices; ++i) {
        std::uniform_int_distribution<int> distribution(0, maxDelaySamples);
        int randomDelay = distribution(generator);

        // Match Python's pitch shift calculation logic
        int shift = static_cast<int>(pitchShifts[i] * input.totalFrames / 1200.0f);
        int totalOffset = shift + randomDelay;

        for (long f = 0; f < input.totalFrames; ++f) {
            // Roll logic: (f - totalOffset) with wrap-around
            long sourceFrame = (f - totalOffset) % input.totalFrames;
            if (sourceFrame < 0) sourceFrame += input.totalFrames;

            int outChan = i % 2;

            // Normalize gain per side based on number of voices assigned to that side
            float gainDivisor = std::max(1.0f, std::floor(numVoices / 2.0f));

            int inIdx = (input.channels == 2) ? (sourceFrame * 2 + outChan) : sourceFrame;
            output.buffer[f * 2 + outChan] += input.buffer[inIdx] / gainDivisor;
        }
    }

    // Normalization and Clipping (Matching Python's 0.9 multiplier)
    float maxVal = 0.0f;
    for (float sample : output.buffer) maxVal = std::max(maxVal, std::abs(sample));
    if (maxVal > 0) {
        for (float& sample : output.buffer) {
            sample = (sample / maxVal) * 0.9f;
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
        }
    }
}

int main() {
    // 1. Get User Input
    float pitchOffset = getValidPitchOffset();
    int numVoices = getValidNumVoices();
    float maxDelay = getValidMaxDelay();

    // 2. Load File
    SF_INFO sfInfo;
    SNDFILE* inFile = sf_open("vocal.wav", SFM_READ, &sfInfo);
    if (!inFile) {
        std::cerr << "Error: Could not find 'vocal.wav'!" << std::endl;
        return 1;
    }

    AudioData input;
    input.sampleRate = sfInfo.samplerate;
    input.channels = sfInfo.channels;
    input.totalFrames = sfInfo.frames;
    input.buffer.resize(input.totalFrames * input.channels);
    sf_readf_float(inFile, input.buffer.data(), input.totalFrames);
    sf_close(inFile);

    // 3. Process
    AudioData output;
    output.sampleRate = input.sampleRate;
    output.totalFrames = input.totalFrames;
    processChoir(input, output, numVoices, pitchOffset, maxDelay);

    // 4. Playback
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, output.sampleRate, 256, nullptr, nullptr);

    Pa_StartStream(stream);
    std::cout << "Playing Choir Effect... Press Enter to stop." << std::endl;
    Pa_WriteStream(stream, output.buffer.data(), output.totalFrames);

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear buffer
    std::cin.get();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    return 0;
}