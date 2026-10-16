/*
 * GlobalMidiParser.c
 *
 * MIDI-wide system handling that does not belong to the per-channel parser.
 */

#include "GlobalMidiParser.h"

#include "ChannelMidiParser.h"
#include "MidiParser.h"
#include "Preset/ParameterArray.h"
#include "Preset/ParameterIngress.h"
#include "clockSync.h"
#include "globals.h"
#include "mixer.h"
#include "modulationNode.h"
#include "sequencer.h"
#include "uARTFrontSYX/frontPanelReceivingProtocol.h"
#include "uARTFrontSYX/frontPanelSendingProtocol.h"
#include "usb_manager.h"
#include "valueShaper.h"

static inline uint8_t midiParser_voiceMidiChannel(uint8_t voice)
{
   /* Local voice-to-MIDI-channel lookup used by the global CC ladder. */
   return (voice < 8) ? midi_MidiChannels[voice] : 0;
}

// this will be set to some value if we are ignoring all mtc messages until the next 0 message
static uint8_t midiParser_mtcIgnore = 1;
static volatile uint32_t midiParser_lastMtcReceived = 0x0;
static uint8_t midiParser_mtcIsRunning = 0;
static uint16_t globalMidiParser_activeNrpnNumber = 0;
static uint8_t globalMidiParser_nrpnSelected = 0;

/* Special NRPN numbers (93-96) are defined in GlobalMidiParser.h so receive
   and menu TX share them (Session 039 P3). */
#define GLOBAL_MIDI_ROLL_RATE_MAX       15
/* Data value that, on CC100/CC101 right after an NRPN data entry, forms the
   standard "RPN null" deselect. */
#define GLOBAL_MIDI_RPN_NULL_VALUE      127

/* ¢ RX-FILTER STATE  (Session 039)
   WHAT: 1 while the AVR RxFilter is one of the ¢ entries.
   WHY:  gates CC38 consumption and NRPN 96 (see globalMidiParser_setCc38Swap()).
   WRITER: globalMidiParser_setCc38Swap() only (from midi_setFilter()).
   READERS: globalMidiParser_handleNrpnControl(),
            globalMidiParser_applySpecialNrpn(). Default 0 = published
            behaviour until the AVR sends its globals at boot. */
static uint8_t globalMidiParser_cc38Swap = 0;

/* RPN-NULL WINDOW  (Session 039)
   WHAT: 1 from an NRPN CC6 data entry (with a selected NRPN) until the next
         Global CC that is not ¢-consumed CC38 or a null byte.
   WHY:  lets CC100/101 = 127 be swallowed as an NRPN deselect only when it
         directly follows NRPN traffic, so deliberate Snare/Cymbal pan moves
         on CC100/101 keep working.
   WRITER/READER: globalMidiParser_handleNrpnControl() only. */
static uint8_t globalMidiParser_nrpnDataPending = 0;

#define GLOBAL_MIDI_RAW_UNMAPPED PAR_NONE

/* Table indices are fixed legacy external Global CC numbers. Values are raw
   AVR/Preset PAR_* ids, never MidiMessages.h low apply ids: Preset storage,
   AVR PARAM_CC packets, and pattern automation all use this raw domain. */
