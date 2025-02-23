#include <iostream>
#include <numbers>
#include <ranges>
#include <span>
#include <format>
#include <expected>
#include <source_location>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>
#include <portaudio.h>
#include <fftw3.h>
#include <ncurses.h>
#include <cmath>
#include <pstl/glue_numeric_defs.h>

using namespace std::literals;

// Error handling with std::expected
struct TunerError {
    std::string message;
    std::source_location location;
    
    TunerError(std::string msg, const std::source_location& loc = std::source_location::current())
        : message(std::move(msg)), location(loc) {}
};

template<typename T>
using Result = std::expected<T, TunerError>;

// Modern logging utility
class Logger {
public:
    template<typename... Args>
    static void log(std::format_string<Args...> fmt, Args&&... args) {
        auto timestamp = std::chrono::system_clock::now();
        std::cout << std::format("[{}] {}\n", 
            std::chrono::current_zone()->format("%T", timestamp),
            std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        auto loc = std::source_location::current();
        log("Error at {}:{} - {}", 
            loc.file_name(), 
            loc.line(), 
            std::format(fmt, std::forward<Args>(args)...));
    }
};

// Modern audio buffer using std::span
class AudioBuffer {
    std::vector<double> data;
public:
    explicit AudioBuffer(size_t size) : data(size) {}
    
    std::span<double> get_span() { return std::span(data); }
    std::span<const double> get_span() const { return std::span(data); }
    
    void from_float_buffer(std::span<const float> input) {
        std::ranges::transform(input, data.begin(), 
            [](float f) { return static_cast<double>(f); });
    }
};

// Modern tuning configuration with modules (requires C++23 modules support)
struct TuningConfig {
    struct Note {
        std::string name;
        double frequency;
        
        constexpr auto operator<=>(const Note&) const = default;
    };
    
    static constexpr std::array tuning_presets = {
        std::pair{"Standard"sv, std::array{329.63, 246.94, 196.00, 146.83, 110.00, 82.41}},
        std::pair{"Drop D"sv, std::array{329.63, 246.94, 196.00, 146.83, 110.00, 73.42}},
        std::pair{"Open G"sv, std::array{392.00, 293.66, 196.00, 146.83, 98.00, 98.00}},
        std::pair{"DADGAD"sv, std::array{293.66, 220.00, 196.00, 146.83, 110.00, 73.42}}
    };
    
    static constexpr std::array<std::string_view, 12> note_names = {
        "C"sv, "C#"sv, "D"sv, "D#"sv, "E"sv, "F"sv, 
        "F#"sv, "G"sv, "G#"sv, "A"sv, "A#"sv, "B"sv
    };
};

// Modern FFT analyzer using RAII and modern memory management
class FFTAnalyzer {
    static constexpr size_t window_size = 4096;
    std::unique_ptr<double[]> window;
    std::unique_ptr<fftw_complex[], void(*)(void*)> output;
    fftw_plan plan;

public:
    FFTAnalyzer() : 
        window(std::make_unique<double[]>(window_size)),
        output((fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size), fftw_free) {
        
        // Initialize Hanning window
        for (size_t i = 0; i < window_size; ++i) {
            window[i] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / (window_size - 1)));
        }
        
        plan = fftw_plan_dft_r2c_1d(window_size, window.get(), output.get(), FFTW_ESTIMATE);
    }
    
    ~FFTAnalyzer() {
        fftw_destroy_plan(plan);
    }
    
    Result<double> analyze(std::span<const double> audio_data) {
        if (audio_data.size() != window_size) {
            return std::unexpected(TunerError("Invalid audio buffer size"));
        }
        
        // Apply window function
        for (size_t i = 0; i < window_size; ++i) {
            window[i] = audio_data[i] * window[i];
        }
        
        fftw_execute(plan);
        
        // Find dominant frequency using quadratic interpolation
        double max_magnitude = 0.0;
        size_t max_bin = 0;
        
        for (size_t i = 1; i < window_size/2 - 1; ++i) {
            double magnitude = std::hypot(output[i][0], output[i][1]);
            if (magnitude > max_magnitude) {
                max_magnitude = magnitude;
                max_bin = i;
            }
        }
        
        // Quadratic interpolation
        double alpha = std::log(std::abs(output[max_bin-1][0]));
        double beta = std::log(std::abs(output[max_bin][0]));
        double gamma = std::log(std::abs(output[max_bin+1][0]));
        double peak_bin = max_bin + 0.5 * (alpha - gamma) / (alpha - 2*beta + gamma);
        
        return peak_bin * 44100.0 / window_size; // Sample rate hardcoded for simplicity
    }
};

// Modern display using RAII
class TunerDisplay {
    struct WindowDeleter {
        void operator()(WINDOW* w) const { delwin(w); }
    };
    
    std::unique_ptr<WINDOW, WindowDeleter> main_win;
    std::unique_ptr<WINDOW, WindowDeleter> meter_win;
    
public:
    TunerDisplay() {
        initscr();
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        
        main_win.reset(newwin(20, 80, 0, 0));
        meter_win.reset(newwin(3, 60, 15, 10));
        
        nodelay(main_win.get(), TRUE);
        keypad(main_win.get(), TRUE);
        noecho();
        curs_set(0);
    }
    
