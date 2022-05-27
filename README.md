This is a work in progress; better README to come soon. Meanwhile:

**Untested hardware and software — Do not assume anything works!**

# Clock

This is a clock synth module in Kosmo format.

I took [Tik-Tak SM](https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded) as my starting point, but there's not much of it left — I changed and added a lot in both hardware and software.

Features are:

* Rotary encoder to set tempo and other parameters.
* Tap button for a different way to set tempo.
* OLED for display of tempo and settings menu.
* 5 outputs: Clock out, clock divided by 2, 4, and 8, and clock divided by arbitrary number (between 1 and 64) with offset.
* Two tempo submodes: INC (increment) mode, to set any whole number of beats per minute (BPM) from 8 to 928, and MM (Maelzel Metronome) mode, to more quickly set standard MM values and generalizations of these from 7.5 (= 60 ÷ 8) to 208 BPM.
* Clock pulses per beat (PPB) variable from 1 to 24.
* Clock pulse width (duty cycle) variable from 5% to 95%.
* Interrupt based timer and encoder codes for accuracy.

## Usage

### Pulses and beats

"Beat" is a musical term, referring to the fundamental unit of rhythm, with usually (not always) a quarter note duration corresponding to one beat. "Tempo" refers to how many beats occur per minute.

Since there are shorter notes than quarter notes, this clock can put out multiple — 4 or 8 or some other number — clock pulses per beat.

This module, instead of doing things in terms of clock speed in pulses per minute, uses the more music-oriented idea of tempo in beats per minute, along with clock pulses per beat.

### Run mode

The module starts up in **run mode**. Here the display shows the tempo in BPM along with PPB.

At the left, if the clock is running, there is a dot that flashes once per beat.

At the lower left is also shown the tempo submode, INC or MM; see below.

In this mode:

* Turning the encoder clockwise or counterclockwise raises or lowers the tempo.
* Tapping the tactile button sets the tempo to match the time between the last two taps.
* Short pressing the encoder stops or restarts the clock.
* A long encoder press switches between INC and MM submodes.
* A long tactile button press switches to **set mode**.

At startup, the tempo is 120 BPM and there are 4 PPB (so the clock speed is 480 pulses per minute).

#### Tempo submodes

In the INC tempo submode, any whole number of BPM from 8 to 208 may be set. When using the encoder, turning one step clockwise or counterclockwise steps to the next higher or lower whole number. When using the tap button, the tap interval is converted to the nearest whole number BPM.

In the MM submode, the tempo is constrained to a generalized version of the usual Maelzel metronome markings, which are

|    |    |    |    |    |    |    |    |    |    |     |     |     |     |     |     |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
|    |    |    |    |    |    | 40 | 42 | 44 | 46 | 48 |  50 |  52 |  54 |  56 |  58 |
| 60 | 63 | 66 | 69 | 72 | 76 | 80 | 84 | 88 | 92 | 96 | 100 | 104 | 108 | 112 | 116 |
| 120 | 126 | 132 | 138 | 144 | 152 | 160 | 168 | 176 | 184 | 192 | 200 | 208 |     |     |     |
|    |    |    |    |    |    |    |    |    |    |     |     |     |     |     |     |

