#include <iostream>
#include <vector>
#include <cmath>
#include <portaudio.h>
// Define the target frequencies for each guitar string (standard tuning)
std::vector<double> string_tunings = { 329.63, 246.94, 196.00, 146.83, 110.00, 82.41 };

// Function to calculate the closest note to the given frequency
std::string closest_note_frequency(double frequency)
{
    std::vector<std::pair<std::string, double>> notes = {
        { "E", 329.63 }, { "F", 349.23 }, { "F#", 369.99 }, { "G", 392.00 }, { "G#", 415.30 }, { "A", 440.00 },
        { "A#", 466.16 }, { "B", 493.88 }, { "C", 523.25 }, { "C#", 554.37 }, { "D", 587.33 }, { "D#", 622.25 }
    };

    std::string closest_note;
    double min_difference = std::numeric_limits<double>::max();

    for (const auto& note : notes) {
        double difference = std::abs(note.second - frequency);
        if (difference < min_difference) {
            min_difference = difference;
            closest_note = note.first;
        }
    }

    return closest_note;
}

// Callback function for audio input processing
int process_audio_input(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags, void* userData)
{
    double* audio_data = static_cast<double*>(const_cast<void*>(inputBuffer));
    unsigned int num_samples = framesPerBuffer;

    // Perform pitch analysis
    double nyquist = 0.5 * static_cast<double>(Pa_GetSampleRate(static_cast<PaStream*>(userData)));
    double* fft_data = new double[num_samples];
    for (unsigned int i = 0; i < num_samples; ++i) {
        fft_data[i] = audio_data[i];
    }

    // Compute FFT
    // ...

    // Calculate dominant frequency
    double frequency = 0.0;
    // ...

    std::string closest_note = closest_note_frequency(frequency);

    // Print the results
    std::cout << "Detected Frequency: " << frequency << " Hz" << std::endl;
    std::cout << "Closest Note: " << closest_note << std::endl;
    // ...

    delete[] fft_data;

    return paContinue;
}

int main()
{
    PaStream* stream;
    PaError error;

    // Initialize PortAudio
    error = Pa_Initialize();
    if (error != paNoError) {
        std::cerr << "PortAudio initialization error: " << Pa_GetErrorText(error) << std::endl;
        return 1;
    }

    // Open the microphone stream
    error = Pa_OpenDefaultStream(&stream, 1, 0, paFloat64, 44100, paFramesPerBufferUnspecified,
        process_audio_input, nullptr);
    if (error != paNoError) {
        std::cerr << "PortAudio stream open error: " << Pa_GetErrorText(error) << std::endl;
        return 1;
    }

    // Start the stream
    error = Pa_StartStream(stream);
    if (error != paNoError) {
        std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(error) << std::endl;
        return 1;
    }

    std::cout << "Guitar Tuner App" << std::endl;
    std::cout << "Press Enter to quit..." << std::endl;

    // Wait for Enter key to quit
    std::cin.ignore();

    // Stop and close the stream
    error = Pa_StopStream(stream);
    if (error != paNoError) {
        std::cerr << "PortAudio stream stop error: " << Pa_GetErrorText(error) << std::endl;
        return 1;
    }

    error = Pa_CloseStream(stream);
    if (error != paNoError) {
        std::cerr << "PortAudio stream close error: " << Pa_GetErrorText(error) << std::endl;
        return 1;
    }

    // Terminate PortAudio
    error = Pa_Terminate();
    if (error != paNoError) {
        std::cerr << "PortAudio termination error: " << Pa_GetErrorText(error) << std::endl;
        return 1;
    }

    return 0;
}
