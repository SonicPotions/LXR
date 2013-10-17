@echo off
del old_FIRMWARE.BIN 2>nul
ren FIRMWARE.BIN old_FIRMWARE.BIN 2>nul
firmwareimagebuilder ..\mainboard\firmware\DrumSynth_FPU\Release\DrumSynth_FPU.bin ..\front\AVR\Release\DrumSynthAvr.bin FIRMWARE.BIN
