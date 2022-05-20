/*
 * Clock module code
 * by Rich Holmes  5/2022
 * Heavily modified and extended from code by SynthMafia
 */

#include <TimerOne.h>
// From https://github.com/PaulStoffregen/TimerOne
#include <assert.h>

#define DEBUG 1  // Nonzero for debug prints to serial

// Hardware configuration

const int CLOCK_OUT = 2;
const int DIV2      = 3;
const int DIV4      = 4;
const int DIV8      = 5;
const int DIVN      = 6;
const int ENCA      = 7;
const int ENCB      = 8;
const int ENCPUSH   = 9;
const int TACT      = 10;
const int CLOCK_IN  = 11;

int count = 0;

bool started = false;
bool tact_pushed = false;
long tap_millis;           // millis for latest tap
long tap_millis_prev = -1;  // millis for previous tap
int tap_time;             // time between taps
int tempo_pot = 0;      // latest tempo pot reading
int tempo_pot_prev = 0; // previous tempo pot reading
int duty_pot = 0;       // duty cycle pot reading
float BPM = 120.0;
float BPM_tact = 0.0;        // next BPM from tact tap
float max_BPM = 960.0;
float min_BPM = 7.875;
int max_time = ((1/(min_BPM/60)) * 1000);
int min_time = ((1/(max_BPM/60)) * 1000);
float duty_cycle = 0.3;
long cycle_start = 0;
long cycle_stop = 0;

int Ndiv = 7;    // variable division amount

// Encoder handling
int enc_a_prev;
int enc_a;
int enc_b;

// For the timer interrupt
long millicount = 0; // millisecond counter
long time1 = 0;  // time to turn off
long time2 = 0;  // time to turn on again


int MM[18] = {60, 63, 66, 69, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 126};   // Maelzel tempos
bool MMmode = true;

/*********************************************************************/

void cycle_off()
{
  digitalWrite (CLOCK_OUT, LOW);
  digitalWrite (DIV2, LOW);
  digitalWrite (DIV4, LOW);
  digitalWrite (DIV8, LOW);
  digitalWrite (DIVN, LOW);

  count++;

  if (count == 8*Ndiv)
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
	  if (count%8 == 0)
	    {
	      digitalWrite (DIV8, HIGH);
	    }
	}
    }
  if (count%Ndiv == 0)
    {
      digitalWrite (DIVN, HIGH);
    }
}

/*********************************************************************/

void timerStuff()
{
  // Called by interrupt handler
  millicount++;
  if (time1 > 0 && millicount >= time1)
    {
      cycle_off();
      time1 = -1;
    }
  if (millicount >= time2)
    {
      cycle_on();
      if (millicount > 2147483647L - cycle_start)
        millicount = millicount % (8*Ndiv); // prevent overflow, preserve divisions
      time1 = millicount + cycle_stop;
      time2 = millicount + cycle_start;
    }
}

/*********************************************************************/

int encoder()
{
  // Return +1 for clockwise click, -1 for CCW, 0 for none
  
  int delta = 0;
  enc_a = digitalRead(ENCA);
  enc_b = digitalRead(ENCB);
  if (enc_a != enc_a_prev)
    {
      if (enc_b != enc_a)
	{
	  // Means pin A Changed first - we're Rotating Clockwise
	  if (enc_a == LOW)
	    delta = 1;
	}
      else
	{
	  // Otherwise B changed first and we're moving CCW
	  if (enc_a == LOW)
	    delta = -1;
	}
    }
  enc_a_prev = enc_a;

  return delta;
} 

/*********************************************************************/

