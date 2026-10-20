# PodPlaits — Plaits for Daisy Pod + yellow touchscreen

A Pod adaptation of the **Plaits DSP core from CyberDuck79/Mutables_Daisies**, using the existing ESP32 touchscreen and Mac USB relay. This instrument pairs a Daisy Pod with an ESP32 yellow touchscreen, using a Mac as the USB relay.

The upstream application targets Daisy Patch, with its own screen, CV inputs, four audio inputs/outputs, presets and custom data loading. This adaptation replaces that hardware/UI layer. It does not claim to reproduce all Patch application features.

## Try it

Connect headphones or powered speakers to the Pod’s audio output, starting with low volume. Connect both boards to the Mac and open **Start.command**. Wait for POD CONNECTED, then tap START SOUND.

- **Sound:** use < / > to choose among 24 engines; adjust Harmonics, Timbre and Morph.
- **Play:** choose AUTO NOTES or DRONE. Auto Notes generates a gentle random walk in a minor pentatonic scale. Adjust root note, decay and note rate. Drone holds the root note; percussive engines may respond differently in drone mode.
- **Space:** reverb, slow drift of the sound parameters, and output volume.
- **Moods:** tap a named starting sound to set the engine, mode and all nine controls together. Loading leaves playback paused. Tweak it on the other tabs, then save your version in Scenes.
- **Scenes:** 32 named slots saved on the Pod’s SD card. Use the arrows to select a slot, EDIT NAME to name it, then SAVE. Existing scenes ask for REPLACE confirmation. LOAD recalls the scene with playback paused; tap START SOUND when ready.
- **Start/Pause:** fades the complete output. Starts paused after power-on.

Pod knob 1 controls Timbre, knob 2 Morph (with pickup after touchscreen changes). The encoder selects the engine. Button 1 starts/pauses; button 2 switches Auto Notes/Drone. The touchscreen follows these controls.

The touchscreen uses cyan, amber, purple and green tab accents and individually coloured sliders on a black background. Its panel inversion setting and saved calibration are retained. The Pod’s programmable RGB/status LEDs are kept off during normal operation to eliminate visible software-PWM flicker. Hold the touchscreen during startup to recalibrate.

Plaits OUT and AUX are routed to the Pod’s left and right outputs, then through stereo reverb. AUX is an alternate synthesis output, so the two channels can differ significantly by engine.

## Included and excluded

All 24 factory engines are included, including the three built-in 6-operator FM banks. This version runs one Plaits Voice (some engines have internal multi-voice behaviour), adds autonomous notes/drift and reverb, and supplies USB touchscreen controls.

It does not currently load the WAV files on your SD card, custom FM/wavetable/wave-terrain data, Patch presets, audio-rate input modulation, external CV, or the Patch port’s extra polyphonic voices. It writes only its scene files under `/plaits/scenes/`; WAV samples in the root are left untouched. Clouds, Rings and the other planned modules are not present in the referenced repository’s finished ports.

## Firmware and bootloader

Plaits is larger than the Seed’s 128 KB internal program flash. The supplied matching Daisy v6.4 bootloader occupies internal flash, and `pod-firmware.bin` runs from QSPI flash at **0x90040000**. `daisy-bootloader.bin` is installed at **0x08000000**. Do not flash the Plaits application at the old ambient firmware’s internal address.

`display-app.bin` is the ESP32 app at **0x10000**, retaining calibration. `display-firmware.bin` is a full merged ESP32 image at **0**, resetting calibration.

The Pod accepts a private USB `UPDATE` command to enter the Daisy programmer indefinitely, and `BOOT` to enter the STM32 ROM programmer. These maintenance commands are not forwarded by the touchscreen relay. Physical BOOT/RESET remains available.

## Sources and rebuilding

`pod/` contains the adaptation, Makefile and user-data provider stub. `vendor/eurorack/` includes the Plaits and stmlib sources/headers used. `TouchPanel/` contains the ESP32 sketch and TFT setup snapshot. `bridge.py` uses standard Python 3 only.

The Makefile expects built libDaisy and DaisySP/DaisySP-LGPL dependencies in `deps/libDaisy` and `deps/DaisySP`. LIBDAISY_DIR, DAISYSP_DIR and GCC_PATH can be overridden. See [BUILD.md](BUILD.md) for the toolchain and display setup. The Plaits `.cc` source list follows the upstream project’s Makefile, with PLAITS_DAISY enabled.

Upstream: https://github.com/CyberDuck79/Mutables_Daisies . The Plaits/stmlib sources carry their original Émilie Gillet MIT notices. ReverbSc is DaisySP-LGPL (LGPL-2.1); libDaisy/DaisySP components retain their respective licences. This is an adaptation, not an official Mutable Instruments product.

## Validation

The Pod and touchscreen builds compile. A native memory-sanitized build using the actual synthesis and reverb code produced non-silent, finite, bounded audio from every engine while varying parameters, and verified pause fade. USB relay tests cover engine/parameter/mode/play messages, fragmented state messages and rejection of invalid/maintenance commands. Physical hardware checks are recorded below after installation.

