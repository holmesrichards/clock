# Clock notes

## Possible starting points

* Tik-Tak SM [https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded](https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded). Code needs work, timer stuff needs to move to top of loop.
    * [Adam M's version](https://lookmumnocomputer.discourse.group/t/my-build-progress/345/4271) adds an OLED for tempo display and uses a rotary encoder
* Aebi's metronome [https://create.arduino.cc/projecthub/Ryaebi/metronome-with-leds-tone-ace8ec](https://create.arduino.cc/projecthub/Ryaebi/metronome-with-leds-tone-ace8ec)
* csurgay's metronome [https://www.instructables.com/Arduino-Metronome/](https://www.instructables.com/Arduino-Metronome/). Code online is incomplete but you can see how setup goes

###Stuff to add:

* Start/stop button
* Ext clock in
* RE + OLED interface:
    * Tempo
    * Duty cycle
    * Division
* OLED to display state
* Variable division in addition to hard /2, /4, /8.
* Quantization: Restrict settings to MM standard values:

>    The most common arrangement of tempos on a Maelzel metronome begins with at 40 beats per minute and increases by 2s:

>    40 42 44 46 48 50 52 54 56 58 60

>    then by 3s: 63 66 69 72

>    then by 4s: 72 76 80 84 88 92 96 100 104 108 112 116 120

>    then by 6s: 126 132 138 144

>    then by 8s: 144 152 160 168 176 184 192 200 208.

    Extend this to 2*108, 2*112, 2*116... 2*208, 4*108... and 76/2, 72/2, 69/2, 66/2... 40/2, 76/4...
    Or in short: [63 66 69 72 76 80 84 88 92 96 100 104 108 112 116 120] times powers of 2.

* Capacitive pad for tap?


## Menus/controls

* Run mode:

    * (RE long push to enter set mode)
    * (RE to toggle start/stop)
    * Tempo  (left = x 1/2, right = x 2, RE = ±1 BPM if free mode or next/prev MM if MM mode, long left or right for 120 BPM)

* Set mode:

    * (RE push to move to next line)
    * (RE long push to enter run mode)
    * Lines:
        1. Width  (left = narrower, right = wider, RE = ±1%, long left or right for 50%).
        2. Division (left = x 1/2 (rounded), right = x 2, RE = ±1, long left or right for /2)
        3. Tempo mode  (left or RE or right to change between MM and Free)

"narrower" and "wider" are previous/next in sequence 3%, 6%, 12%, 25%, 50%, 75%, 88%, 94%, 97%. Min and max width are 3%, 97%. Min and max tempo TBD.


## Panel

* OLED
* RE
* 2 push buttons (left, right)
* Push or pad: Tap
* 6 jacks: Clock in, clock out, /2, /4, /8, /n

