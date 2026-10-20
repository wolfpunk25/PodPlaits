# Build and install

The checked-in firmware is the version tested on a Daisy Pod and a 2.8-inch ESP32-32E 240×320 resistive-touch CYD. Other CYD variants may need different display settings. A Mac running the Python relay connects the two USB devices; this is not yet a direct standalone connection between the boards.

## Pod

Use GNU Arm Embedded GCC, Make, libDaisy and DaisySP with its LGPL components. The tested compiler and dependency revisions are recorded below. Put the dependencies under `deps/` or override `LIBDAISY_DIR` and `DAISYSP_DIR` when invoking Make. Initialize their submodules and build both libraries, including DaisySP-LGPL, before building this application.

```sh
make -C pod GCC_PATH=/path/to/arm-toolchain/bin
```

`pod/build/plaits_pod.bin` is the application. It requires the matching Daisy bootloader and runs from QSPI, not internal application flash.

For a new Seed, enter the STM32 ROM programmer by holding BOOT while pressing/releasing RESET, then release BOOT. Install `daisy-bootloader.bin` at **0x08000000** using dfu-util. This replaces any existing internal application. Reset the Seed and enter the Daisy programmer by briefly pressing RESET, then separately pressing BOOT within two seconds. Upload `pod-firmware.bin` at **0x90040000** through the Daisy programmer. Never install the application at the bootloader address.

An already running PodPlaits accepts the private USB command `UPDATE` to enter the Daisy programmer indefinitely. Stop the relay before using maintenance tools. The relay blocks maintenance commands from the display.

## Touchscreen

The tested setup uses Arduino ESP32 core **3.3.12**, TFT_eSPI and XPT2046_Touchscreen. Preferences and SPI are supplied by the ESP32 core.

Configure TFT_eSPI by copying `TouchPanel/DisplaySetup.h` to that library’s `User_Setup.h` before compiling. The setup must apply to the library compilation as well as the sketch. It selects the alternative ILI9341 driver, HSPI, the CYD LCD pins, and 55 MHz SPI. Touch uses a separate SPI bus configured in the sketch.

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --build-path build/display TouchPanel
```

Flash the application at **0x10000** to retain existing touch calibration. `display-app.bin` is the supplied application image. `display-firmware.bin` is a full merged image for address **0**, which resets calibration. After a first install, touch and hold each calibration cross, releasing between crosses. Hold the screen during startup to recalibrate. The tested panel uses `invertDisplay(true)` for the black background.

## Relay

Python 3 with standard libraries only, on macOS:

```sh
python3 bridge.py --pod /dev/cu.usbmodemYOURPOD --display /dev/cu.usbserial-YOURDISPLAY
```

`Start.command` uses automatic port discovery. The default Pod pattern matches the original Seed’s USB identifier; use explicit arguments for another Seed. Leave the relay running while using the screen.

## Scene and relay tests

From the repository root, using Clang and Python 3:

```sh
mkdir -p build/tests
clang++ -std=c++14 -fsanitize=address,undefined tests/test_scene_codec.cpp -o build/tests/codec
build/tests/codec
clang++ -std=c++14 -fsanitize=address,undefined -Itests/mock-scene tests/test_scene_storage.cpp -o build/tests/storage
build/tests/storage
python3 tests/test_scene_bridge.py
```

The storage test runs the actual scene functions against a mock filesystem with write/rename failures. The relay test uses pseudo terminals and does not access the connected boards. Hardware checks are documented in `Hardware-check.txt` and `scenes-hardware-check.json`.

## Tested dependency revisions

- libDaisy: `cc146d5065dd8286078a662e2830bf820c37a612`
- DaisySP: `599511b740f8f3a9b8db72a0642aa45b8a23c3a3`
- DaisySP-LGPL: `c89d380c62620d6fee9105c5035e081c9b7ea843`

The dependency repositories use additional submodules; initialize them recursively. See [THIRD_PARTY.md](THIRD_PARTY.md) for upstream sources and notices.
