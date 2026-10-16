# Operation Modes

The synth provides different operation modes, each optimised for a different task. The active mode
changes the function of the user interface controls.

## Voice Edit Mode

The voice edit mode is used to modify sound parameters and to do basic pattern editing.

- **Display (1):** parameters of the selected synthesis page.
- **Knobs (2):** edit the visible parameters.
- **Sequencer buttons (5):** set and remove the 16 main steps of the current track.
- **Select buttons (7):** select the active synthesis page (oscillator, envelope, mixer, etc.)
- **Voice buttons (9):** select the voice and sequencer track to edit; with shift, mute single voices.

### Pattern Editing

The 16 sequencer buttons set and remove steps in the selected pattern. Each button is a
$\frac{1}{16}$th note from a 4/4 bar. A lit LED above the button indicates the step is set.

### Step Parameter Editing

\begin{figure}[H]
  \centering
  \includegraphics[width=\linewidth]{front_panel.png}
  \caption{Front panel reference for using Voice mode.}
\end{figure}

1. Press and hold the shift button.
2. The display shows the step parameter page of the selected step.
3. Use the knobs and encoder to change parameters.
4. A flashing LED indicates the selected step.
5. Select a different step using the 16 sequencer buttons.

### Sub Step Editing

\begin{figure}[H]
  \centering
  \includegraphics[width=\linewidth]{substep_panel.jpeg}
  \caption{Substep divisions within single main sequencer step.}
\end{figure}

Sub steps can be set and removed in voice edit mode:

1. Press and hold the shift button.
2. Use the 16 sequencer buttons to select the main step whose sub steps you want to edit.
3. A flashing LED indicates the selected step.
4. The sub step pattern is shown on the 8 select button LEDs.
5. Set and remove sub steps using the 8 select buttons.

\attention{The step parameter menu always shows the parameters of the last-tweaked step.
Setting or removing a sub step causes the display to show that sub step's parameters. For more
ergonomic step parameter editing, use Step (Edit) mode.}

### Sound Editing

- The 7 voice buttons select the active voice (lit LED indicates selection).
- Voice parameters are divided into pages selectable with the 8 select buttons.
- Each voice type has its own parameter set, but the overall page structure is the same.
- Parameters shown in the display are edited with the 4 knobs.

**Example — changing the filter frequency of voice 3:**

1. Press Voice Button 3 to select the voice.
2. Press Select Button 6 to show the filter page.
3. Use the first knob to adjust the \param{frq} parameter.

## Performance Mode

This mode is designed for playing songs live. You can chain and change patterns on the fly,
trigger manual rolls for each voice, loop the sequencer in smaller sub-divisions, and access
the morph and global sample rate effects. Voice editing is not available in this mode.

- **Display (1):** performance parameters.
- **Knobs (2):** change the performance parameters.
- **Sequencer buttons 1–7 (5):** trigger a manual roll for each corresponding voice.
- **Sequencer buttons 10-16 (5):** loop the sequencer in length from 1/2 bar to 1/64th note.
- **Sequencer button 9 (5):** while looping, increase the sequencer loop length by 50%.
- **Select buttons (7):** change the playing pattern.
- **Voice buttons (9):** mute/unmute voices without holding shift.

**Re-align pattern:** If tracks become misaligned (due to different pattern timescales, step lengths, or independent track pattern assignments), press the pattern button of the selected and viewed pattern again to realign all tracks to the master clock.

### Looper
Sequencer buttons 9–16 provide looper functionality in PERF mode. Button 10 sets the longest loop length (64 sub-steps, or 1/2 bar), halving at each subsequent button down to 16 (1 sub-step, or 1/64th note). Holding button 9 in addition 'dots' the loop length, adding 50% to the duration of the last loop button held. Releasing all loop buttons immediately returns the sequencer to the position it would have reached without looping engaged.

### Basic Performance Menu

