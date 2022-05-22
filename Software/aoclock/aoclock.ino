/*
 * Clock module code
 * by Rich Holmes  5/2022
 * Heavily modified and extended from code by SynthMafia
 */

#include <TimerOne.h> // Interrupt timer, from https://github.com/PaulStoffregen/TimerOne
#include <U8g2lib.h>  // Drawing to OLED, from https://github.com/olikraus/u8g2
#include <assert.h> // Builtin

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

int count = 0; // count of clock pulses

// State of things
bool running;  // clock is running?
bool set_mode; // menu mode for settings vs run mode for speed control
bool MMmode;   // Maelzel Metronome vs ANY submode
bool tact_pushed = false;
long tact_push_millis = -1;
bool tact_push_handled = false;
bool enc_pushed = false;
long enc_push_millis = -1;
bool enc_push_handled = false;
long tap_millis_prev = -1;  // millis for previous tap

// Tempo and duty cycle parameters
float BPM = 120.0;
float max_BPM = 960.0;
float min_BPM = 7.875;
int max_time = 60000 / min_BPM;
int min_time = 60000 / max_BPM;
int duty_cycle = 50;  // in percent
int min_duty = 5;
int max_duty = 95;
long cycle_start = 0;
long cycle_stop = 0;

// Variable division handling
int Ndiv = 16;    // variable division amount
int Noff = 0;    // variable division offset

// External clock handling
bool ext_clock = false;  // external or internal clock
bool ec_on = false; // external clock state

// Encoder handling
int enc_a_prev;

// For the timer interrupt
long millicount = 0; // millisecond counter
long time1 = -1;  // time to turn off clock pulse
long time2 = -1;  // time to turn on again

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8;  // object for OLED control
const int AMT_ROW = 2;
const int OFF_ROW = 3;
const int CLK_ROW = 4;
const int WID_ROW = 5;
// Initial cursor position
int curs_col = 0;
int curs_row = AMT_ROW;

int MM[18] = {60, 63, 66, 69, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 126};   // Maelzel tempos

/*********************************************************************/

void cycle_off()
{
  if (running)
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
}

/*********************************************************************/

void cycle_on()
{
  if (running)
    {
      digitalWrite (CLOCK_OUT, HIGH);
      if (count%2 == 0)
	{
	  digitalWrite (DIV2, HIGH);
	  if (count%4 == 0)
	    {
	      digitalWrite (DIV4, HIGH);
	      if (count%8 == 0)
		digitalWrite (DIV8, HIGH);
	    }
	}
      if (count%Ndiv == Noff)
	digitalWrite (DIVN, HIGH);
    }
}

/*********************************************************************/

