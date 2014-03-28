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
#define MENU_MIDI_ROUTING	12
#define MENU_MIDI_FILTERING 13
#define MENU_PPQ		 	14

//-----------------------------------------------------------------
// Shared texts. Reduce mem usage by pooling common text
// all of these need to be 3 chars
const char menuText_ok[] PROGMEM = 	"ok ";
const char menuText_off[] PROGMEM = "off";
const char menuText_on[] PROGMEM = 	"on ";
const char menuText_mix[] PROGMEM = "mix";
const char menuText_fm[] PROGMEM = 	"fm ";
const char menuText_dash[] PROGMEM ="---";
const char menuText_blank[] PROGMEM="   ";
const char menuText_any[] PROGMEM = "any";
//-----------------------------------------------------------------
const char ppqNames[][4] PROGMEM  =
{
	{5},		//number of entries
	{"1"},
	{"4"},
	{"8"},
	{"16"},
	{"32"},

};
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
	{"off"},
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
	{"off"}	,	//7 - 111
	//{"All"}	,	//7 - 111
};

//-----------------------------------------------------------------
const char midiRoutingNames[][6] PROGMEM =
{
		{6},	// number of entries
		{"off"},
		{"U2M"},
		{"M2M"},
		{"A2M"},
		{"M2U"},
		{"M2A"},
};
//-----------------------------------------------------------------
const char midiFilterNames[][16] PROGMEM =
{
	{16},	// number of entries
	{"off"},
	{"N"},
	{"R"},
	{"RN"},
	{"C"},
	{"CN"},
	{"CR"},
	{"CRN"},
	{"P"},
	{"PN"},
	{"PR"},
	{"PRN"},
	{"PC"},
	{"PCN"},
	{"PCR"},
	{"all"},
};
//-----------------------------------------------------------------
// these must correspond with shortNamesEnum
const char shortNames[][4] PROGMEM  = 
{
	{""},
	{"coa"},
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
	{"mrp"},
	{"nte"},
	{"prb"},
	{"stp"},
	{"len"},
	{"rot"},
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
	{"shu"},
	{"ssv"},
	{"x"},
	{"y"},
	{"flx"},
	{"mid"},
	{"mrt"}, // midi routing
	{"txf"}, // midi tx filter
	{"rxf"},  // midi rx filter
	{"cki"},  // trigger clock in ppq
	{"co1"},  // trigger clock out1 ppq
	{"co2"},  // trigger clock out2 ppq
	{"pcr"}, // pattern change resets bar counter
};
//-----------------------------------------------------------------
// These correspond with the catNamesEnum in menu.h
const char catNames[][16] PROGMEM = 
{
	{""},
	//01234567
	{"Oscilltr"},
	{"Veloc EG"},
	{"Mod EG"},
	{"PitchMod"},
	{"FM"},
	{"Voice"},
	{"Noise"},
	{"Nois/Osc"},
	{"Filter"},
	{"Mod Osc"},
	{"LFO"},
	{"Transnt"},
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
	{"Parametr"},
	{"Sequencr"},
	{"Generatr"},
	{"MIDI"},
	{"Trigger"},
};
//-----------------------------------------------------------------
// these must correspond to longNamesEnum
const char longNames[][16] PROGMEM = 
{
	{""},
	//01234567
	{"Coarse"},
	{"Fine"},
	{"Attack"},
	{"Decay"},
	{"Amount"},
	{"Frequncy"},
	{"Overdriv"},
	{"Volume"},
	{"Panning"},
	{"Mix"},
	{"RepeatCt"},
	{"Resnance"},
	{"Type"},
	{"Gain"},
	{"Waveform"},
	{"DstParam"},
	{"ClockSnc"},
	{"Retriggr"},
	{"Offset"},		
	{"DstVoice"},
	{"Slope"},
	{"Dcy Clsd"},
	{"Dcy Open"},
#if USE_CODEC_EQ		
	{"Enable"},
	{"Release"},
	{"Threshold max"},
	{"Threshold min"},
#endif		
	{"RollRate"},
	{"Morph"},
	{"Note"},
	{"Prbablty"},
	{"Number"},
	{"Length"},
	{"Steps"},
	{"Rotation"},
	{"Tempo"},
	{"AudioOut"},
	{"Channel"},
	{"SampleRt"},
	{"NextPatt"},
	{"Phase"},
	{"Mode"},
	{"Vol mod"},
	{"Fetch"},	
	{"Follow"},	
	{"Quantize"},
	{"AutTrack"},
	{"Aut Dest"},
	{"AutValue"},
	{"Shuffle"},
	{"Screensv"},
	{"X Positn"},
	{"Y Positn"},
	{"Flux"},
	{"Velocity"},
	{"Freqcy 1"},
	{"Freqcy 2"},
	{"Gain 1"},
	{"Gain 2"},
	{"Routing"}, // midi routing
	{"TxFilter"}, // midi tx filtering
	{"RxFilter"}, // midi rx filtering
	{"In PPQ"},
	{"Out1 PPQ"},
	{"Out2 PPQ"},
	{"Gate Mode"},
	{"PCReset" }, // reset bar counter on manual pattern change
};


#endif /* MENUTEXT_H_ */
