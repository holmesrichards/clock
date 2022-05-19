# Clock notes

## Possible starting points

* Tik-Tak SM [https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded](https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded). Code needs work, timer stuff needs to move to top of loop.
    * [Adam M's version](https://lookmumnocomputer.discourse.group/t/my-build-progress/345/4271) adds an OLED for tempo display and uses a rotary encoder
* Aebi's metronome [https://create.arduino.cc/projecthub/Ryaebi/metronome-with-leds-tone-ace8ec](https://create.arduino.cc/projecthub/Ryaebi/metronome-with-leds-tone-ace8ec)
* csurgay's metronome [https://www.instructables.com/Arduino-Metronome/](https://www.instructables.com/Arduino-Metronome/). Code online is incomplete but you can see how setup goes

###Stuff to add:


* Ext clock in
* RE + OLED + tact button interface:
    * Start/stop
    * Tempo
    * Duty cycle
    * Division amount and offset
* OLED to display state
* Variable division (amount and offset) in addition to hard /2, /4, /8.
* Mode that restricts tempo settings to MM standard values.

The usual Maelzel metronome settings are:

|    |    |    |    |    |    |    |    |    |    |     |     |     |     |     |     |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
|    |    |    |    |    | 40 | 42 | 44 | 46 | 48 |  50 |  52 |  54 |  56 |  58 |  60 |
| 63 | 66 | 69 | 72 | 76 | 80 | 84 | 88 | 92 | 96 | 100 | 104 | 108 | 112 | 116 | 120 |
| 126 | 132 | 138 | 144 | 152 | 160 | 168 | 176 | 184 | 192 | 200 | 208 |     |     |     |     |

(Note each line is 2x the previous.)

    Extend this to [63 66 69 72 76 80 84 88 92 96 100 104 108 112 116 120] times powers of 2. Maybe from 7.875 to 960 (63/8 to 120*8).

## Menus/controls

* Run mode:

    * RE turn to set tempo, ±1 BPM if free mode or next/prev MM if MM mode
    * RE press to toggle start/stop
    * RE long press to change tempo mode (free or MM)
    * Tact press(es) to set tempo, constrained to integer if free mode or MM value if MM mode
    * Tact long press to enter set mode

* Set mode:

    * Tact long press to enter run mode
    * RE turn to move to next line, RE press to select
    * Lines — RE press to leave line function, tact long press to enter run mode:
        1. Width  (RE turn ±1%, tact press ±5%)
        2. Division amount (RE turn ±1, tact press ±4)
        3. Division offset (RE turn ±1, tact press ±4)

Switch to MM mode does not change tempo, but restricts what tempo can be changed to.

Offset 0 means output pulses occur on clock pulses 1 (start), 1+div amount, 1+2*div amount...

Nonzero offset means output pulses occur that many clock pulses later, on pulses 1+offset, 1+offset+div amount, 1+offset+2*div amount...

For instance /4 is pulses 1, 5, 9, 13, 17...; /4 with offset 3 is pulses 4, 8, 12, 16, 20...

Offset is set to 0 when division amount is changed, and can be set in range 0 to amount.

## Panel

* OLED
* RE
* Push or pad: Tap
* 6 jacks: Clock in, clock out, /2, /4, /8, /n+offset