static const uint16_t globalMidiParser_ccToRawParam[128] = {
   [2] = PAR_OSC_WAVE_DRUM1,
   [3] = PAR_OSC_WAVE_DRUM2,
   [4] = PAR_OSC_WAVE_DRUM3,
   [5] = PAR_OSC_WAVE_SNARE,
   [7] = PAR_WAVE1_CYM,
   [8] = PAR_WAVE1_HH,
   [9] = PAR_COARSE1,
   [10] = PAR_FINE1,
   [11] = PAR_COARSE2,
   [12] = PAR_FINE2,
   [13] = PAR_COARSE3,
   [14] = PAR_FINE3,
   [15] = PAR_COARSE4,
   [16] = PAR_FINE4,
   [17] = PAR_COARSE5,
   [18] = PAR_FINE5,
   [19] = PAR_COARSE6,
   [20] = PAR_FINE6,
   [21] = PAR_MOD_WAVE_DRUM1,
   [22] = PAR_MOD_WAVE_DRUM2,
   [23] = PAR_MOD_WAVE_DRUM3,
   [24] = PAR_WAVE2_CYM,
   [25] = PAR_WAVE3_CYM,
   [26] = PAR_WAVE2_HH,
   [27] = PAR_WAVE3_HH,
   [28] = PAR_NOISE_FREQ1,
   [29] = PAR_MIX1,
   [30] = PAR_MOD_OSC_F1_CYM,
   [31] = PAR_MOD_OSC_F2_CYM,
   [32] = PAR_MOD_OSC_GAIN1_CYM,
   [33] = PAR_MOD_OSC_GAIN2_CYM,
   [34] = PAR_MOD_OSC_F1,
   [35] = PAR_MOD_OSC_F2,
   [36] = PAR_MOD_OSC_GAIN1,
   [37] = PAR_MOD_OSC_GAIN2,
   [38] = PAR_FILTER_FREQ_1,
   [39] = PAR_FILTER_FREQ_2,
   [40] = PAR_FILTER_FREQ_3,
   [41] = PAR_FILTER_FREQ_4,
   [42] = PAR_FILTER_FREQ_5,
   [43] = PAR_FILTER_FREQ_6,
   [44] = PAR_RESO_1,
   [45] = PAR_RESO_2,
   [46] = PAR_RESO_3,
   [47] = PAR_RESO_4,
   [48] = PAR_RESO_5,
   [49] = PAR_RESO_6,
   [50] = PAR_VELOA1,
   [51] = PAR_VELOD1,
   [52] = PAR_VELOA2,
   [53] = PAR_VELOD2,
   [54] = PAR_VELOA3,
   [55] = PAR_VELOD3,
   [56] = PAR_VELOA4,
   [57] = PAR_VELOD4,
   [58] = PAR_VELOA5,
   [59] = PAR_VELOD5,
   [60] = PAR_VELOA6,
   [61] = PAR_VELOD6_CLOSED,
   [62] = PAR_VELOD6_OPEN,
   [63] = PAR_VOL_SLOPE1,
   [64] = PAR_VOL_SLOPE2,
   [65] = PAR_VOL_SLOPE3,
   [66] = PAR_VOL_SLOPE4,
   [67] = PAR_VOL_SLOPE5,
   [68] = PAR_VOL_SLOPE6,
   [69] = PAR_REPEAT4,
   [70] = PAR_REPEAT5,
   [71] = PAR_MOD_EG1,
   [72] = PAR_MOD_EG2,
   [73] = PAR_MOD_EG3,
   [74] = PAR_MOD_EG4,
   [75] = PAR_MODAMNT1,
   [76] = PAR_MODAMNT2,
   [77] = PAR_MODAMNT3,
   [78] = PAR_MODAMNT4,
   [79] = PAR_PITCH_SLOPE1,
   [80] = PAR_PITCH_SLOPE2,
   [81] = PAR_PITCH_SLOPE3,
   [82] = PAR_PITCH_SLOPE4,
   [83] = PAR_FMAMNT1,
   [84] = PAR_FM_FREQ1,
   [85] = PAR_FMAMNT2,
   [86] = PAR_FM_FREQ2,
   [87] = PAR_FMAMNT3,
   [88] = PAR_FM_FREQ3,
   [89] = PAR_VOL1,
   [90] = PAR_VOL2,
   [91] = PAR_VOL3,
   [92] = PAR_VOL4,
   [93] = PAR_VOL5,
   [94] = PAR_VOL6,
   [95] = PAR_PAN1,
   [96] = PAR_PAN2,
   [97] = PAR_PAN3,
   [100] = PAR_PAN4,
   [101] = PAR_PAN5,
   [102] = PAR_PAN6,
   [103] = PAR_DRIVE1,
   [104] = PAR_DRIVE2,
   [105] = PAR_DRIVE3,
   [106] = PAR_SNARE_DISTORTION,
   [107] = PAR_CYMBAL_DISTORTION,
   [108] = PAR_HAT_DISTORTION,
   [109] = PAR_VOICE_DECIMATION1,
   [110] = PAR_VOICE_DECIMATION2,
   [111] = PAR_VOICE_DECIMATION3,
   [112] = PAR_VOICE_DECIMATION4,
   [113] = PAR_VOICE_DECIMATION5,
   [114] = PAR_VOICE_DECIMATION6,
   [115] = PAR_VOICE_DECIMATION_ALL,
   [116] = PAR_FREQ_LFO1,
   [117] = PAR_FREQ_LFO2,
   [118] = PAR_FREQ_LFO3,
   [119] = PAR_FREQ_LFO4,
   [120] = PAR_FREQ_LFO5,
   [121] = PAR_FREQ_LFO6,
   [122] = PAR_AMOUNT_LFO1,
   [123] = PAR_AMOUNT_LFO2,
   [124] = PAR_AMOUNT_LFO3,
   [125] = PAR_AMOUNT_LFO4,
   [126] = PAR_AMOUNT_LFO5,
   [127] = PAR_AMOUNT_LFO6,
};