\begin{paramtable}
shu & Shuffle amount    & Sets the amount of global shuffle. \\
rol & Manual roll rate  & Trigger rate for the manual roll function of sequencer buttons 1–7. \\
nte & Roll note         & Sets the note (pitch) used for the manual roll function. \\
vel & Roll velocity     & Sets the velocity used when triggering the manual roll function. \\
srt & Sample rate       & Global sample rate reduction effect. \\
mrp & Morph amount      & Ratio between the original drumkit sound and the morph drumkit target. \\
md1 & Drum 1 Morph      & Morph amount for just the Drum 1 voice. \\
md2 & Drum 2 Morph      & Morph amount for just the Drum 2 voice. \\
md3 & Drum 3 Morph      & Morph amount for just the Drum 3 voice. \\
msn & Snare Morph      & Morph amount for just the Snare voice. \\
mcy & Cymbal Morph      & Morph amount for just the Cymbal voice. \\
mhh & HiHat Morph      & Morph amount for just the HiHat voice. \\
\end{paramtable}

### Manual Roll

The first 7 sequencer buttons trigger a manual roll for each corresponding voice. The roll rate, note, and
velocity used are set with the \param{rol}, \param{nte}, and \param{vel} parameters. Rolls are recorded to 
the pattern when recording mode is active. Rolls can be recorded in three modes: full (pitch and velocity),
note only (pitch), or velocity only. The roll record mode is set in the SHIFT + REC button menu. 

Rolls can also be played and adjusted over MIDI:

- **MIDI roll trigger:** with the MIDI roll offset (\param{rol} in the Global Settings menu, described under *MIDI Roll Offset* below)
  switched on, MIDI notes above each voice's trigger note start and hold a roll on that voice.
- **Roll parameters by NRPN:** \param{rol}, \param{nte}, and \param{vel} can be set from the
  global MIDI channel with NRPN 93 (roll rate, values 0–15), NRPN 94 (roll note), and NRPN 95 (roll
  velocity). The Performance menu display follows the incoming values.

### Morph

The morph feature transitions from one preset sound to another. Load any preset from the SD card
as a morph target. The \param{mrp} parameter controls the ratio between the original and target
sound. As morph increases, the current sound is gradually transformed into the morph target.

Morph can be performed globally, to all voices at once, or to a single voice at a time. Global
morph is set from the PERF menu, or by Modulation (CC1) on the global MIDI channel, and overrides
the individual morph of all voices. 

Single voice morph can be set from the PERF menu, from CC1 on individual voice MIDI channels, from
step automation on the sequencer, through velocity automation, and by LFO modulation. All morph except
for LFO modulation will update the parameter shown in the PERF menu. 

For loading a morph target, see the Load and Save Mode section.

### Shuffle

The shuffle function shifts the position of every other $\frac{1}{16}$th note.

Original timing: `| X x x x X x x x X x x x X x x x |`

With shuffle applied: `| X xx xX xx xX xx xX xx x |`

### Change Pattern

The 8 select buttons change the playing pattern, selecting from the 8 patterns of the loaded
pattern set. By default, pattern changes occur only at the end of a bar. However, this behavior
can be toggled to 'instant' switching on the next sub-step, maintaining the same sequencer
position, by a global settings option.

Each individual track may play from a different pattern if desired. To change pattern on a per-
track basis, hold the 'voice' button of the track, and press the select button of the pattern
it should play. 

### Chain Patterns

To build longer pattern chains, go to the pattern options screen:

1. Press and hold shift.
2. A flashing LED indicates the selected pattern.
3. The display shows pattern options.

\oled{Rpt \quad nxt}{1 \hspace{2.5em} p2}

For each pattern you can specify the **next pattern** to play and the **repeat/change measure**.

**Change Measure / Repeat (rpt):** The change measure is relative to the absolute song position in
bars. This ensures pattern chains stay in sync regardless of when you manually switch patterns.

**Next Pattern (nxt):** Values p1–p8 select a specific pattern. Values r2–r8 select a random pattern
between pattern 1 and the selected value (e.g. r4 randomly selects from patterns 1–4).

