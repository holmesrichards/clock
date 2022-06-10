# LED notes

On the breadboard, there is some modulation of the 12 V rail with clock pulsing. It goes away when the LEDs are removed. On the breadboard the beat LED is green and the rest are red, and the green LED has a decidedly larger effect than the reds. With green disconnected and red connected the effect is there but very small, hard to quantify in the noise. With green connected the modulation is about 10 mV.

RL = 470R for the green and 3.3k for the reds. Voltage drop across RL is 2.8 V for green and 3.2 V for red, so green is drawing 6 mA and red are drawing 1 mA each.

LEDs are Tayda A-261 and A-262 (3 mm, 5 mm with similar specs are A-1554 and A-1553).

Tayda also has A-057 Green Water Clear Super Bright, with higher Vf (3.0–3.4 V) and higher intensity (6500–7500 mCD). This with a large RL might be a better choice.

There also is A-706 Red Water Clear Ultra Bright, Vf = 2.0–2.4 V) and higher intensity (6000–7000 mCD).