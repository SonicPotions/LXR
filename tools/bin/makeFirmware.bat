del "..\..\firmware image\old_FIRMWARE.BIN" 2>nul
ren "..\..\firmware image\FIRMWARE.BIN" old_FIRMWARE.BIN 2>nul

call avr-objcopy -O binary -R .eeprom -R .fuse -R .lock -R .signature  "..\..\front\AVR\Release\AVR.elf" "DrumSynthAvr.bin"

firmwareimagebuilder ..\..\mainboard\firmware\DrumSynth_FPU\Debug\DrumSynthCortex.bin DrumSynthAvr.bin "..\..\firmware image\FIRMWARE.BIN"