void timerStuff()
{
  // Called by interrupt handler
  if (ext_clock)
    return;
  
  if (running)
    {
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
}

/*********************************************************************/

int encoder()
{
  // Return +1 for clockwise click, -1 for CCW, 0 for none
  
  int delta = 0;
  int enc_a = digitalRead(ENCA);
  int enc_b = digitalRead(ENCB);
  if (enc_a == LOW && enc_a_prev == HIGH)
    delta = enc_b == enc_a ? -1 : 1;
  enc_a_prev = enc_a;

  return delta;
} 

/*********************************************************************/

float MMdir (float was, int dir)
{
  // Return nearest or next higher or lower (if dir is 0 or +1 or -1) MM tempo
  // compared to was

  // Find power of 2 p2 such that was is in [MM[1]*p2, MM[17]*p2)
  float p2 = 1;
  assert (was > 0);
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

void oled_display_set()
{
  // OLED display in set mode
  
  char buf[17];
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (0, 0, "                ");      
  u8x8.drawString (0, 1, "                ");      
  u8x8.drawString (0, WID_ROW, ext_clock ? "                " : " Width          ");      
  u8x8.drawString (0, AMT_ROW, " /N amt         ");      
  u8x8.drawString (0, OFF_ROW, " /N off         ");      
  u8x8.drawString (0, CLK_ROW, " Clock          ");      
  u8x8.drawString (0, 6, "                ");      
  u8x8.drawString (0, 7, "                ");
  String os = ext_clock ? String (" ") : String (duty_cycle) + String ("%");
  int los = os.length();
  os.toCharArray (buf, los+1);
  u8x8.drawString (15-los, WID_ROW, buf);
  os = String (Ndiv);
  los = os.length();
  os.toCharArray (buf, los+1);
  u8x8.drawString (15-los, AMT_ROW, buf);
  os = String (Noff);
  los = os.length();
  os.toCharArray (buf, los+1);
  u8x8.drawString (15-los, OFF_ROW, buf);
  u8x8.drawString (12, CLK_ROW, ext_clock ? "EXT" : "INT");	  
  u8x8.drawString (curs_col, curs_row, curs_col == 0 ? ">" : "<");
}

/*********************************************************************/

void oled_display_run()
{
  // OLED display in run mode

  char buf[17];
  if (ext_clock || !running)
    {
      u8x8.setFont(u8x8_font_victoriamedium8_r);
      u8x8.drawString (0, 0, "                ");
      u8x8.drawString (0, 1, "                ");
      u8x8.drawString (0, 5, "                ");
      u8x8.drawString (0, 6, "                ");
      u8x8.drawString (0, 7, "                ");
      u8x8.setFont(u8x8_font_profont29_2x3_r);
      u8x8.drawString (0, 2, ext_clock ? "EXTERNAL" : " STOPPED");
    }
  else
    {
      u8x8.setFont(u8x8_font_profont29_2x3_r);
      u8x8.drawString (0, 0, "                ");

      // Right justify BPM with only as many decimal places as needed
      float ff = BPM;
      int i = 0;
      for (; i < 4; ++i)
	{
	  if (ff == int(ff))
	    break;
	  ff *= 10;
	}
      String s = String (BPM, i);
      while (s.length() < 8)
	s = String(" ") + s;
      s.toCharArray (buf, 9);      

      u8x8.drawString (0, 1, buf);
      u8x8.drawString (0, 4, "     BPM");
      u8x8.setFont(u8x8_font_victoriamedium8_r);
      u8x8.drawString (0, 7, MMmode ? "MM " : "ANY");
    }     
}

/*********************************************************************/

void stop_it()
{
  digitalWrite (CLOCK_OUT, LOW);
  digitalWrite (DIV2, LOW);
  digitalWrite (DIV4, LOW);
  digitalWrite (DIV8, LOW);
  digitalWrite (DIVN, LOW);
}

/*********************************************************************/

void start_it()
{
  time1 = -1;
  time2 = -1;
  count = 0;
  cycle_on();
}

/*********************************************************************/

void set_mode_handler (int dre, int drt)
{
  // Set mode interface ==========================================
      
  if (!enc_pushed && dre == HIGH)
    {
      // Set mode: Encoder has been newly pushed ====================
      enc_pushed = true;
      enc_push_handled = false;
      return;
    }
  else if (enc_pushed && dre == LOW)
    {
      // Set mode: Encoder push is over ====================
      enc_pushed = false;
      curs_col = 15-curs_col;
      oled_display_set();
      return;
    }
  else if (!tact_pushed && drt == HIGH)
    {
      // Set mode: Tact has been newly pushed ====================
      tact_pushed = true;
      tact_push_millis = millis();
      tact_push_handled = false;
      return;
    }
  else if (tact_pushed && !tact_push_handled 
	   && drt == HIGH && millis() - tact_push_millis >= 500)
    {
      // Set mode: Unhandled long tact press in progress ====================
      set_mode = false;
      oled_display_run();
      tact_push_handled = true;
      return;
    }
  else if (tact_pushed && drt == LOW)
    {
      // Set mode: Tact push is over ====================
      tact_pushed = false;
      if (millis() - tact_push_millis >= 500 && !tact_push_handled)
	{
	  set_mode = false;
	  oled_display_run();
	}
      return;
    }

  // No push in progress or pending  ====================
  // Check encoder rotation
      
  int delta = encoder();
  if (delta != 0)
    {
      if (curs_col == 0)
	// Go to next/previous row
	curs_row = constrain (curs_row+delta, 2, 5);
      else if (!ext_clock && curs_row == WID_ROW)
	// Change pulse width
	duty_cycle = constrain (duty_cycle + 5*delta, min_duty, max_duty);
      else if (curs_row == AMT_ROW)
	{
	  // Change division amount
	  Ndiv = constrain (Ndiv + delta, 1, 64);
	  Noff = 0;
	}
      else if (curs_row == OFF_ROW)
	// Change division offset
	Noff = (Noff + delta + Ndiv) % Ndiv;
      else if (curs_row == CLK_ROW)
	// Toggle external clock
	if (ext_clock)
	  {
	    ext_clock = false;
	    start_it();
	  }		
	else
	  {
	    stop_it();		    
	    ext_clock = true;
	  }
      oled_display_set();
    }
}

/*********************************************************************/

void run_mode_handler (int dre, int drt)
{
  // Run mode interface ==========================================
  if (!ext_clock && !enc_pushed && dre == HIGH)
    {
      // Run mode: Encoder has been newly pushed ====================
      enc_pushed = true;
      enc_push_millis = millis();
      enc_push_handled = false;
      return;
    }
  else if (running && !ext_clock && enc_pushed && !enc_push_handled 
	   && dre == HIGH && millis() - enc_push_millis >= 500)
    {
      // Run mode: Unhandled long encoder press in progress ====================
      MMmode = !MMmode;
      oled_display_run();
      enc_push_handled = true;
      return;
    }
  else if (!ext_clock && enc_pushed && dre == LOW)
    {
      // Run mode: Encoder push is over ====================
      enc_pushed = false;
      if (millis() - enc_push_millis < 500)
	{
	  // Short press, toggle running
	  if (running)
	    {
	      stop_it();
	      running = false;
	    }
	  else
	    {
	      running = true;
	      start_it();
	    }
	}
      else
	if (!enc_push_handled)
	  MMmode = !MMmode;
	  
      oled_display_run(); 
      return;
   }
  else if (!tact_pushed && drt == HIGH)
    {
      // Run mode: Tact has been newly pushed ====================
      tact_pushed = true;
      tact_push_millis = millis();
      tact_push_handled = false;
      return;
    }
  else if (tact_pushed && !tact_push_handled 
	   && drt == HIGH && millis() - tact_push_millis >= 500)
    {
      // Run mode: Unhandled long tact press in progress ====================
      set_mode = true;
      curs_col = 0;
      curs_row = 2;	  
      oled_display_set();
      tact_push_handled = true;
      return;
    }
  else if (tact_pushed && drt == LOW)
    {
      // Run mode: Tact push is over ====================
      tact_pushed = false;
      if (running && !ext_clock && millis() - tact_push_millis < 500)
	{
	  if (tap_millis_prev == -1)
	    // See a tap with no previous one, set tap_millis_prev
	    {
	      tap_millis_prev = millis ();
	    }
	  else
	    {
	      // See a tap with a previous one, set BPM
	      long tap_millis = millis ();
	      long tap_time = constrain (tap_millis - tap_millis_prev, min_time, max_time);
	      tap_millis_prev = tap_millis;
	      if (MMmode)
		BPM = constrain (MMdir (60000.0 / tap_time, 0), min_BPM, max_BPM);
	      else
		BPM = int (constrain (60000.0 / tap_time, min_BPM, max_BPM) + 0.5);
	      oled_display_run();
	    }
	}
      else if (tact_push_millis >= 500 && !tact_push_handled)
	{
	  set_mode = true;
	  curs_col = 0;
	  curs_row = 2;	  
	  oled_display_set();
	}
      return;
    }

  // No push in progress or pending  ====================
  // Check encoder rotation
      
  int delta = encoder();
  if (delta != 0)
    {
      if (MMmode)
	BPM = constrain (MMdir (BPM, delta), min_BPM, max_BPM);
      else
	BPM = constrain (int(BPM+delta+0.5), int(min_BPM), int(max_BPM));
      oled_display_run();
    }
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
  pinMode (ENCA, INPUT);
  pinMode (ENCB, INPUT);
  pinMode (ENCPUSH, INPUT);
  pinMode (CLOCK_IN, INPUT);
  enc_a_prev = digitalRead(ENCA);

  MMmode = true;
  
  //OLED
  u8x8.begin();
  
  // Timer interrupt
  running = true;
  Timer1.initialize(1000); // interrupt every 1 ms
  Timer1.attachInterrupt(timerStuff);

  set_mode = false;
  oled_display_run();
}

/*********************************************************************/

void loop()
{
  static bool started = false;
  if (!started)
    {
      long cycletime = (60000/BPM);
      cycle_start = cycletime;
      cycle_stop = long(cycletime * duty_cycle * 0.01);

      cycle_on();

      started = true;
    }

  if (ext_clock)
    {
      int new_ec = digitalRead (CLOCK_IN) == HIGH;
      if (new_ec and !ec_on)
	cycle_on();
      else if (!new_ec and ec_on)
	cycle_off();
      ec_on = new_ec;
    }

  int dre = digitalRead (ENCPUSH);
  int drt = digitalRead (TACT);
  if (set_mode)
    set_mode_handler (dre, drt);
  else
    run_mode_handler (dre, drt);
      
  // No taps for a while, cancel tap processing
  if (millis() - tap_millis_prev > 4000)
    tap_millis_prev = -1; 
  
  // Set cycle start and stop times
  long cycletime = 60000/BPM;
  cycle_start = cycletime;
  cycle_stop = long(cycletime * duty_cycle * 0.01);
}