/*
 * MenuText.h
 *
 * Created: 04.11.2012 08:52:54
 *  Author: Julian
 */ 


#ifndef MENUTEXT_H_
#define MENUTEXT_H_

//-----------------------------------------------------------------
// -bc- these get bitshifted by 4 - not sure if more than 15 will cause problems
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
#define MENU_MIDI			11 // -bc- this is possibly unused?
#define MENU_MIDI_ROUTING	12
#define MENU_MIDI_FILTERING 13

/* ¢ RX-FILTER CONSTANTS  (Session 039)
   MENU_CHAR_CENT: the cent sign on this LCD's character ROM is code 0x5C
     (between '[' 0x5B and ']' 0x5D), written as the C escape "\\". If a
     display variant renders 0x5C as '¥' or '\', define a CGRAM glyph with
     lcd_generatechar() (Hardware/lcd.c) in slot 1 and change this to "\x01";
     slot 0 cannot be used in strings.
   MIDI_FILTER_CENT_FIRST: first ¢ value of PAR_MIDI_FILT_RX (16).
   MIDI_FILTER_RX_ENTRIES: RX encoder range, 16 normal + 8 ¢ = 24.
   MIDI_FILTER_RX_CC38_SWAP: wire bit 4 sent with SEQ_MIDI_FILT_RX for ¢
     values; must equal STM MIDI_RX_FILTER_CC38_SWAP (MidiParser.h).
   ACCESSORS: midiFilterNames[], menu_getDtypeMenuEntryCount(),
              menu_parseKnobValue(), menu_parseGlobalParam() (menu.c). */
#define MENU_CHAR_CENT            "\\"
#define MIDI_FILTER_CENT_FIRST    16
#define MIDI_FILTER_RX_ENTRIES    24
#define MIDI_FILTER_RX_CC38_SWAP  0x10