/* NRPN values are already canonical high raw ids: this range begins at 128,
   so unlike low parameters it must not receive the MIDI apply +1 conversion. */
static const uint16_t globalMidiParser_nrpnToRawParam[] = {
   PAR_FILTER_DRIVE_1,
   PAR_FILTER_DRIVE_2,
   PAR_FILTER_DRIVE_3,
   PAR_FILTER_DRIVE_4,
   PAR_FILTER_DRIVE_5,
   PAR_FILTER_DRIVE_6,
   PAR_MIX_MOD_1,
   PAR_MIX_MOD_2,
   PAR_MIX_MOD_3,
   PAR_VOLUME_MOD_ON_OFF1,
   PAR_VOLUME_MOD_ON_OFF2,
   PAR_VOLUME_MOD_ON_OFF3,
   PAR_VOLUME_MOD_ON_OFF4,
   PAR_VOLUME_MOD_ON_OFF5,
   PAR_VOLUME_MOD_ON_OFF6,
   PAR_VELO_MOD_AMT_1,
   PAR_VELO_MOD_AMT_2,
   PAR_VELO_MOD_AMT_3,
   PAR_VELO_MOD_AMT_4,
   PAR_VELO_MOD_AMT_5,
   PAR_VELO_MOD_AMT_6,
   PAR_VEL_DEST_1,
   PAR_VEL_DEST_2,
   PAR_VEL_DEST_3,
   PAR_VEL_DEST_4,
   PAR_VEL_DEST_5,
   PAR_VEL_DEST_6,
   PAR_WAVE_LFO1,
   PAR_WAVE_LFO2,
   PAR_WAVE_LFO3,
   PAR_WAVE_LFO4,
   PAR_WAVE_LFO5,
   PAR_WAVE_LFO6,
   PAR_VOICE_LFO1,
   PAR_VOICE_LFO2,
   PAR_VOICE_LFO3,
   PAR_VOICE_LFO4,
   PAR_VOICE_LFO5,
   PAR_VOICE_LFO6,
   PAR_TARGET_LFO1,
   PAR_TARGET_LFO2,
   PAR_TARGET_LFO3,
   PAR_TARGET_LFO4,
   PAR_TARGET_LFO5,
   PAR_TARGET_LFO6,
   PAR_RETRIGGER_LFO1,
   PAR_RETRIGGER_LFO2,
   PAR_RETRIGGER_LFO3,
   PAR_RETRIGGER_LFO4,
   PAR_RETRIGGER_LFO5,
   PAR_RETRIGGER_LFO6,
   PAR_SYNC_LFO1,
   PAR_SYNC_LFO2,
   PAR_SYNC_LFO3,
   PAR_SYNC_LFO4,
   PAR_SYNC_LFO5,
   PAR_SYNC_LFO6,
   PAR_OFFSET_LFO1,
   PAR_OFFSET_LFO2,
   PAR_OFFSET_LFO3,
   PAR_OFFSET_LFO4,
   PAR_OFFSET_LFO5,
   PAR_OFFSET_LFO6,
   PAR_FILTER_TYPE_1,
   PAR_FILTER_TYPE_2,
   PAR_FILTER_TYPE_3,
   PAR_FILTER_TYPE_4,
   PAR_FILTER_TYPE_5,
   PAR_FILTER_TYPE_6,
   PAR_TRANS1_VOL,
   PAR_TRANS2_VOL,
   PAR_TRANS3_VOL,
   PAR_TRANS4_VOL,
   PAR_TRANS5_VOL,
   PAR_TRANS6_VOL,
   PAR_TRANS1_WAVE,
   PAR_TRANS2_WAVE,
   PAR_TRANS3_WAVE,
   PAR_TRANS4_WAVE,
   PAR_TRANS5_WAVE,
   PAR_TRANS6_WAVE,
   PAR_TRANS1_FREQ,
   PAR_TRANS2_FREQ,
   PAR_TRANS3_FREQ,
   PAR_TRANS4_FREQ,
   PAR_TRANS5_FREQ,
   PAR_TRANS6_FREQ,
   PAR_AUDIO_OUT1,
   PAR_AUDIO_OUT2,
   PAR_AUDIO_OUT3,
   PAR_AUDIO_OUT4,
   PAR_AUDIO_OUT5,
   PAR_AUDIO_OUT6,
};