\begin{figure}[H]
  \centering
  \includegraphics[width=\linewidth]{pattern_chain_example1.png}
  \caption{Pattern chain example: pattern 1 plays 3 times, then pattern 2 plays once.}
\end{figure}

\begin{figure}[H]
  \centering
  \includegraphics[width=\linewidth]{pattern_chain_example2.png}
  \caption{Pattern chain with manual switch: the break always lands on bar 4.}
\end{figure}

\tip{When editing multiple chained patterns, set the pattern follow (\param{flw}) parameter in the
settings menu to `Off`. This lets you edit a pattern without it jumping to follow playback.}

### Follow Mode and Viewed Pattern

The follow mode setting is in the global settings menu (on by default).

When follow mode is active, the viewed pattern always matches the playing pattern — all LEDs and
display values update automatically on every pattern change.

When follow mode is off, the user must manually select the viewed pattern. This allows editing a
different pattern from the one currently playing.

**Selecting the viewed pattern (performance mode only):**

1. The currently viewed pattern blinks; the playing pattern shows a solid LED.
2. Hold shift.
3. Use the 8 select buttons to pick the pattern to view/edit.
4. Release shift.

## Step (Edit) Mode

Step edit mode is for editing step parameters more efficiently. Think of it as an inverted voice
edit mode with respect to the sequencer.

The display always shows the step parameter page. The sequencer and select buttons are only used
to select steps — you can edit step parameters without holding shift.

The 16 sequencer buttons select a main step and show its 8 sub steps on the select button LEDs.
The selected step is indicated by a blinking LED.

### Setting and Removing Steps

To set or remove steps from the pattern, hold the shift button while pressing the sequencer buttons.

## Load and Save Mode

The 4th mode button opens the load/save page. Pressing it again toggles between load and save.

A push on the Load/Save button brings up the load page:

\oled{Load: Sound}{[ \quad 1] Name}

Pressing the button again shows the save page:

\oled{Save: Sound}{[ \quad 1] Name \hfill ok}

### Display Description

- **Row 1:** active mode (Load or Save) and the selected data type (Sound, Pattern, Morph Sound,
  Settings).
- **Row 2:** active preset number and the name of the selected preset.
- Pages with an `ok` button only execute when you navigate to `ok` and push the encoder.
- Pages without an `ok` button execute as soon as a change occurs.

### Menu Navigation

Use the encoder to navigate and change entries. Square brackets `[ ]` show the field that will be
changed by the encoder (edit mode). Pushing the encoder switches to selection mode (shows `>`).

\tip{For the data type field (Sound, Pattern, Morph Sound, Settings), use the first knob to select
directly without clicking through encoder modes.}

### Sound Presets

Sound presets contain all synthesis parameters for the 6 voices.

**Loading:** sounds load automatically when the preset number changes.

**Saving:**

1. Select the preset number.
2. Navigate to the name letters.
3. Push the encoder and turn it to change each letter.
4. Navigate to `ok` and push the encoder to save.

**Quick naming:** While the cursor is in the name area, knob 1 changes save type, knob 2 moves the cursor, and knobs 3 and 4 scroll through ASCII characters. Capital letters are positioned at the left of knob 3, followed by numbers in the middle. Lower case letters are positioned at the left of knob 4.

**Re-loading** Pressing SHIFT + PLAY reverts all voice parameters to their state at the last file load, discarding any live edits made to the parameters.

**Load individual drum voices:** The Load menu includes entries to load individual drum voices directly from .kit files without replacing the full drumkit. The name and number shown for a loaded voice reflect the kit it was derived from.

### Morph Presets

A morph preset is a normal preset loaded as a morph target.

**Loading:**

1. Select `MorphSnd` as the data type.
2. Change the preset number — the morph sound loads automatically.
3. Note: no audible change occurs if the \param{mrp} (morph amount) parameter is set to zero.

**Saving morph results:** select `MorphSnd` on the save screen to save the current blended sound
to a new preset slot.

### Pattern Sets