#define MENU_PPQ		 	14
#define MENU_TRACK_SCALE 15
#define MENU_FILE_LOAD_BACKGROUND 0


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
const char menuText_step[] PROGMEM = "stp";
const char menuText_bar[] PROGMEM = "bar";
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
// -bc- I don't think this menu gets used any more... possible to retcon for something else?
const char midiModes[][4] PROGMEM  =
{
	{5},		//number of entries
	{"trg"},
	{"nte"},
   {"vel"},
   {"bth"},
   {"all"},
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
	{18},		//number of entries
	{"sin"},
	{"tri"},
	{"sup"},
	{"sdn"},
	{"sqr"},
	{"rnd"},
	{"xup"},
	{"xdn"},
   {"xtr"},
   {"si1"},
	{"tr1"},
	{"su1"},
	{"sd1"},
	{"sq1"},
	{"rn1"},
	{"xu1"},
	{"xd1"},
   {"xt1"},
};
//-----------------------------------------------------------------
const char rollRateNames[][4] PROGMEM  = 
{
	{16},		//number of entries
	{"One"},		
	{" 1d"},
   {"1/1"},
   {" 2d"},
	{"1/2"},
   {" 4d"},		
	{"1/4"},		
   {" 8d"},		
	{"1/8"},		
	{"16d"},		
	{"16"},		
	{"32d"},		
	{"32"},		
	{"64d"},		
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
	{8},		//number of entries
	{"LP"}	,	//1 - 001
	{"HP"}	,	//2 - 010
	{"BP"}	,	//3 - 011
	{"UBP"}	,	//4 - 100
	{"Nch"}	,	//5 - 101
	{"Pek"}	,	//6 - 110
	{"LP2"}	,	//1 - 001
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
const char midiFilterNames[][4] PROGMEM =
{
	{16},	// pot/TX entry count; RX encoder also reaches the ¢ rows below (MIDI_FILTER_RX_ENTRIES)
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
	/* ¢ RX TWINS  (Session 039)
	   WHAT: values 16..23 of PAR_MIDI_FILT_RX. Each is identical to the
	         C-entry it shadows (C, CN, CR, CRN, PC, PCN, PCR, all) plus the
	         CC38 swap: Global CC38 ignored, NRPN 96 = Drum 1 filter cutoff.
	   WHY:  opt-in workaround for Elektron NRPN senders, which append CC38.
	         Rows sit after the count byte's range, so pots and TxFilter
	         (which use getMaxEntriesForMenu() = 16) can never reach them; only
	         the RX encoder edit path (menu_getDtypeMenuEntryCount()) can.
	   ORDER IS ABI: value 16+k must keep RX bits
	         0x04 | (k & 3) | ((k & 4) << 1), as encoded in menu_parseGlobalParam().
	   ACCESSORS: getMenuItemNameForValue() (display only). */
	{MENU_CHAR_CENT},
	{MENU_CHAR_CENT "N"},
	{MENU_CHAR_CENT "R"},
	{MENU_CHAR_CENT "RN"},
	{"P" MENU_CHAR_CENT},
	{"P" MENU_CHAR_CENT "N"},
	{"P" MENU_CHAR_CENT "R"},
	{"al" MENU_CHAR_CENT},
};
//-----------------------------------------------------------------
const char trackScaleNames[][4] PROGMEM =
{
	{8},	// number of entries
	{"off"},
	{" x2"},
	{" x4"},
	{" x8"},
	{"x16"},
	{"x32"},
	{"x64"},
	{"128"},
};
//-----------------------------------------------------------------
const char backgroundLoadNames[][4] PROGMEM =
{
	{5},	// number of entries
	{"off"},
	{"pat"},
	{"prf"},
	{"all"},
	{"tot"},
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
   {"sca"},
	{"rot"},
   {"ssr"},
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
   {"pci"}, // pattern change time is bar or step
   {"stg"}, // shift is toggle instead of momentary
   {"b2p"}, // bank changes change perfs
   {"sfr"}, // skip first roll
   {"mrv"}, // morph individual voice

   // Legacy macro UI strings, kept as placeholders until the macro feature is
   // removed from the parameter/menu tables.
   {"d1m"}, // TEXT_MORPH_DRUM1,
   {"d2m"}, // TEXT_MORPH_DRUM2,
   {"d3m"}, // TEXT_MORPH_DRUM3,
   {"svm"}, // TEXT_MORPH_SNARE,
   {"cvm"}, // TEXT_MORPH_CYM,
   {"hvm"}, // TEXT_MORPH_HIHAT,
   
   {" "}, // TEXT_UNUSED1,
   {" "}, // TEXT_UNUSED2,
   {" "}, // TEXT_UNUSED3,
   {" "}, // TEXT_UNUSED4,
   {"rln"}, // TEXT_ROLL_NOTE,
   {"rlv"}, // TEXT_ROLL_VELOCITY
   {"rol"}, // TEXT_ROLL_MODE
   {"xps"}, // TEXT_TRANSPOSE,
   {"xps"}, // TEXT_TRANSPOSE_ON_OFF,
   {"flb"}, // TEXT_FILE_LOAD_BACKGROUND,
   {"egp"}, // TEXT_ENVELOPE_POSITION
   {"itp"}, // TEXT_OSC_WAVE_INTERPOLATION
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
   // Legacy macro long names retained for table stability during removal.
   {" "},
   {" "},
   {" "},
   {" "},
   {" "},
   {" "},
   {"Trnspos"},
   {"Individ"},
   {"File"},
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
   {"Scale"},
	{"Steps"},
	{"Rotation"},
   {"SubStpRt"},
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
   {"PCInstnt" }, // pattern change occurs on bar or step
   {"ShiftTog" }, // shift is toggle instead of momentary
   {"BnkToPrf" }, // Bank changes change perfs
   {"Skp1Roll"}, // skip first roll
   {"MrphVoice" }, // individual voice morph
   // Legacy macro long text entries kept only until the macro menu/table code
   // can be deleted.
   {"Drm1Mrph"  }, // TEXT_MORPH_DRUM1,
   {"Drm2Mrph"  }, // TEXT_MORPH_DRUM2,
   {"Drm3Mrph" }, // TEXT_MORPH_DRUM3,
   {"SnreMrph" }, // TEXT_MORPH_SNARE
   {"CymMrph" }, // TEXT_MORPH_CYM,
   {"HHatMrph" }, // TEXT_MORPH_HIHAT,
   
   {" " }, // TEXT_UNUSED1,
   {" " }, // TEXT_UNUSED1,
   {" " }, // TEXT_UNUSED1,
   {" " },  // TEXT_UNUSED1,  
      
   {"RollNote" },  // TEXT_ROLL_NOTE,
   {"RollVelo" },  // TEXT_ROLL_VELOCITY,
   {"RollMode" },  // TEXT_ROLL_MODE
   /* Long label for the global MIDI roll-note offset.
      Spelling follows the requested eight-character display string "RolOfset". */
   {"RolOfset" },  // TEXT_ROLL_NOTE_OFFSET
   {"NoteAmt"},   // TEXT_TRANSPOSE
   {"Sequence"}, // TEXT_TRANSPOSE_ON_OFF,
   {"LoadBgnd"}, // TEXT_FILE_LOAD_BACKGROUND,
   {"Position"}, // TEXT_ENVELOPE_POSITION
   {"WavIntrp"}, // TEXT_OSC_WAVE_INTERPOLATION
};


#endif /* MENUTEXT_H_ */