    ~TunerDisplay() {
        endwin();
    }
    
    void update(double frequency, std::string_view note, double target_freq, double cents_off) {
        wclear(main_win.get());
        wclear(meter_win.get());
        
        box(main_win.get(), 0, 0);
        box(meter_win.get(), 0, 0);
        
        mvwprintw(main_win.get(), 1, 2, std::format("Frequency: {:.2f} Hz", frequency).c_str());
        mvwprintw(main_win.get(), 2, 2, std::format("Note: {}", note).c_str());
        mvwprintw(main_win.get(), 3, 2, std::format("Target: {:.2f} Hz", target_freq).c_str());
        mvwprintw(main_win.get(), 4, 2, std::format("Cents off: {:.2f}", cents_off).c_str());
        
        // Draw meter
        int meter_pos = 30 + static_cast<int>(cents_off / 2);
        meter_pos = std::clamp(meter_pos, 0, 59);
        
        for (int i = 0; i < 58; ++i) {
            if (i == meter_pos) {
                wattron(meter_win.get(), COLOR_PAIR(std::abs(cents_off) < 5 ? 1 : 3));
                mvwaddch(meter_win.get(), 1, i + 1, '|');
                wattroff(meter_win.get(), COLOR_PAIR(std::abs(cents_off) < 5 ? 1 : 3));
            } else {
                mvwaddch(meter_win.get(), 1, i + 1, '-');
            }
        }
        
        wrefresh(main_win.get());
        wrefresh(meter_win.get());
    }
};

// Main tuner class using modern C++ features
class GuitarTuner {
    static constexpr size_t BUFFER_SIZE = 2048;
    static constexpr double MIN_AMPLITUDE = 0.01;
    
    FFTAnalyzer fft;
    TunerDisplay display;
    std::atomic<bool> running{true};
    
    static int audio_callback(const void* input_buffer, void* output_buffer,
                            unsigned long frames_per_buffer,
                            const PaStreamCallbackTimeInfo* time_info,
                            PaStreamCallbackFlags status_flags,
                            void* user_data) {
        auto* tuner = static_cast<GuitarTuner*>(user_data);
        return tuner->process_audio(
            std::span(static_cast<const float*>(input_buffer), frames_per_buffer));
    }
    
    PaError process_audio(std::span<const float> input) {
        AudioBuffer buffer(BUFFER_SIZE);
        buffer.from_float_buffer(input);
        
        // Calculate RMS
        auto rms = std::transform_reduce(input.begin(), input.end(), 0.0, std::plus{},
            [](float x) { return x * x; });
        rms = std::sqrt(rms / input.size());
        
        if (rms > MIN_AMPLITUDE) {
            if (auto freq = fft.analyze(buffer.get_span())) {
                auto [note, target] = find_closest_note(*freq);
                double cents = 1200 * std::log2(*freq / target);
                display.update(*freq, note, target, cents);
            }
        }
        
        return paContinue;
    }
    
    std::pair<std::string, double> find_closest_note(double frequency) const {
        // Implementation using C++23 ranges and algorithms
        auto note_freq = [](int note, int octave) {
            return 440.0 * std::pow(2.0, (octave * 12 + note - 57) / 12.0);
        };
        
        auto notes = std::views::cartesian_product(
            std::views::iota(0, 8),  // octaves
            std::views::iota(0, 12)  // notes
        ) | std::views::transform([&](auto tuple) {
            auto [octave, note] = tuple;
            return std::pair{
                std::format("{}{}", TuningConfig::note_names[note], octave),
                note_freq(note, octave)
            };
        });
        
        auto closest = std::ranges::min_element(notes,
            [frequency](const auto& a, const auto& b) {
                return std::abs(a.second - frequency) < std::abs(b.second - frequency);
            });
            
        return *closest;
    }
    
public:
    Result<void> run() {
        PaStream* stream;
        
        if (auto error = Pa_Initialize(); error != paNoError) {
            return std::unexpected(TunerError(Pa_GetErrorText(error)));
        }
        
        auto cleanup = std::scope_guard([&] {
            Pa_Terminate();
        });
        
        if (auto error = Pa_OpenDefaultStream(&stream,
                1, 0, paFloat32, 44100, BUFFER_SIZE,
                audio_callback, this); error != paNoError) {
            return std::unexpected(TunerError(Pa_GetErrorText(error)));
        }
        
        auto stream_cleanup = std::scope_guard([&] {
            Pa_CloseStream(stream);
        });
        
        if (auto error = Pa_StartStream(stream); error != paNoError) {
            return std::unexpected(TunerError(Pa_GetErrorText(error)));
        }
        
        while (running) {
            if (int ch = getch(); ch == 'q' || ch == 'Q') {
                running = false;
            }
            std::this_thread::sleep_for(50ms);
        }
        
        return {};
    }
};

int main() {
    try {
        GuitarTuner tuner;
        if (auto result = tuner.run(); !result) {
            Logger::error("Tuner error: {}", result.error().message);
            return 1;
        }
    } catch (const std::exception& e) {
        Logger::error("Unexpected error: {}", e.what());
        return 1;
    }
    return 0;
}