Source revisions: Mutables_Daisies c69c57f92296b9787e9565414bf4d86440ef67ed; its eurorack submodule 25851b2eb8251fcc80f984296bcfc9066b768c3f.

Physical hardware verification: all 24 Plaits engines ran on the Pod in a silent benchmark. Every engine stayed within its audio deadline, with a maximum observed processing peak of 56% and zero missed blocks. The temporary installation bootloader is replaced by the normal v6.4 two-second version before delivery. The touchscreen application was hash-verified and its saved calibration retained.

Colour/LED update: both firmware builds compiled and were loaded; the display image was hash-verified. The previously selected engine, mode and all nine parameters were restored and acknowledged over USB, with playback paused. No synthesis/audio-processing code changed in this update.

## Display rear RGB indicator

The display’s rear RGB LED mirrors the sound-engine family colour shown beside the engine name: cyan for engines 8–15 (synth bank), amber for 16–23 (swarm/noise/percussion/resonator bank), and purple for 0–7 (new-engine bank, including 6-op FM and string machine). These are internal zero-based engine indices. The indicator stays steady while paused and turns off when the display loses Pod status.

The CYD common-anode LED uses red GPIO22, green GPIO16 and blue GPIO17. Hardware PWM runs at 5 kHz. The rear LED uses full-intensity, saturated cyan, amber and purple/magenta colours for illumination through a case: each colour’s strongest channels reach 100% duty. Screen colours remain softer. The Pod’s programmable LEDs remain off.

## Scenes

Each scene stores the engine, Harmonics, Timbre, Morph, root note, decay, note rate, reverb, drift, volume, and Auto Notes/Drone mode. Names support 20 characters. EDIT NAME opens the keyboard; CLEAR empties the name, DELETE removes a character, DONE accepts the name, and SAVE writes it to the card. Navigating to another slot discards an unsaved name edit. “Current settings differ” indicates that sound controls have changed since saving.

Loading fades out, restores the settings and clears the reverb tail, leaving playback paused. Scenes store sound settings, not recorded audio, melody history or synthesis voice state. Power-on starts paused with default sound settings; choose a scene and LOAD to recall it. The original hardware was tested using a FIRST SOUND scene in slot 1. Scene files live on the SD card and are not included in this repository.

Scene files are fixed 64-byte, versioned records with CRC32 checksums. Files are named `scene01.scn` through `scene32.scn` inside `/plaits/scenes/`. Saving writes and syncs a temporary file before replacing the scene. Overwrites retain the previous successful file as `.bak`; reads can recover a valid backup or completed temporary file when the main file is unavailable or invalid. These measures reduce interrupted-save risk, but do not make FAT storage immune to power loss. Insert the SD card before powering on; an unavailable card is reported on the Scenes page.

Scenes validation: both firmware builds compiled and were installed, with display hash verification. Sanitized native tests exercised scene encoding, bounds and corruption rejection, and the actual storage logic with injected write/rename failures and backup/temp recovery. Relay integration tests covered scene commands and metadata. On the Pod, FIRST SOUND saved, marked changed after edits, restored all saved controls while paused, overwrote with a backup, and survived a software restart and reload. Saving during a muted audio benchmark produced zero missed blocks. Touchscreen naming and overwrite confirmation remain to be checked by touch on the physical display.

## Mood presets

Six built-in starting points are available on the MOODS tab, even without an SD card:

| Mood | Character | Engine | Mode |
| --- | --- | --- | --- |
| Warm Drift | Slow, rounded chords | Chords | Auto Notes |
| Glass Garden | Gentle ringing notes | Modal resonator | Auto Notes |
| Night Tide | Dark, moving waves | Virtual analog | Drone |
| Floating Choir | Airy, changing vowels | Granular formants | Drone |
| Soft Sparks | Sparse, quiet plucked strings | String resonator | Auto Notes |
| Deep Space | Low, shifting harmonics | Harmonic oscillator | Drone |

Selecting a mood fades out, clears the old reverb tail and sets the engine, mode, root note, pacing, sound controls, reverb, drift and volume. It stays paused until START SOUND. The selected mood has an outline while its controls match exactly; edits remove that outline. These are starting settings rather than promises of identical loudness or a particular emotional response.

To keep a variation, open SCENES, choose a slot and SAVE. The mood’s name is suggested in the currently selected scene slot; choosing another slot adopts that slot’s existing/default name. Loading a mood never writes to the SD card or replaces a saved scene.

Pod knobs wait for movement and pickup before taking over after loading a mood or scene, so a stationary knob cannot immediately nudge a recalled setting.

The preset table in `TouchPanel/Moods.h` is shared by Pod and display builds. Native tests check all preset bounds, paused loading, rejection of invalid indices and absence of SD writes. The actual audio renderer produced finite, bounded, non-silent output for every mood in a 12-second sanitized test, and pause fade passed. Physical checks are recorded in `moods-hardware-check.json`. Subjective sound balance and physical touchscreen operation require listening and touch checks.