static uint8_t globalMidiParser_isGlobalChannel(MidiMsg msg)
{
   const uint8_t chanonly = (msg.status & 0x0F) + 1;
   return (chanonly == midiParser_voiceMidiChannel(7));
}

/* Converts a raw Global lookup target solely for the legacy live DSP switch.
   Example: PAR_VOL6 is raw 93, while the switch case VOL6 is MIDI apply 94.
   Raw AVR/Preset/restore traffic must not use this conversion. */
static uint16_t globalMidiParser_midiApplyParamFromRaw(uint16_t rawParam)
{
   if(rawParam == GLOBAL_MIDI_RAW_UNMAPPED
      || rawParam == PAR_RESERVED4
      || rawParam >= END_OF_SOUND_PARAMETERS)
      return GLOBAL_MIDI_RAW_UNMAPPED;

   if(rawParam < PAR_RESERVED4)
      return rawParam + 1;

   if(rawParam >= PAR_FILTER_DRIVE_1)
      return rawParam;

   return GLOBAL_MIDI_RAW_UNMAPPED;
}

/* Global MIDI lookup tables yield raw Preset ids. Store that raw id first,
   then apply its MIDI-shaped equivalent to the live DSP switch. Keeping these
   operations separate prevents updateOriginalValue from storing a low MIDI
   apply id as though it were an AVR/Preset raw parameter id. */
static void globalMidiParser_applyRawParameter(uint16_t rawParam,
                                               uint8_t value,
                                               uint8_t updateOriginalValue,
                                               enum MidiSource source)
{
   MidiMsg internalMsg = {0};
   const uint16_t midiApplyParam =
      globalMidiParser_midiApplyParamFromRaw(rawParam);

   if(midiApplyParam == GLOBAL_MIDI_RAW_UNMAPPED)
      return;

   if(updateOriginalValue)
   {
      /* Preset owns the canonical raw parameter image used by morph, reload,
         automation release, and AVR protocol traffic. */
      preset_storeParameterIngress(rawParam, value);
      frontParser_originalCcValues[midiApplyParam] = value;
   }

   if(midiApplyParam < 128)
   {
      internalMsg.status = MIDI_CC;
      internalMsg.data1 = (uint8_t)midiApplyParam;
   }
   else
   {
      internalMsg.status = MIDI_CC2;
      internalMsg.data1 = (uint8_t)(midiApplyParam - 128);
   }

   internalMsg.data2 = value;
   internalMsg.bits.source = source;
   internalMsg.bits.sysxbyte = 0;
   internalMsg.bits.length = 2;

   /* Storage above is deliberately raw-domain. Zero keeps this legacy DSP
      helper from repeating its updateOriginalValue storage path with the MIDI
      apply id carried in internalMsg.data1. */
   frontParser_applyParameterCommand(internalMsg, 0);

   if((midiParser_txRxFilter & 0x04) && updateOriginalValue)
   {
      if(seq_recordActive)
         seq_recordAutomationMidiDestination(frontParser_activeTrack, midiApplyParam, value);
      else
         channelMidiParser_sendParameterEcho(midiApplyParam, value);
   }
}

/* GLOBAL SPECIAL-NRPN APPLY  (Session 039)
   WHAT: applies the Global NRPN numbers that bypass
         globalMidiParser_nrpnToRawParam[]:
           93 roll rate      -> seq_setRollRate(clamp 0..15)  + AVR report
           94 roll note      -> seq_setRollNote(0..127)       + AVR report
           95 roll velocity  -> seq_setRollVelocity(0..127)   + AVR report
           96 Drum 1 cutoff  -> PAR_FILTER_FREQ_1, ¢ mode only (otherwise
                                not handled, so the caller's range check
                                ignores it as before)
   WHY:  roll rate/note/velocity are runtime PERF controls, not raw sound
         parameters, so they cannot live in the raw table; they are not
         recorded as automation or echoed (unchanged NRPN 93 behaviour).
         Each roll change is reported to the AVR so the PERF page shows the
         MIDI value; otherwise the next encoder/pot move would resend the
         stale menu value and overwrite it. Reports are sent on every data
         entry (no change cache): the front panel can also set these values,
         so an STM-side cache could go stale and suppress a needed report.
         NRPN 96 reuses globalMidiParser_applyRawParameter(), the exact
         store/apply/echo/record path Global CC38 uses, so ¢ mode only moves
         the transport, never the semantics.
   INPUT:  nrpn - selected NRPN number; value - CC6 data (0..127);
           updateOriginalValue, source - forwarded for NRPN 96.
   OUTPUT: 1 if handled (caller returns), 0 if not a special NRPN (or NRPN
           96 outside ¢ mode).
   ACCESSORS: globalMidiParser_handleNrpnControl() (CC6 case) only.
   AFFILIATES: seq_setRollRate()/seq_setRollNote()/seq_setRollVelocity()
               (Sequencer/sequencer.c), frontPanelSending_sendRollParamReport()
               (uARTFrontSYX/frontPanelSendingProtocol.c), AVR
               SEQ_REPORT_ROLL_* handlers (avrCommsReceivingProtocol.c),
               globalMidiParser_cc38Swap. */
