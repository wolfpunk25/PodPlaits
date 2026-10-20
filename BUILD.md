# Build and install

The checked-in firmware is the version tested on a Daisy Pod and a 2.8-inch ESP32-32E 240×320 resistive-touch CYD. Other CYD variants may need different display settings. The current firmware connects Pod and display directly over 3.3V UART; a Mac is needed only for programming and optional diagnostics.

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

## Legacy relay

This section applies to the earlier USB-relay firmware only. Current firmware uses the direct connection documented below. Do not run the relay with direct-link firmware.

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
clang++ -std=c++14 -fsanitize=address,undefined tests/test_command_line.cpp -o build/tests/command_line
build/tests/command_line
```

The storage test runs the actual scene functions against a mock filesystem with write/rename failures. The relay test uses pseudo terminals and does not access the connected boards. Hardware checks are documented in `Hardware-check.txt` and `scenes-hardware-check.json`.

## Tested dependency revisions

- libDaisy: `cc146d5065dd8286078a662e2830bf820c37a612`
- DaisySP: `599511b740f8f3a9b8db72a0642aa45b8a23c3a3`
- DaisySP-LGPL: `c89d380c62620d6fee9105c5035e081c9b7ea843`

The dependency repositories use additional submodules; initialize them recursively. See [THIRD_PARTY.md](THIRD_PARTY.md) for upstream sources and notices.

## Standalone wiring

With all power disconnected, connect the Pod expansion header SDA (Seed D12 / PB9 / UART4 TX) to the display IO35, and Pod SCL (Seed D11 / PB8 / UART4 RX) to display IO27. IO35 is on the screen GPIO connector; IO27 is on its SPI expansion connector. The display’s SD reader is unused by this application. The Pod SD reader remains available for scenes. These are 3.3V logic signals; never connect them to 5V. Ground must be shared.

A shared regulated 5V supply connects to Pod VIN and the screen UART socket’s 5V pin, with its negative output connected to both GND pins. Disconnect the screen’s separate battery. Do not feed 5V into BAT or 3.3V. The original setup measured 4.8V with both boards powered; converter capability still needs checking during extended battery operation. Avoid joining USB power supplies to the battery-fed 5V rail without suitable isolation; switch off the battery supply for programming.

The link runs at 115200 baud, 8 data bits, no parity and one stop bit. It carries the same control/state messages as the earlier USB relay. Screen GPIO27 is TX; GPIO35 is RX. The rear RGB indicator remains available. Each board’s USB connection stays available for updates. No relay process is required for the current firmware.

Pod USB status includes `WIRE <received-lines> <sent-reports> <errors>`. The screen sends a harmless `PING` every second, and its USB diagnostics echo validated UART settings as `DIRECT PSTATE ...`. These diagnostics let both wire directions be verified without user interaction. Pod USB maintenance commands remain available. Separate USB/UART parsers prevent partial messages from being combined.
