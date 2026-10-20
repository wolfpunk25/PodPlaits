TARGET = plaits_pod
CPP_SOURCES = main.cpp user_data_provider.cpp
USE_FATFS = 1
APP_TYPE = BOOT_QSPI
LIBDAISY_DIR ?= ../deps/libDaisy
DAISYSP_DIR ?= ../deps/DaisySP
USE_DAISYSP_LGPL = 1
EURORACK_DIR ?= ../vendor/eurorack
PLAITS_DIR = $(EURORACK_DIR)/plaits/dsp
STMLIB_DIR = $(EURORACK_DIR)/stmlib
CPP_STANDARD = -std=gnu++17
OPT = -O2
PLAITS_CC_SOURCES = \
	$(PLAITS_DIR)/voice.cc \
	$(PLAITS_DIR)/chords/chord_bank.cc \
	$(PLAITS_DIR)/fm/algorithms.cc \
	$(PLAITS_DIR)/fm/dx_units.cc \
	$(PLAITS_DIR)/engine/additive_engine.cc \
	$(PLAITS_DIR)/engine/bass_drum_engine.cc \
	$(PLAITS_DIR)/engine/chord_engine.cc \
	$(PLAITS_DIR)/engine/fm_engine.cc \
	$(PLAITS_DIR)/engine/grain_engine.cc \
	$(PLAITS_DIR)/engine/hi_hat_engine.cc \
	$(PLAITS_DIR)/engine/modal_engine.cc \
	$(PLAITS_DIR)/engine/noise_engine.cc \
	$(PLAITS_DIR)/engine/particle_engine.cc \
	$(PLAITS_DIR)/engine/snare_drum_engine.cc \
	$(PLAITS_DIR)/engine/speech_engine.cc \
	$(PLAITS_DIR)/engine/string_engine.cc \
	$(PLAITS_DIR)/engine/swarm_engine.cc \
	$(PLAITS_DIR)/engine/virtual_analog_engine.cc \
	$(PLAITS_DIR)/engine/waveshaping_engine.cc \
	$(PLAITS_DIR)/engine/wavetable_engine.cc \
	$(PLAITS_DIR)/engine2/chiptune_engine.cc \
	$(PLAITS_DIR)/engine2/phase_distortion_engine.cc \
	$(PLAITS_DIR)/engine2/six_op_engine.cc \
	$(PLAITS_DIR)/engine2/string_machine_engine.cc \
	$(PLAITS_DIR)/engine2/virtual_analog_vcf_engine.cc \
	$(PLAITS_DIR)/engine2/wave_terrain_engine.cc \
	$(PLAITS_DIR)/physical_modelling/modal_voice.cc \
	$(PLAITS_DIR)/physical_modelling/resonator.cc \
	$(PLAITS_DIR)/physical_modelling/string.cc \
	$(PLAITS_DIR)/physical_modelling/string_voice.cc \
	$(PLAITS_DIR)/speech/lpc_speech_synth.cc \
	$(PLAITS_DIR)/speech/lpc_speech_synth_controller.cc \
	$(PLAITS_DIR)/speech/lpc_speech_synth_phonemes.cc \
	$(PLAITS_DIR)/speech/lpc_speech_synth_words.cc \
	$(PLAITS_DIR)/speech/naive_speech_synth.cc \
	$(PLAITS_DIR)/speech/sam_speech_synth.cc \
	$(EURORACK_DIR)/plaits/resources.cc \
	$(STMLIB_DIR)/utils/random.cc \
	$(STMLIB_DIR)/dsp/units.cc

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
C_INCLUDES += -I. -I$(EURORACK_DIR)
C_DEFS += -DPLAITS_DAISY
# Add .cc source handling
CC_OBJ = $(addprefix $(BUILD_DIR)/,$(notdir $(PLAITS_CC_SOURCES:.cc=.o)))
vpath %.cc $(sort $(dir $(PLAITS_CC_SOURCES)))

# Compile .cc files with C++ compiler
$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -c $(CPPFLAGS) $(CPP_STANDARD) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cc=.lst)) $< -o $@

# Override link step to include .cc objects
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(CC_OBJ) $(LIBDAISY_DIR)/build/libdaisy.a
	$(CXX) $(CC_OBJ) $(OBJECTS) $(LDFLAGS) -o $@