static uint8_t globalMidiParser_applySpecialNrpn(uint16_t nrpn,
                                                 uint8_t value,
                                                 uint8_t updateOriginalValue,
                                                 enum MidiSource source)
{
   switch(nrpn)
   {
      case GLOBAL_MIDI_NRPN_ROLL_RATE:
         if(value > GLOBAL_MIDI_ROLL_RATE_MAX)
            value = GLOBAL_MIDI_ROLL_RATE_MAX;
         seq_setRollRate(value);
         frontPanelSending_sendRollParamReport(FRONT_SEQ_REPORT_ROLL_RATE, value);
         return 1;

      case GLOBAL_MIDI_NRPN_ROLL_NOTE:
         seq_setRollNote(value);
         frontPanelSending_sendRollParamReport(FRONT_SEQ_REPORT_ROLL_NOTE, value);
         return 1;

      case GLOBAL_MIDI_NRPN_ROLL_VELOCITY:
         seq_setRollVelocity(value);
         frontPanelSending_sendRollParamReport(FRONT_SEQ_REPORT_ROLL_VELOCITY, value);
         return 1;

      case GLOBAL_MIDI_NRPN_FILTER_FREQ_1:
         if(!globalMidiParser_cc38Swap)
            return 0;
         globalMidiParser_applyRawParameter(PAR_FILTER_FREQ_1,
                                            value,
                                            updateOriginalValue,
                                            source);
         return 1;

      default:
         return 0;
   }
}

/* GLOBAL NRPN PROTOCOL HANDLER
   WHAT: consumes NRPN protocol CCs on the Global channel and applies the
         selected NRPN on data entry:
           CC99/CC98  select the NRPN number (MSB/LSB) and clear the
                      RPN-null window;
           CC6        applies the selected NRPN: special numbers first
                      (globalMidiParser_applySpecialNrpn(): 93-95 roll,
                      96 Drum 1 cutoff in ¢ mode), then the raw table 0..92;
                      opens the RPN-null window;
           CC38       consumed only in ¢ mode (data-entry LSB);
           CC100/101  value 127 inside the RPN-null window is the standard
                      NRPN deselect: consumed, clears nrpnSelected.
         Every other CC closes the RPN-null window and returns 0 so the
         caller applies it through the Global CC table.
   WHY:  without consumption, standard NRPN traffic writes published sound
         parameters: Elektron CC38 -> Drum 1 cutoff (¢ mode fixes it),
         RPN-null CC101/100 -> Cymbal/Snare hard pan (fixed in all modes).
         The window keeps deliberate CC100/101 pan moves working; only a
         127 sent immediately after an NRPN data entry is treated as a null.
   INPUT:  msg - Global-channel MIDI_CC; updateOriginalValue - forwarded to
           the apply helpers.
   OUTPUT: 1 if the CC was consumed here, 0 if the caller must apply it as
           an ordinary Global CC. Updates globalMidiParser_activeNrpnNumber,
           _nrpnSelected and _nrpnDataPending.
   ACCESSORS: globalMidiParser_MIDIccHandler() only.
   AFFILIATES: globalMidiParser_applySpecialNrpn(),
               globalMidiParser_applyRawParameter(),
               globalMidiParser_nrpnToRawParam[], globalMidiParser_cc38Swap,
               MIDI_TABLE.md "Global MIDI NRPN Table". */