Pattern sets are collections of 8 patterns stored in one file.

**Loading:** navigate to `OK` on the display and push the encoder to start loading (required because
loading takes 4–5 seconds).

**Saving:** same procedure as saving a sound preset.

### Performance Files

A performance file (.prf) contains a drum kit, a morph kit, a pattern set, and a BPM tempo. Loading and saving follow the same procedure as for a pattern set.

### All Files

'All' files (.all) contain all the data of a performance file and, in addition, all global settings as below.Loading and saving follow the same procedures as for performance files.

### Background file loading

Pattern, performance and 'all' files may be loaded in the background while the sequencer is playing if the global setting matches that file type or 'tot' (total file set). When loading a file, the currently playing pattern and parameters are held in a temporary slot while the load executes in the background. The loaded sound becomes active when the next new pattern is played. The screen will indicate when a background load is initiated. Until the next pattern change, patgen and kit reload are disabled (SHIFT + PLAY, SHIFT + PERF). The performance mode LED will flash if performance mode is not selected, and the SELECT buttons will flash in performance mode until a newly loaded pattern is activated.

### Samples

In the Load menu, samples can be imported from the SD card using the last option, **Load Samples**. At least one of two folders should be present on the card, named 'SAMPLES' or 'LOOPS', containing 16-bit, mono pcm .wav files (44.1kHz). When selected, **Load Samples** imports files alphabetically from 'SAMPLES' and then 'LOOPS', to a maximum of 248 individual files, or ~300kB. When used as an oscillator waveform, the sample will always play looped if it was imported from the 'LOOPS' folder. Samples are retained after power-down and are only overwritten by a subsequent sample import or firmware update.

### Settings

The settings file contains all parameters from the settings menu. It is automatically loaded at
boot. To save current settings, go to save mode, select `Globals`, navigate to `Ok`, and push the
encoder.

## Pattern Generator Mode

The pattern generator creates polyrhythms automatically using the Euclidean algorithm. It can also 
apply offset to the steps of the pattern non-destructively, rotating the track through main steps and
sub-steps based on the settings.

### Accessing the Pattern Generator

1. Press and hold shift.
2. Press Mode Button 2 (Perf/Pattern).
3. A flashing mode 2 LED indicates the pattern generator is active.

### Pattern Generator Menu

\begin{paramtable}
len & Track Length   & Length of the track in $\frac{1}{16}$ notes. Each track can have a
                       different length. \\
stp & Number of Steps & The number of active steps to distribute in the pattern. \\
rot & Step Rotation   & Rotation of the track in $\frac{1}{16}$ notes. \\
ssr & Sub-step Rotation & Rotation of the track in $\frac{1}{128}$ notes.  \\
\end{paramtable}

### Generating Patterns

- The generator modifies a single track at a time. Select the track with the 7 voice buttons.
- Changing either \param{len} or \param{stp} immediately generates a new pattern on the active
  track.

\attention{The pattern data is overwritten when *exiting* the Patgen page. It is recommended to copy to another pattern first if you have data you want to keep.}

### Resetting a Generated Pattern

Each track is saved prior to any change while in the pattern generator. To restore all tracks to
their original state, press the SHIFT + PERF combination again twice. Note, that if track length was
adjusted, the tracks may be out of sync. Pattern re-alignment can be performed by selecting the pattern
twice while in PERF mode. 

## Global Settings Menu

### Accessing the Settings Menu

1. Press and hold shift.
2. Press Mode Button 4 (Load/Save).
3. A flashing mode 4 LED indicates the settings menu is active.

### Menu Options

\begin{paramtable}
bpm & Tempo in BPM    & Internal clock tempo. Set to 0 to sync to incoming MIDI clock. \\
qnt & Sequencer Quantisation & Sets the quantisation grid for recorded data. \\
mid & Global MIDI Channel & Sets the channel for global functions (see MIDI Table). \\
txf & Transmit MIDI Filter & Sets what MIDI commands are transmitted from the LXR. With C on and a global MIDI channel set, parameter changes made in the menu are sent as the CC or NRPN listed in the MIDI tables, on the global channel. \\
rxf & Receive MIDI Filter & Sets what MIDI commands are recognized by the LXR. Additional ¢ entries,
                           reachable only with the encoder, swap CC38 to NRPN 96
                           (see Receive MIDI Filter below). \\
