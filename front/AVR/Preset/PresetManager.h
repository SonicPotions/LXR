/*
 * PresetManager.h
 *
 * Created: 09.05.2012 14:11:49
 *  Author: Julian
 */ 


#ifndef PRESETMANAGER_H_
#define PRESETMANAGER_H_

#include <avr/io.h>

extern char preset_currentName[8];

void preset_init();


/** save the parameters of all 6 voices + LFO settings to a file on the SD card 
if isMorph==1 the sound will be loaded into the morph buffer
*/
void preset_saveDrumset(uint8_t presetNr, uint8_t isMorph);
uint8_t preset_loadDrumset(uint8_t presetNr, uint8_t isMorph);


void preset_saveGlobals();
void preset_loadGlobals();

void preset_saveAll(uint8_t presetNr, uint8_t isAll);
void preset_loadAll(uint8_t presetNr, uint8_t isAll);

char* preset_loadName(uint8_t presetNr, uint8_t what);

/** save a pattern set to the sd card */
void preset_savePattern(uint8_t presetNr);
uint8_t preset_loadPattern(uint8_t presetNr);
/** morph pattern linear to üpattern buffer 2*/
void preset_morph(uint8_t morph);
uint8_t preset_getMorphValue(uint16_t index, uint8_t morph);
void preset_sendDrumsetParameters();

#endif /* PRESETMANAGER_H_ */