Note each line is 2x the previous, and the steps from one value to the next increase from 2 to 3 to 4 to 6 to 8. Roughly speaking, the gaps between values are about 5% of the value (it's a bit like E48 resistor values); getting from a low value to a high value requires much less encoder turning than in INC mode. The generalized version extends this pattern:

|    |    |    |    |    |    |    |    |    |    |     |     |     |     |     |     |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
| 7.5 |7.875 | 8.25 | 8.625 | 9 | 9.5 | 10 | 10.5 | 11 | 11.5 | 12 | 12.5 | 13 | 13.5 | 14 | 14.5 |
| 15 |15.75 | 16.5 | 17.25 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 |
| 30 |31.5 | 33 | 34.5 | 36 | 38 | 40 | 42 | 44 | 46 | 48 | 50 | 52 | 54 | 56 | 58 |
| 60 | 63 | 66 | 69 | 72 | 76 | 80 | 84 | 88 | 92 | 96 | 100 | 104 | 108 | 112 | 116 |
| 120 | 126 | 132 | 138 | 144 | 152 | 160 | 168 | 176 | 184 | 192 | 200 | 208 |     |     |     |
|    |    |    |    |    |    |    |    |    |    |     |     |     |     |     |     |


In MM submode, when using the encoder, turning one step clockwise or counterclockwise steps to the next higher or lower number in the above list. When using the tap button, the tap interval is converted to the nearest MM value.

At startup, the module is in MM submode.

### Set mode

In this mode five menu lines are shown:

* /N amount
* /N offset
* Pulse/beat
* Width

A cursor on the left edge points to one of these lines. Turning the encoder moves the cursor up or down. Pressing the encoder moves the cursor to the right edge, indicating this line has been selected. Now turning the encoder cycles between available options for that line. Pressing the encoder again moves the cursor back to the left edge.

**/N amt** is the amount of clock division applied on the **Div N** output; there will be an output pulse once every *amt* clock pulses. Any number from 1 through 64 may be selected. At startup, *amt* is 16.

**/N off** is the offset applied to the **Div N** output; output pulses will occur *off* pulses later than they would if *off* were zero. For example, if *amt* is 4 and *off* is 0, output pulses will occur on the same clock pulses on both the **Div 4** and **Div N** outputs, but if *off* is changed to 1, output pulses will occur on the **Div N** output 1 clock pulse after they occur on the **Div 4** output. You can think of the **Div 4** pulses as occurring on the beat while the **Div N** pulses are off the beat. *off* can be set to any value from 0 through *amt*-1. At startup, *off* is 0, and when *amt* is changed, *off* is automatically reset to 0.

**Pulse/beat** is the number of clock pulses per musical beat, or PPB. Any number from 1 through 24 may be selected. At startup, PPB is 4.

**Width** represents the pulse width (duty cycle) of the clock, in percent. Turning the encoder increases or decreases the width in increments of 5%, in the range 5% to 95%. At startup, *width* is 50.

At any time in set mode, a long press on the tactile button switches to run mode.

## Current draw
 mA +12 V,  mA -12 V


## Photos

![]()

![]()

## Documentation

* [Schematic](Docs/.pdf)
* PCB layout: [front](Docs/_layout_front.pdf), [back](Docs/_layout_back.pdf)
* [BOM](Docs/_bom.md)
* [Build notes](Docs/build.md)

## Libraries

The Arduino software uses these libraries:

* TimerOne (from [https://github.com/PaulStoffregen/TimerOne](https://github.com/PaulStoffregen/TimerOne))
* U8g2lib  (from [https://github.com/olikraus/u8g2](https://github.com/olikraus/u8g2))
* DirectIO (from [https://github.com/mmarchetti/DirectIO.git](https://github.com/mmarchetti/DirectIO.git))
* assert (built into Arduino system)

The user must install the first three of these to use the software.

## GitHub repository

* [https://github.com/holmesrichards/clock](https://github.com/holmesrichards/clock)

## Submodules

This repo uses submodules aoKicad and Kosmo_panel, which provide needed libaries for KiCad. To clone:

```
git clone git@github.com:holmesrichards/clock.git
git submodule init
git submodule update
```


Alternatively do

```
git clone --recurse-submodules git@github.com:holmesrichards/clock.git
```

Or if you download the repository as a zip file, you must also click on the "aoKicad" and "Kosmo\_panel" links on the GitHub page (they'll have "@ something" after them) and download them as separate zip files which you can unzip into this repo's aoKicad and Kosmo\_panel directories.

If desired, copy the files from aoKicad and Kosmo\_panel to wherever you prefer (your KiCad user library directory, for instance, if you have one). Then in KiCad, go into Edit Symbols and add symbol libraries 

```
aoKicad/ao_symbols
Kosmo_panel/Kosmo
```
and go into Edit Footprints and add footprint libraries 
```
aoKicad/ao_tht
Kosmo_panel/Kosmo_panel.
```
