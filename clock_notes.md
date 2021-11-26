# Clock notes

## Possible starting points

* Tik-Tak SM [https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded](https://create.arduino.cc/projecthub/Synthemafia/modular-synth-clock-module-diy-arduino-sm-tik-tak-bd8ded). Code needs work, timer stuff needs to move to top of loop.
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

>    Make this vs. free selection a compile option? Make them separate interface modes?

* Capacitive pad for tap?

## Panel

OLED
Pot/RE
Switch: Tempo/Duty/Div or use RE push
Push or toggle: Start/stop
Push or pad: Tap
6 jacks: Clock in, clock out, /2, /4, /8, /n
