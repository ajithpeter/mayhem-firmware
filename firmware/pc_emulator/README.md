# PortaPack Mayhem PC Emulator

A PC-based emulator for PortaPack Mayhem firmware development and testing.

## Prerequisites

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config \
    libsdl2-dev libportaudio-dev libsoapysdr-dev
```

### Fedora
```bash
sudo dnf install gcc-c++ cmake pkgconfig \
    SDL2-devel portaudio-devel SoapySDR-devel
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake pkgconf \
    sdl2 portaudio soapysdr
```

### macOS (Homebrew)
```bash
brew install cmake sdl2 portaudio soapysdr
```

## Building

### Quick Build (Mock RF - No SDR Hardware)
```bash
cd firmware/pc_emulator
mkdir build && cd build
cmake .. -DUSE_MOCK_RF=ON
make -j$(nproc)
```

### Build with SDR Support
```bash
cd firmware/pc_emulator
mkdir build && cd build
cmake .. -DUSE_SOAPYSDR=ON
make -j$(nproc)
```

### Build with Tests
```bash
cd firmware/pc_emulator
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DUSE_MOCK_RF=ON
make -j$(nproc)
make pc_emulator_tests
```

### Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_MOCK_RF=ON
make -j$(nproc)
```

## Running

### Basic Run
```bash
./portapack_emulator
```

### With Custom SD Card Path
```bash
./portapack_emulator --sdcard /path/to/sdcard
```

### Run Tests
```bash
./pc_emulator_tests                    # Run all tests
./pc_emulator_tests --list-test-cases  # List available tests
./pc_emulator_tests -tc="Storage*"     # Run specific test suite
```

## Keyboard Controls

| Key | PortaPack Button |
|-----|------------------|
| Arrow Keys | D-pad navigation |
| Enter / Space | Select |
| Escape / Backspace | Back |
| D | DFU mode |
| Mouse Wheel | Encoder rotation |
| Mouse Click | Touch input |

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `USE_SOAPYSDR` | ON | Enable SoapySDR for real SDR hardware |
| `USE_MOCK_RF` | OFF | Use mock RF backend (no hardware needed) |
| `BUILD_TESTS` | OFF | Build unit tests |
| `CMAKE_BUILD_TYPE` | Release | Debug or Release build |

## Directory Structure

```
pc_emulator/
├── main_pc.cpp          # Entry point
├── CMakeLists.txt       # Build configuration
├── tests/               # Unit tests
└── build/               # Build output (created by cmake)

../hal/
├── interface/           # Abstract HAL interfaces
└── pc/                  # PC implementations (SDL, PortAudio, etc.)

../shim/                 # API compatibility layer
```

## Troubleshooting

### SDL2 not found
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev

# Verify
pkg-config --modversion sdl2
```

### PortAudio not found
```bash
# Ubuntu/Debian
sudo apt install libportaudio-dev

# Verify
pkg-config --modversion portaudio-2.0
```

### No display window appears
- Ensure you have a display server running (X11 or Wayland)
- Check SDL2 is using the correct video driver: `export SDL_VIDEODRIVER=x11`

### Audio not working
- Check PortAudio devices: `pacmd list-sinks` (PulseAudio) or `aplay -l` (ALSA)
- The emulator will run without audio if PortAudio initialization fails

## License

GNU General Public License v2 or later. See LICENSE file for details.
