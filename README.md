# Guitar Tuner Application

This project is a guitar tuner application built for Linux and WSL. It uses the PortAudio library for real-time audio input and the FFTW library for frequency analysis. The application detects the frequency of the played guitar string and matches it to the closest musical note.

## Features

- Real-time audio input processing using PortAudio.
- Frequency analysis using FFTW.
- Detection of the closest musical note to the detected frequency.
- User-friendly console output displaying the detected frequency and closest note.

## Prerequisites

Before you begin, ensure you have met the following requirements:

- **Operating System:** Linux or WSL.
- **Dependencies:**
  - [PortAudio](http://www.portaudio.com/)
  - [FFTW](http://www.fftw.org/)

## Installing Dependencies

To install the necessary libraries, run the following commands:

```bash
# Install PortAudio
sudo apt-get install portaudio19-dev

# Install FFTW
sudo apt-get install libfftw3-dev
```

## Usage

1. Clone this repository to your local machine.

```bash
git clone https://github.com/aashishbishow/tuner.git
cd tuner
```

2. Compile the application using `g++`.

```bash
g++ -o guitar_tuner tuner.cpp -lportaudio -lfftw3
```

3. Run the application.

```bash
./guitar_tuner
```

4. Play a note on your guitar. The application will display the detected frequency and the closest musical note.

5. Press `Enter` to quit the application.

## Contribution

Contributions are always welcome! Please adhere to the following guidelines:

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-foo`).
3. Make your changes.
4. Commit your changes (`git commit -m 'Add some foo'`).
5. Push to the branch (`git push origin feature-foo`).
6. Open a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.