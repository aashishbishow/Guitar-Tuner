#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <portaudio.h>
#include <fftw3.h>

// Define the target frequencies for each guitar string (standard tuning)
std::vector<double> string_tunings = { 329.63, 246.94, 196.00, 146.83, 110.00, 82.41 };

// Function to calculate the closest note to the given frequency
std::string closest_note_frequency(double frequency)
{
    std::vector<std::pair<std::string, double>> notes = {
        { "C0", 16.35 }, { "C#0", 17.32 }, { "D0", 18.35 }, { "D#0", 19.45 }, { "E0", 20.60 }, { "F0", 21.83 }, { "F#0", 23.12 }, { "G0", 24.50 }, { "G#0", 25.96 },
        { "A0", 27.50 }, { "A#0", 29.14 }, { "B0", 30.87 },
        { "C1", 32.70 }, { "C#1", 34.65 }, { "D1", 36.71 }, { "D#1", 38.89 }, { "E1", 41.20 }, { "F1", 43.65 }, { "F#1", 46.25 }, { "G1", 49.00 }, { "G#1", 51.91 },
        { "A1", 55.00 }, { "A#1", 58.27 }, { "B1", 61.74 },
        { "C2", 65.41 }, { "C#2", 69.30 }, { "D2", 73.42 }, { "D#2", 77.78 }, { "E2", 82.41 }, { "F2", 87.31 }, { "F#2", 92.50 }, { "G2", 98.00 }, { "G#2", 103.83 },
        { "A2", 110.00 }, { "A#2", 116.54 }, { "B2", 123.47 },
        { "C3", 130.81 }, { "C#3", 138.59 }, { "D3", 146.83 }, { "D#3", 155.56 }, { "E3", 164.81 }, { "F3", 174.61 }, { "F#3", 185.00 }, { "G3", 196.00 }, { "G#3", 207.65 },
        { "A3", 220.00 }, { "A#3", 233.08 }, { "B3", 246.94 },
        { "C4", 261.63 }, { "C#4", 277.18 }, { "D4", 293.66 }, { "D#4", 311.13 }, { "E4", 329.63 }, { "F4", 349.23 }, { "F#4", 369.99 }, { "G4", 392.00 }, { "G#4", 415.30 },
        { "A4", 440.00 }, { "A#4", 466.16 }, { "B4", 493.88 },
        { "C5", 523.25 }, { "C#5", 554.37 }, { "D5", 587.33 }, { "D#5", 622.25 }, { "E5", 659.25 }, { "F5", 698.46 }, { "F#5", 739.99 }, { "G5", 783.99 }, { "G#5", 830.61 },
        { "A5", 880.00 }, { "A#5", 932.33 }, { "B5", 987.77 },
        { "C6", 1046.50 }, { "C#6", 1108.73 }, { "D6", 1174.66 }, { "D#6", 1244.51 }, { "E6", 1318.51 }, { "F6", 1396.91 }, { "F#6", 1479.98 }, { "G6", 1567.98 }, { "G#6", 1661.22 },
        { "A6", 1760.00 }, { "A#6", 1864.66 }, { "B6", 1975.53 },
        { "C7", 2093.00 }, { "C#7", 2217.46 }, { "D7", 2349.32 }, { "D#7", 2489.02 }, { "E7", 2637.02 }, { "F7", 2793.83 }, { "F#7", 2959.96 }, { "G7", 3135.96 }, { "G#7", 3322.44 }
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

// Function to compute the FFT and return the dominant frequency
double compute_fft(double* audio_data, unsigned int num_samples, double sampleRate)
{
    // Create FFTW plan
    fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * num_samples);
    fftw_plan plan = fftw_plan_dft_r2c_1d(num_samples, audio_data, out, FFTW_ESTIMATE);

    // Execute FFT
    fftw_execute(plan);

    // Find the dominant frequency
    double max_magnitude = 0.0;
    double dominant_frequency = 0.0;
    for (unsigned int i = 0; i < num_samples / 2; ++i) {
        double real = out[i][0];
        double imag = out[i][1];
        double magnitude = sqrt(real * real + imag * imag);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            dominant_frequency = (i * sampleRate) / num_samples;
        }
    }

    // Clean up
    fftw_destroy_plan(plan);
    fftw_free(out);

    return dominant_frequency;
}

// Function to format and print the results
void print_detection_results(double frequency, const std::string& closest_note)
{
    std::cout << std::fixed << std::setprecision(2);  // Fixed-point notation with 2 decimal places
    std::cout << "Detected Frequency: " << frequency << " Hz" << std::endl;
    std::cout << "Closest Note: " << closest_note << std::endl;
}

// Callback function for audio input processing
int process_audio_input(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags, void* userData)
{
    float* float_audio_data = (float*)inputBuffer;
    unsigned int num_samples = framesPerBuffer;

    // Convert float audio data to double
    double* audio_data = new double[num_samples];
    for (unsigned int i = 0; i < num_samples; ++i) {
        audio_data[i] = static_cast<double>(float_audio_data[i]);
    }

    // Perform pitch analysis
    double sampleRate = 44100;
    double frequency = compute_fft(audio_data, num_samples, sampleRate);

    std::string closest_note = closest_note_frequency(frequency);

    // Print the results
    print_detection_results(frequency, closest_note);

    delete[] audio_data;

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
    error = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, 44100, paFramesPerBufferUnspecified,
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
    std::cout << "Listening for guitar notes..." << std::endl;
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
