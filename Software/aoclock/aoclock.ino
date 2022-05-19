/*
 * Clock module code
 * by Rich Holmes  5/2022
 * Heavily modified and extended from code by SynthMafia
 */

#include <SimpleTimer.h>
// Several "SimpleTimer" libraries exist, use this one:
// https://github.com/jfturcot/SimpleTimer

#define DEBUG 1  // Nonzero for debug prints to serial

# Hardware configuration

const int CLOCK_OUT = D2;
const int DIV2      = D3;
const int DIV4      = D4;
const int DIV8      = D5;
const int DIVN      = D6;
const int ENCA      = D7;
const int ENCB      = D8;
const int ENCPUSH   = D9;
const int TACT      = D10;
const int CLOCK_IN  = D11;

SimpleTimer timer;
int count = 0;

bool started = false;
bool tact_pushed = false;
int tap_millis;           // millis for latest tap
int tap_millis_prev = 0;  // millis for previous tap
int tap_time;             // time between taps, 0 if not 
int tempo_pot = 0;      // latest tempo pot reading
int tempo_pot_prev = 0; // previous tempo pot reading
int duty_pot = 0;       // duty cycle pot reading
int BPM = 0;
int BPM_tap = 0;        // next BPM from tact tap
int max_BPM = 500;
int min_BPM = 60;
int max_time = ((1/(min_BPM/60)) * 1000);
int min_time = ((1/(max_BPM/60)) * 1000);
long cycle_start = 0;
long cycle_stop = 0;

/*********************************************************************/

void cycle_off()
{
  digitalWrite (CLOCK_OUT, LOW);
  digitalWrite (DIV2, LOW);
  digitalWrite (DIV4, LOW);
  digitalWrite (DIV8, LOW);
  digitalWrite (DIVN, LOW);

  count++;

  if (count == 8)
    count = 0;
}



/*********************************************************************/


void cycle_on()
{
  digitalWrite (CLOCK_OUT, HIGH);
  if (count%2 == 0)
    {
      digitalWrite (DIV2, HIGH);
      if (count%4 == 0)
	{
	  digitalWrite (DIV4, HIGH);
	  if (count%4 == 0)
	    {
	      digitalWrite (DIV8, HIGH);
	    }
	}
    }

  timer.setTimeout (cycle_start, cycle_on);
  timer.setTimeout (cycle_stop, cycle_off);
}

/*********************************************************************/

void setup()
{
#if DEBUG  
  Serial.begin (9600);
#endif
  pinMode (CLOCK_OUT, OUTPUT);
  pinMode (DIV2, OUTPUT);
  pinMode (DIV4, OUTPUT);
  pinMode (DIV8, OUTPUT);
  pinMode (TACT, INPUT);    
}

void loop()
{  
  if (!started)
    {
      cycle_on();
      started = true;
    }
  
  timer.run();

  if (digitalRead (TACT) == LOW)
    if (tact_pushed)
      {
	// Tact not pushed but was pushed, let tap BPM take effect
	if (BPM_tact > 0)
	  BPM = BPM_tact;
	tact_pushed = false;
      }
    else
      {
	// Tact not pushed and was not pushed, check pots
	tempo_pot = analogRead (A0);
	duty_pot = analogRead (A1);

	// If the tempo pot moves, set BPM from that
	if ((tempo_pot_prev - tempo_pot > 5) ||
	    (tempo_pot_prev - tempo_pot < -5))
	  {
	    BPM = map (tempo_pot, 0, 1023, min_BPM, max_BPM);
	    BPM_tact = 0;
	  }
	tempo_pot_prev = tempo_pot;
  
	float duty_cycle =  map (duty_pot, 0, 1023, 1, 90) * 0.01;
      }
  else
    if (tact_pushed)
      {
	// Tact pushed and was pushed, do nothing
      }
    else
      {
	// Tact pushed and was not pushed, process tap
	tact_pushed = true;
	if (tap_millis_prev == 0)
	  // See a tap with no previous one, set tap_millis_prev
	  tap_millis_prev = millis ();
	else
	  {
	    // See a tap with a previous one, set BPM_tact
	    tap_millis = millis ();
	    tap_time = constrain (tap_millis - tap_millis_prev, min_time, max_time);
	    tap_millis_prev = tap_millis;
	    BPM_tact = (60000 / tap_time);
	  }
      }
  
  // No taps for a while, cancel tap processing
  if (millis() - tap_millis_prev > 4000)
    tap_millis_prev = 0; 

  // Set cycle start and stop times
  long cycletime = (60000/BPM);
  cycle_start = cycletime;
  cycle_stop = long(cycletime * duty_cycle);

#if DEBUG  
  Serial.print(" BPM: ");
  Serial.println(BPM);
#endif
}