static uint8_t globalMidiParser_handleNrpnControl(MidiMsg msg,
                                                  uint8_t updateOriginalValue)
{
   switch(msg.data1)
   {
      case NRPN_FINE:          /* CC98 */
         globalMidiParser_activeNrpnNumber &= (uint16_t)~0x7f;
         globalMidiParser_activeNrpnNumber |= (uint16_t)(msg.data2 & 0x7f);
         globalMidiParser_nrpnSelected = 1;
         globalMidiParser_nrpnDataPending = 0;
         return 1;

      case NRPN_COARSE:        /* CC99 */
         globalMidiParser_activeNrpnNumber &= 0x7f;
         globalMidiParser_activeNrpnNumber |= (uint16_t)(msg.data2 << 7);
         globalMidiParser_nrpnSelected = 1;
         globalMidiParser_nrpnDataPending = 0;
         return 1;

      case NRPN_DATA_ENTRY_COARSE:   /* CC6 */
         /* A data entry opens the RPN-null window only if it addressed a
            selected NRPN; CC6 before any selector stays ignored. */
         globalMidiParser_nrpnDataPending = globalMidiParser_nrpnSelected;

         if(!globalMidiParser_nrpnSelected)
            return 1;

         if(globalMidiParser_applySpecialNrpn(globalMidiParser_activeNrpnNumber,
                                              msg.data2,
                                              updateOriginalValue,
                                              msg.bits.source))
            return 1;

         if(globalMidiParser_activeNrpnNumber
               < (sizeof(globalMidiParser_nrpnToRawParam)
                  / sizeof(globalMidiParser_nrpnToRawParam[0])))
         {
            globalMidiParser_applyRawParameter(
               globalMidiParser_nrpnToRawParam[globalMidiParser_activeNrpnNumber],
               msg.data2,
               updateOriginalValue,
               msg.bits.source);
         }
         return 1;

      case DATA_MSB_LSB:       /* CC38: data-entry LSB */
         /* ¢ mode: consume. The RPN-null window is left as-is because an
            Elektron sender emits CC38 between CC6 and any null. */
         if(globalMidiParser_cc38Swap)
            return 1;
         globalMidiParser_nrpnDataPending = 0;
         return 0;            /* published Drum 1 cutoff mapping */

      case RPN_LSB:            /* CC100 */
      case RPN_MSB:            /* CC101 */
         if(globalMidiParser_nrpnDataPending
            && msg.data2 == GLOBAL_MIDI_RPN_NULL_VALUE)
         {
            /* Standard deselect: a later CC6 must not re-apply the NRPN.
               The window stays open so the paired null byte is consumed too. */
            globalMidiParser_nrpnSelected = 0;
            return 1;
         }
         globalMidiParser_nrpnDataPending = 0;
         return 0;            /* published Snare/Cymbal pan mapping */

      default:
         globalMidiParser_nrpnDataPending = 0;
         return 0;
   }
}

uint8_t globalMidiParser_handleSystemMessage(MidiMsg msg)
{
   /* Global MIDI clock/start/stop/MTC handling lives here. */
   switch(msg.status)
   {
      case MIDI_CLOCK:
         if((midiParser_txRxFilter & 0x02) && seq_getExtSync())
            seq_sync();
         return 1;

      case MIDI_START:
      case MIDI_CONTINUE:
         if((midiParser_txRxFilter & 0x02) && seq_getExtSync())
            sync_midiStartStop(1);
         return 1;

      case MIDI_STOP:
         if((midiParser_txRxFilter & 0x02) && seq_getExtSync())
            sync_midiStartStop(0);
         return 1;

      case MIDI_MTC_QFRAME:
         /* --AS Strategy:
          * If we get through all 8 of the mtc messages with a value of 0, it means that
          * the song has just started playing. That is the only time we will start the
          * sequencer. so we will not start it if they start the tape recorder playing half way
          * through the song. Also, if the sequencer is running, or we are in external sync mode
          * we will ignore mtc messages as well
          */
         if((midiParser_txRxFilter & 0x02) == 0)
            return 1;

         // keep track of when we got the last mtc. only do this for the 0 msg to save time
         if((msg.data1 & 0x70) == 0)
         {
            midiParser_lastMtcReceived = systick_ticks;
         }

         if(seq_isRunning())
            return 1; // already running, so we don't care to figure out where we are

         if(seq_getExtSync())
            return 1; // bypass the lot. we are using external midi clock sync

         // figure out whether we are at 0:0:0:0
         if((msg.data1 & 0x7F) == 0)
         {
            // this is the first mtc message of the set AND it's value is 0
            midiParser_mtcIgnore = 0; // reset our level of ignorance
         }
         else if(midiParser_mtcIgnore)
         {
            return 1; // not the first msg and we are ignoring
         }
         else if((msg.data1 & 0x70) != 0x70)
         {
            if((msg.data1 & 0x0F) != 0)
            {
               midiParser_mtcIgnore = 1;
               return 1;
            }
         }
         else
         {
            // message 7 and we are not ignoring yet
            if((msg.data1 & 0x01) == 0)
            {
               // well, we got all the way thru all 8 messages with 0, so the song has just begun
               // tell the front that we've started running on our own
               frontPanelSending_sendRunStop(1);
               midiParser_mtcIgnore = 1; // in case we happen to miss a 0 message. probably wouldn't happen, but...
               midiParser_mtcIsRunning = 1;
               midiParser_lastMtcReceived = systick_ticks; // also might not be needed, but...
               // start the sequencer
               seq_setRunning(1);
            }
         }

         return 1;

      default:
         return 0;
   }
}