mrt & MIDI Routing & Sets how MIDI is transmitted between the DIN jacks and USB. \\
fet & Parameter fetch & When active, you must `fetch' the displayed value with the knob
                        before it changes — prevents value jumps on page switches. \\
flw & Pattern follow  & When active, the viewed pattern always matches the playing pattern. \\
ssv & Screensaver     & Turns the screensaver on or off. \\
pcr & Bar Reset Mode & Sets whether bar count for repeat/chain resets on manual pattern changes. \\
pci & Pattern Change Instant & Pattern changes occur instantly instead of end of bar when on. \\
cki & Trigger in PPQ Time & Sets the PPQ time the LXR syncs to for the trigger expansion. \\
co1 & Trigger out PPQ 1 & Sets the timing output by the trigger expansion for clock 1.\\
co2 & Trigger out PPQ 2 & Sets the timing output by the trigger expansion for clock 2.\\
mod & Trigger Gate Mode & Sets gate out on/off when trigger expansion is installed.\\
b2p & Performance Bank Change & When enabled, MIDI Bank Change (CC0) loads a .prf file instead of a .kit.\\
stg & Shift Toggle & When enabled, SHIFT works as a toggle.\\
flb & File Background Load & Sets which files load in the background and queue change on next pattern.\\
itp & Waveform Interpolation & Toggles interpolating between main oscillator waveforms when modulated.\\
rol & MIDI Roll Offset & Semitone offset above each voice's MIDI trigger note at which incoming
                         notes play rolls instead of single hits. \texttt{off} disables MIDI roll
                         triggering (see MIDI Roll Offset below).\\
\end{paramtable}

### Follow Mode

When follow mode is active, all viewed pattern data (step LEDs, display values) updates
automatically on every pattern change. When off, the user selects the viewed pattern manually —
allowing editing of a pattern other than the one playing. The chaselight is only visible when the
playing pattern is viewed.

### Screensaver

The display is an OLED. To extend its lifetime and prevent burn-in, a screensaver activates after
a few minutes of inactivity. Any control touch dismisses it. The screensaver can be disabled in
the settings menu.

### MIDI Roll Offset

If the \param{rol} offset has a value, that value is applied to incoming MIDI Notes: Notes corresponding to a value above the voice's normal MIDI trigger by \param{rol} start a roll at the current roll settings defined in MODE PERF. 

- If the voice is on the Global Channel, the roll is triggered by the note at \param{rol} above the voice's assigned note, or above the voice default if no note is set.
- If the voice is assigned its own channel without a fixed note (chromatic mode), the offset works as a keyboard split: notes
  below it play the voice normally, and notes at or above it play rolls.

### Receive MIDI Filter

The \param{rxf} setting selects which incoming MIDI message types the LXR responds to. Each
letter enables one type:

| Letter | Message type |
|:---:|---|
| N | Notes |
| R | Realtime: clock, start, stop |
| C | Controllers: CC and NRPN |
| ¢ | Controllers, alternate 038 Mapping |
| P | Program change |


A ¢ entry works exactly like 'C', except:

- CC38 on the global MIDI channel is ignored.
- Voice 1 filter frequency, normally on global CC38, moves to NRPN 96.

Use a ¢ entry with gear that sends a CC38 (data entry LSB) after every NRPN value. Without it, each NRPN sent to the LXR also moves voice 1's filter frequency, usually closing it.

The ¢ entries may only be selected when the parameter is clicked with the encoder and is in single-parameter view. 

### Loading and Saving Global Settings

Settings are loaded automatically on power-up. To save current settings, go to the save menu,
select `Settings`, navigate to `Ok`, and push the encoder.