float MMdir (float was, int dir)
{
  // Return nearest or next higher or lower (if dir is 0 or +1 or -1) MM tempo
  // compared to was

  assert (was > 0);

  // Find power of 2 p2 such that was is in [MM[1]*p2, MM[17]*p2)
  float p2 = 1;
  while (was < MM[1]*p2)
    p2 /= 2.0;
  while (was >= MM[17]*p2)
    p2 *= 2.0;

  // Find i such that was is in [MM[i]*p2, MM[i+1]*p2)
  float wasf = was / p2;
  int i = 1;
  for (; i < 16; ++i)
    if (wasf < MM[i+1])
      break;

  if (dir == 0)
    {
      if (wasf-MM[i] < MM[i+1]-wasf)
	return MM[i]*p2;
      else
	return MM[i+1]*p2;
    }
  if (dir == -1)
    {
      if (wasf == MM[i])
	return MM[i-1]*p2;
      else
	return MM[i]*p2;
    }
  if (dir == 1)
    return MM[i+1]*p2;
}

/*********************************************************************/

void setup()
{
#if DEBUG  
  Serial.begin (9600);
  Serial.println ("setup");
#endif
  pinMode (CLOCK_OUT, OUTPUT);
  pinMode (DIV2, OUTPUT);
  pinMode (DIV4, OUTPUT);
  pinMode (DIV8, OUTPUT);
  pinMode (DIVN, OUTPUT);
  pinMode (TACT, INPUT);    
  pinMode (ENCA,INPUT);
  pinMode (ENCB,INPUT);
  enc_a_prev = digitalRead(ENCA);

  // Timer interrupt
  Timer1.initialize(1000); // interrupt every 1 ms
  Timer1.attachInterrupt(timerStuff);
}

/*********************************************************************/

void loop()
{
  if (!started)
    {
      // check pots
      duty_pot = analogRead (A1);
      BPM_tact = 0.0;
      duty_cycle =  map (duty_pot, 0, 1023, 1, 90) * 0.01;
      long cycletime = (60000/BPM);
      cycle_start = cycletime;
      cycle_stop = long(cycletime * duty_cycle);

      cycle_on();

      started = true;
    }
  
  if (digitalRead (TACT) == LOW)
    {
      if (tact_pushed)
	{
	  // Tact not being pushed but was pushed, let tap BPM take effect
	  if (BPM_tact > 0.0)
	    BPM = BPM_tact;
	  tact_pushed = false;
	}
      else
	{
	  // Tact not being pushed and was not pushed, do nothing
	}

      // check pots
      duty_pot = analogRead (A1);
      
      // If the encoder moves, set BPM from that
      int delta = encoder();
      if (delta != 0)
	{
	  if (MMmode)
	    BPM = constrain (MMdir (BPM, delta), min_BPM, max_BPM);
	  else
	    BPM = int(constrain (BPM+delta, min_BPM, max_BPM));
	  BPM_tact = 0.0;
#if DEBUG
	  Serial.print ("BPM = ");
	  Serial.println (BPM);
#endif
	}

      duty_cycle =  map (duty_pot, 0, 1023, 1, 90) * 0.01;
    }
  else
    if (tact_pushed)
      {
	// Tact being pushed and was pushed, do nothing
      }
    else
      {
	// Tact being pushed and was not pushed, process tap
	tact_pushed = true;
	if (tap_millis_prev == -1)
	  // See a tap with no previous one, set tap_millis_prev
	  {
	    tap_millis_prev = millis ();
#if DEBUG
	    Serial.print ("First tap, millis ");
	    Serial.println (tap_millis_prev);
#endif
	  }
	else
	  {
	    // See a tap with a previous one, set BPM_tact
	    tap_millis = millis ();
	    tap_time = constrain (tap_millis - tap_millis_prev, min_time, max_time);
	    tap_millis_prev = tap_millis;
	    if (MMmode)
	      BPM_tact = constrain (MMdir (60000.0 / tap_time, 0), min_BPM, max_BPM);
	    else
	      BPM_tact = int (constrain (60000.0 / tap_time, min_BPM, max_BPM));
#if DEBUG
	    Serial.print ("Nonfirst tap, BPM ");
	    Serial.println (BPM_tact);
#endif
	  }
      }
  
  // No taps for a while, cancel tap processing
  if (millis() - tap_millis_prev > 4000)
    tap_millis_prev = -1; 

  // Set cycle start and stop times
  long cycletime = (60000/BPM);
  cycle_start = cycletime;
  cycle_stop = long(cycletime * duty_cycle);
}