// this will check whether mtc is running, and if so will
// check to see whether we need to stop the sequencer due to
// lack of recent mtc activity. It will also reset our running indicator
// if the sequencer has stopped for some other reason
void midiParser_checkMtc(void)
{
   /* Watch the MTC running state and stop the sequencer if it goes stale. */
   if(!midiParser_mtcIsRunning)
      return;

   if(!seq_isRunning())
   {
      // inform mtc that his services are no longer needed
      midiParser_mtcIsRunning = 0;
      return;
   }

   // at a 24fps framerate (the lowest) we should receive a completed message (we receive one every 2 frames)
   // every 83 ms. our tick counter is .25 ms resolution
   if(systick_ticks - midiParser_lastMtcReceived > 100 * 4)
   {
      // overestimate, just in case something untoward should happen
      // too much time has elapsed since our last message. mtc has gone away.
      midiParser_mtcIsRunning = 0;

      frontPanelSending_sendRunStop(0);
      // stop the sequencer
      seq_setRunning(0);
   }
}
/* GLOBAL-CHANNEL CC ROUTER
   WHAT: handles CC2-127 on the configured Global MIDI channel. NRPN protocol
         bytes (CC6/98/99, ¢-mode CC38, RPN-null CC100/101 = 127) are offered
         to globalMidiParser_handleNrpnControl() first; anything it does not
         consume is applied through the fixed legacy Global CC table.
   WHY:  NRPN protocol bytes must be consumed before the CC table, or they
         land on published sound parameters (CC38 = Drum 1 cutoff,
         CC100/101 = Snare/Cymbal pan).
   INPUT:  msg - MIDI_CC; updateOriginalValue - 1 to store/record/echo.
   OUTPUT: live DSP apply, Preset raw storage, optional echo/record.
   ACCESSORS: midiParser_MIDIccHandler() (MidiParser.c).
   AFFILIATES: globalMidiParser_handleNrpnControl(),
               globalMidiParser_applyRawParameter(), MIDI_TABLE.md. */
void globalMidiParser_MIDIccHandler(MidiMsg msg, uint8_t updateOriginalValue)
{
   uint16_t rawParam;

   if(!globalMidiParser_isGlobalChannel(msg))
      return;

   if(msg.data1 == BANK || msg.data1 == MOD_WHEEL)
      return;

   if(globalMidiParser_handleNrpnControl(msg, updateOriginalValue))
      return;

   /* The index is the fixed external legacy CC number; the table result is a
      raw Preset id that the typed helper converts only for live DSP apply. */
   rawParam = globalMidiParser_ccToRawParam[msg.data1 & 0x7f];
   globalMidiParser_applyRawParameter(rawParam,
                                      msg.data2,
                                      updateOriginalValue,
                                      msg.bits.source);
}

/* GLOBAL CC38 SWAP SETTER  (Session 039)
   WHAT: stores the ¢ RX-filter flag decoded by midi_setFilter().
   WHY:  single writer for globalMidiParser_cc38Swap; see GlobalMidiParser.h.
   INPUT:  enabled - nonzero enables the swap.
   OUTPUT: globalMidiParser_cc38Swap = 0/1.
   ACCESSORS: midi_setFilter() (MidiParser.c).
   AFFILIATES: globalMidiParser_handleNrpnControl(),
               globalMidiParser_applySpecialNrpn() (readers). */
void globalMidiParser_setCc38Swap(uint8_t enabled)
{
   globalMidiParser_cc38Swap = enabled ? 1u : 0u;
}

