/*
 * MenuText.h
 *
 * Created: 04.11.2012 08:52:54
 *  Author: Julian
 */ 


#ifndef MENUTEXT_H_
#define MENUTEXT_H_

//-----------------------------------------------------------------
#define MENU_FILTER			1
#define MENU_WAVEFORM		2
#define MENU_AUDIO_OUT		3
#define MENU_ROLL_RATES		4
#define MENU_SYNC_RATES		5
#define MENU_LFO_WAVES		6
#define MENU_RETRIGGER		7
#define MENU_NEXT_PATTERN	8
#define MENU_SEQ_QUANT		9
#define MENU_TRANS			10
#define MENU_MIDI			11
//-----------------------------------------------------------------
const char midiModes[][4] PROGMEM  =
{
	{2},		//number of entries
	{"trg"},
	{"nte"},
};
//-----------------------------------------------------------------
const char quantisationNames[][4] PROGMEM  = 
{
	{5},		//number of entries
	{"Off"},
	{"8"},
	{"16"},
	{"32"},
	{"64"},
};
//-----------------------------------------------------------------
const char transientNames[][4] PROGMEM  = 
{
	{14},		//number of entries
	{"Snp"},
	{"Ofs"},
	{"Clk"},
	{"Ck2"},
	{"Tik"},
	{"Kik"},
	{"Rim"},
	{"Drp"},
	{"Hat"},
	{"Clp"},
	{"Kk2"},
	{"Snr"},
	{"Tom"},
	{"Sp2"},
	
};
//-----------------------------------------------------------------
const char nextPatternNames[][4] PROGMEM  = 
{
	{15},		//number of entries
	{"p1"},
	{"p2"},
	{"p3"},
	{"p4"},
	{"p5"},
	{"p6"},
	{"p7"},
	{"p8"},
	{"r2"},
	{"r3"},
	{"r4"},
	{"r5"},
	{"r6"},
	{"r7"},
	{"r8"},
};
//-----------------------------------------------------------------
const char retriggerNames[][4] PROGMEM  = 
{
	{7},		//number of entries
	{"off"},
	{"v1"},
	{"v2"},
	{"v3"},
	{"v4"},
	{"v5"},
	{"v6"},
};
//-----------------------------------------------------------------
const char lfoWaveNames[][4] PROGMEM  = 
{
	{8},		//number of entries
	{"sin"},
	{"tri"},
	{"sup"},
	{"sdn"},
	{"sqr"},
	{"rnd"},
	{"xup"},
	{"xdn"},
};
//-----------------------------------------------------------------
const char rollRateNames[][4] PROGMEM  = 
{
	{14},		//number of entries
	{"One"},		
	{"1/1"},		
	{"1/2"},		
	{"1/3"},		
	{"1/4"},		
	{"1/6"},		
	{"1/8"},		
	{"12"},		
	{"16"},		
	{"24"},		
	{"32"},		
	{"48"},		
	{"64"},		
	{"128"},
};
//-----------------------------------------------------------------
	const char syncRateNames[][4] PROGMEM  = 
{
	{12},		//number of entries
	{"off"},
	{"4/1"},
	{"2/1"},
	{"1/1"},
	{"1/2"},
	{"1/3"},
	{"1/4"},
	{"1/6"},
	{"1/8"},
	{"12"},
	{"16"},
	{"32"},
};
//-----------------------------------------------------------------
const char waveformNames[][4] PROGMEM  = 
{
	{6},		//number of entries
	{"Sin"}	,
	{"Tri"}	,
	{"Saw"}	,
	{"Rec"}	,
	{"Noi"}	,
	{"Cym"}	,
};
//-----------------------------------------------------------------
const char outputNames[][7] PROGMEM  = 
{
	{6},		//number of entries
	{"St1"}	,	//Stereo Out DAC 1
	{"St2"}	,	//Stereo Out DAC 2
	{"L1"}	,	//Mono Out Left DAC 1
	{"R1"}	,	//Mono Out Right DAC 1
	{"L2"}	,	//Mono Out Left DAC 2
	{"R2"}	,	//Mono Out Right DAC 2
};
//-----------------------------------------------------------------
const char filterTypes[][8] PROGMEM  = 
{
	{7},		//number of entries
	{"LP"}	,	//1 - 001
	{"HP"}	,	//2 - 010
	{"BP"}	,	//3 - 011
	{"UBP"}	,	//4 - 100
	{"Nch"}	,	//5 - 101
	{"Pek"}	,	//6 - 110
	{"Off"}	,	//7 - 111
	//{"All"}	,	//7 - 111
};
//-----------------------------------------------------------------
const char shortNames[][4] PROGMEM  = 
{
	{""},
	{"crs"},
	{"fin"},
	{"atk"},
	{"dec"},
	{"eg2"},
	{"mod"},
	{"amt"},
	{"frq"},
	{"drv"},
	{"vol"},
	{"pan"},
	{"noi"},
	{"rpt"},
	{"mix"},	
	{"res"},
	{"typ"},
	{"f1"},
	{"f2"},
	{"g1"},
	{"g2"},
	{"wav"},
	{"dst"},
	{"snc"},
	{"rtg"},
	{"ofs"},
	{"voi"},
	{"slp"},
	{"d1"},		
	{"d2"},
	{"eqg"},
	{"eqf"},
#if USE_CODEC_EQ
	{"eq "},
	{"lim"},
	{"rel"},
	{"max"},
	{"min"},
#endif
	{"rol"},
	{"mph"},
	{"nte"},
	{"prb"},
	{"stp"},
	{"len"},
	{"bpm"},
	{"ch"},
	{"out"},
	{"srt"},
	{"nxt"},
	{"mod"},
	{"vel"},
	{"fch"},
	{"flw"},
	{"qnt"},
	{"trk"},
	{"val"},
	{"shf"},
	{"ssv"},
	{"x"},
	{"y"},
	{"flx"},
	{"mid"},
		
	
};
//-----------------------------------------------------------------
// These correspond with the catNamesEnum in menu.h
const char catNames[][16] PROGMEM = 
{
	{""},
	//012345678
	{"Oscillatr"},
	{"Veloc EG"},
	{"Mod EG"},
	{"Pitch Mod"},
	{"FM"},
	{"Voice"},
	{"Noise"},
	{"Noise/Osc"},
	{"Filter"},
	{"Mod. Osc"},
	{"LFO"},
	{"Transient"},
	{"EQ"},
#if USE_CODEC_EQ		
	{"Limiter"},
#endif
	{"Pattern"},
	{"Sound"},
	{"Step"},
	{"Euklid"},		
	{"Global"},	
	{"Velocity"},
	{"Parameter"},
	{"Sequencer"},
	{"Generator"},
	{"MIDI"},
};
//-----------------------------------------------------------------
const char longNames[][16] PROGMEM = 
{
	{""},
	//012345678
	{"Coarse"},
	{"Fine"},
	{"Attack"},
	{"Decay"},
	{"Amount"},
	{"Frequency"},
	{"Overdrive"},
	{"Volume"},
	{"Panning"},
	{"Mix"},
	{"Repeat Ct"},
	{"Resonance"},
	{"Type"},
	{"Gain"},
	{"Waveform"},
	{"Dst Param"},
	{"Clock Snc"},
	{"Retrigger"},
	{"Offset"},		
	{"Dst Voice"},
	{"Slope"},
	{"Dcy Closd"},
	{"Dcy Open"},
#if USE_CODEC_EQ		
	{"Enable"},
	{"Release"},
	{"Threshold max"},
	{"Threshold min"},
#endif		
	{"Roll Rate"},
	{"Morph"},
	{"Note"},
	{"Prbabilty"},
	{"Number"},
	{"Length"},
	{"Steps"},
	{"Tempo"},
	{"Audio Out"},
	{"MIDI Chan"},
	{"Sample Rt"},
	{"Next Patt"},
	{"Phase"},
	{"Mode"},
	{"Vol mod"},
	{"Fetch"},	
	{"Follow"},	
	{"Quantize"},
	{"Aut Track"},
	{"Aut Dest"},
	{"Aut Value"},
	{"Shuffle"},
	{"Screensvr"},
	{"X Positn"},
	{"Y Positn"},
	{"Flux"},
	{"Velocity"},	
};


#endif /* MENUTEXT_H_ */
