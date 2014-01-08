del "..\..\firmware image\old_FIRMWARE.BIN" 2>nul
ren "..\..\firmware image\FIRMWARE.BIN" old_FIRMWARE.BIN 2>nul
firmwareimagebuilder ..\..\mainboard\firmware\DrumSynth_FPU\Release\DrumSynth_FPU.bin ..\..\front\AVR\Release\DrumSynthAvr.bin "..\..\firmware image\FIRMWARE.BIN"
pause
