# Third-party code and firmware

PodPlaits adapts the Plaits DSP port from [CyberDuck79/Mutables_Daisies](https://github.com/CyberDuck79/Mutables_Daisies), revision `c69c57f92296b9787e9565414bf4d86440ef67ed`, using its eurorack submodule at `25851b2eb8251fcc80f984296bcfc9066b768c3f`.

- `vendor/eurorack/plaits` and `vendor/eurorack/stmlib` retain their original file notices, including Émilie Gillet’s MIT notices and third-party notices. `vendor/eurorack/stmlib/LICENSE` contains the stmlib licence.
- `third_party/ReverbSc` contains the reverb source used by the application and the DaisySP-LGPL licence text. ReverbSc is distributed under LGPL-2.1; original notices remain in the source.
- The Pod firmware links [libDaisy](https://github.com/electro-smith/libDaisy), [DaisySP](https://github.com/electro-smith/DaisySP), and [DaisySP-LGPL](https://github.com/electro-smith/DaisySP-LGPL). Tested revisions are recorded in BUILD.md. Their upstream licences and component notices apply.
- `daisy-bootloader.bin` is the Electrosmith Daisy v6.4 bootloader. Its MIT licence is included as `Bootloader-LICENSE`. Source: [DaisyBootloader](https://github.com/electro-smith/DaisyBootloader).
- `TouchPanel/DisplaySetup.h` derives from TFT_eSPI’s configuration template. The display firmware uses [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI), [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen), and the [Arduino ESP32 core](https://github.com/espressif/arduino-esp32); these retain their upstream licences.

No single project-wide licence is assigned here. Third-party code remains covered by its existing licences; original PodPlaits additions have no additional licence grant at present.