/* GLOBAL TX REVERSE TABLE  (Session 039 P3)
   WHAT: per raw sound parameter, the Global transmit address: 0 none,
         1..127 = CC number, 128 + n = NRPN n (n <= 92). Built once, lazily,
         by globalMidiParser_buildTxAddressTable().
   WHY:  O(1) lookup for menu TX without duplicating the receive tables.
   WRITER: globalMidiParser_buildTxAddressTable(). READER:
           globalMidiParser_getTxAddress(). */
static uint8_t globalMidiParser_txAddress[END_OF_SOUND_PARAMETERS];
static uint8_t globalMidiParser_txAddressBuilt = 0;

/* TX EXCLUSION TEST  (Session 039 P3)
   WHAT: true for the 18 modulation target selectors (user decision 4).
   WHY:  the menu stores a modTargets index for these and receive stores the
         NRPN value raw; the two domains are not verified to match.
   INPUT: rawParam. OUTPUT: 1 excluded, 0 allowed.
   ACCESSORS: globalMidiParser_buildTxAddressTable(). */
static uint8_t globalMidiParser_isTxExcluded(uint16_t rawParam)
{
   return (uint8_t)(((rawParam >= PAR_VEL_DEST_1) && (rawParam <= PAR_VEL_DEST_6))
                 || ((rawParam >= PAR_VOICE_LFO1) && (rawParam <= PAR_VOICE_LFO6))
                 || ((rawParam >= PAR_TARGET_LFO1) && (rawParam <= PAR_TARGET_LFO6)));
}

/* TX REVERSE TABLE BUILDER  (Session 039 P3)
   WHAT: inverts globalMidiParser_nrpnToRawParam[] first, then
         globalMidiParser_ccToRawParam[2..127] so a CC address overwrites an
         NRPN for the same parameter (CC preferred: 3 bytes instead of 9).
         Skips PAR_NONE slots (CC0/1/6/98/99 and unmapped), out-of-range ids
         and the excluded selectors.
   WHY:  one derivation from the receive tables (see getTxAddress()).
   OUTPUT: globalMidiParser_txAddress[] filled; built flag set.
   ACCESSORS: globalMidiParser_getTxAddress() (first call). */
static void globalMidiParser_buildTxAddressTable(void)
{
   uint16_t i;
   const uint16_t nrpnCount = (uint16_t)(sizeof(globalMidiParser_nrpnToRawParam)
                                       / sizeof(globalMidiParser_nrpnToRawParam[0]));

   for(i = 0; i < nrpnCount; i++)
   {
      const uint16_t raw = globalMidiParser_nrpnToRawParam[i];
      if(raw == PAR_NONE || raw >= END_OF_SOUND_PARAMETERS || globalMidiParser_isTxExcluded(raw))
         continue;
      globalMidiParser_txAddress[raw] = (uint8_t)(128u + i);
   }

   for(i = 2; i < 128; i++)
   {
      const uint16_t raw = globalMidiParser_ccToRawParam[i];
      if(raw == PAR_NONE || raw >= END_OF_SOUND_PARAMETERS || globalMidiParser_isTxExcluded(raw))
         continue;
      globalMidiParser_txAddress[raw] = (uint8_t)i;
   }

   globalMidiParser_txAddressBuilt = 1;
}

/* GLOBAL TX ADDRESS LOOKUP (Session 039 P3); full contract in
   GlobalMidiParser.h. */
uint8_t globalMidiParser_getTxAddress(uint16_t rawParam, uint8_t *number)
{
   uint8_t code;

   if(rawParam >= END_OF_SOUND_PARAMETERS)
      return GLOBAL_MIDI_TX_NONE;

   if(!globalMidiParser_txAddressBuilt)
      globalMidiParser_buildTxAddressTable();

   /* ¢ mirror (user decision 5): record what a ¢-mode receiver accepts. */
   if(rawParam == PAR_FILTER_FREQ_1 && globalMidiParser_cc38Swap)
   {
      *number = GLOBAL_MIDI_NRPN_FILTER_FREQ_1;
      return GLOBAL_MIDI_TX_NRPN;
   }

   code = globalMidiParser_txAddress[rawParam];
   if(code == 0)
      return GLOBAL_MIDI_TX_NONE;

   if(code < 128)
   {
      *number = code;
      return GLOBAL_MIDI_TX_CC;
   }

   *number = (uint8_t)(code - 128);
   return GLOBAL_MIDI_TX_NRPN;
}
