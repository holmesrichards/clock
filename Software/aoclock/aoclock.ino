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
bool MMmode;   // Maelzel Metronome vs INT submode
bool tact_pushed = false;
unsigned long tact_push_millis;
bool tact_push_handled = false;
bool enc_pushed = false;
unsigned long enc_push_millis;
bool enc_push_handled = false;
unsigned long tap_millis_prev;  // millis for previous tap
bool tap_millis_prev_set = false;   // true if tap_millis_prev is valid

// Tempo and duty cycle parameters
float BPM = 120.0;     // Beats per minute
float max_BPM = 208.0;
float min_BPM = 7.5;
int PPB = 4;         // pulses per beat
int max_time = 60000 / min_BPM;
int min_time = 60000 / max_BPM;
int duty_cycle = 50;  // in percent
int min_duty = 5;
int max_duty = 95;
unsigned long period = 0; // period in microseconds
unsigned long ontime = 0; // on time in microseconds

// Variable division handling
int Ndiv = 16;    // variable division amount
int Noff = 0;    // variable division offset

// External clock handling
bool ext_clock = false;  // external or internal clock
bool ec_on = false; // external clock state

// Encoder handling
int enc_a_prev;

// For the timer interrupt
unsigned long tickcount;  // interrupt counter
unsigned long cycle_start_time;  // time (in microsec) when this cycle started
bool clock_state;              // true when clock pulse high

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8;  // object for OLED control
const int AMT_ROW = 1;
const int OFF_ROW = 2;
const int CLK_ROW = 3;
const int WID_ROW = 4;
const int PPB_ROW = 5;
// Initial cursor position
int curs_col = 0;
int curs_row = AMT_ROW;

int MM[18] = {58, 60, 63, 66, 69, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120};   // Maelzel tempos

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
  // Called by interrupt handler once every 100 us
  
  if (ext_clock)
    return;
  
  if (running)
    {
      tickcount += 100; 
      if (clock_state and tickcount - cycle_start_time >= ontime) // should be rollover safe
	{
	  cycle_off();
	  clock_state = false;
	}
      if (tickcount - cycle_start_time >= period)
	{
	  cycle_on();
	  clock_state = true;
	  cycle_start_time += period;
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

void oled_display_set_wid()
{
  char buf[5];
  String os = String (duty_cycle) + String ("%");
  while (os.length() < 4)
    os = String (" ") + os;
  os.toCharArray (buf, 5);
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (11, WID_ROW, buf);
}

/*********************************************************************/

void oled_display_set_amt()
{
  char buf[5];
  String os = String (Ndiv);
  while (os.length() < 4)
    os = String (" ") + os;
  os.toCharArray (buf, 5);
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (11, AMT_ROW, buf);
}
  
/*********************************************************************/

void oled_display_set_off()
{
  char buf[5];
  String os = String (Noff);
  while (os.length() < 4)
    os = String (" ") + os;
  os.toCharArray (buf, 5);
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (11, OFF_ROW, buf);
}

/*********************************************************************/

void oled_display_set_clk()
{
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (12, CLK_ROW, ext_clock ? "EXT" : "INT");	  
}

/*********************************************************************/

void oled_display_set_ppb()
{
  char buf[3];
  String os = String (PPB);
  while (os.length() < 2)
    os = String (" ") + os;
  os.toCharArray (buf, 3);
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (13, PPB_ROW, buf);
}

/*********************************************************************/

void oled_display_set_clear_curs()
{
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (curs_col, curs_row, " ");
}

/*********************************************************************/

void oled_display_set_set_curs()
{  
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (curs_col, curs_row, curs_col == 0 ? ">" : "<");
}

/*********************************************************************/

void oled_display_set()
{
  // OLED display in set mode
  
  char buf[17];
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (0, 0, "                ");      
  u8x8.drawString (0, AMT_ROW, " /N amount      ");      
  u8x8.drawString (0, OFF_ROW, " /N offset      ");      
  u8x8.drawString (0, CLK_ROW, " Clock          ");      
  u8x8.drawString (0, WID_ROW, ext_clock ? "                " : " Width          ");      
  u8x8.drawString (0, PPB_ROW, " Pulse/beat     ");      
  u8x8.drawString (0, 6, "                ");      
  u8x8.drawString (0, 7, "                ");
  oled_display_set_set_curs();
  oled_display_set_amt();
  oled_display_set_off();
  oled_display_set_clk();
  oled_display_set_wid();
  oled_display_set_ppb();
}

/*********************************************************************/

void oled_display_run_bpm()
{
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
  char buf[9];
  s.toCharArray (buf, 9);      
  
  u8x8.setFont(u8x8_font_profont29_2x3_r);
  u8x8.drawString (0, 1, buf);
  u8x8.drawString (0, 4, "     BPM");
}

/*********************************************************************/

void oled_display_run_submode()
{
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (0, 7, MMmode ? "MM " : "INT");
}

/*********************************************************************/

void oled_display_run_ppb()
{
  char buf[7];
  String os = String (PPB) + String(" PPB");
  while (os.length() < 6)
    os = String (" ") + os;
  os.toCharArray (buf, 7);
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (10, 7, buf);
}

/*********************************************************************/

void oled_display_run_ticks()
{
  // for debugging, display tickcount
  char buf[16];
  unsigned long d = 0 - tickcount;
  String os = String (d);
  os.toCharArray (buf, 16);
  u8x8.setFont(u8x8_font_victoriamedium8_r);
  u8x8.drawString (0, 0, buf);
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
      oled_display_run_bpm();
      oled_display_run_ppb();
      oled_display_run_submode();
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
  count = 0;
  cycle_on();
  clock_state = true;
  cycle_start_time = tickcount;
}

/*********************************************************************/

void set_mode_handler (int dre, int drt)
{
  // Set mode interface ==========================================

  unsigned long mli = millis();
  unsigned long tpmdif = mli - tact_push_millis; // should be rollover safe
  
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
      oled_display_set_clear_curs();
      curs_col = 15-curs_col;
      oled_display_set_set_curs();
      return;
    }
  else if (!tact_pushed && drt == HIGH)
    {
      // Set mode: Tact has been newly pushed ====================
      tact_pushed = true;
      tact_push_millis = mli;
      tact_push_handled = false;
      return;
    }
  else if (tact_pushed && !tact_push_handled && drt == HIGH && tpmdif >= 500)
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
      if (tpmdif >= 500 && !tact_push_handled)
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
	{
	  oled_display_set_clear_curs();
	  curs_row = constrain (curs_row+delta, AMT_ROW, PPB_ROW);
	  oled_display_set_set_curs();
	  return;
	}
      else if (curs_row == AMT_ROW)
	{
	  // Change division amount	  
	  Ndiv = constrain (Ndiv + delta, 1, 64);
	  Noff = 0;
	  oled_display_set_amt();
	  oled_display_set_off();
	  return;
	}
      else if (curs_row == OFF_ROW)
	// Change division offset
	{
	  Noff = (Noff + delta + Ndiv) % Ndiv;
	  oled_display_set_off();
	  return;
	}
      else if (curs_row == CLK_ROW)
	// Toggle external clock
	{
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
	  oled_display_set_clk();
	  return;
	}
      else if (!ext_clock && curs_row == WID_ROW)
	// Change pulse width
	{
	  duty_cycle = constrain (duty_cycle + 5*delta, min_duty, max_duty);
	  oled_display_set_wid();
	  return;
	}
      else if (!ext_clock && curs_row == PPB_ROW)
	// Change pulses per beat
	{
	  PPB = (PPB + delta + 23) % 24 + 1;
	  oled_display_set_ppb();
	  return;
	}
    }
}

/*********************************************************************/

void run_mode_handler (int dre, int drt)
{
  // Run mode interface ==========================================

  unsigned long mli = millis();
  unsigned long epmdif = mli - enc_push_millis;
  unsigned long tpmdif = mli - tact_push_millis;

  if (!ext_clock && !enc_pushed && dre == HIGH)
    {
      // Run mode: Encoder has been newly pushed ====================
      enc_pushed = true;
      enc_push_millis = mli;
      enc_push_handled = false;
      return;
    }
  else if (running && !ext_clock && enc_pushed && !enc_push_handled 
	   && dre == HIGH && epmdif >= 500)
    {
      // Run mode: Unhandled long encoder press in progress ====================
      MMmode = !MMmode;
      oled_display_run_submode();
      enc_push_handled = true;
      return;
    }
  else if (!ext_clock && enc_pushed && dre == LOW)
    {
      // Run mode: Encoder push is over ====================
      enc_pushed = false;
      if (epmdif < 500)
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
	  oled_display_run();
	  return;
	}
      else
	if (!enc_push_handled)
	  {
	    MMmode = !MMmode;
	    oled_display_run_submode(); 
	    return;
	  }
   }
  else if (!tact_pushed && drt == HIGH)
    {
      // Run mode: Tact has been newly pushed ====================
      tact_pushed = true;
      tact_push_millis = mli;
      tact_push_handled = false;
      return;
    }
  else if (tact_pushed && !tact_push_handled && drt == HIGH && tpmdif >= 500)
    {
      // Run mode: Unhandled long tact press in progress ====================
      set_mode = true;
      curs_col = 0;
      curs_row = AMT_ROW;	  
      oled_display_set();
      tact_push_handled = true;
      return;
    }
  else if (tact_pushed && drt == LOW)
    {
      // Run mode: Tact push is over ====================
      tact_pushed = false;
      if (running && !ext_clock && tpmdif < 500)
	{
	  if (tap_millis_prev_set)
	    {
	      unsigned long tmpdif = mli - tap_millis_prev;
	      if (MMmode)
		BPM = constrain (MMdir (60000.0 / tmpdif, 0), min_BPM, max_BPM);
	      else
		BPM = int(constrain (60000.0 / tmpdif, min_BPM, max_BPM) + 0.5);
	      oled_display_run_bpm();
	    }
	  tap_millis_prev = mli;
	  tap_millis_prev_set = true;
	  return;
	}
      else if (tpmdif && !tact_push_handled)
	{
	  set_mode = true;
	  curs_col = 0;
	  curs_row = AMT_ROW;
	  oled_display_set();
	  return;
	}
    }

  // No push in progress or pending  ====================
  // Check encoder rotation
      
  int delta = encoder();
  if (delta != 0)
    {
      if (MMmode)
	BPM = constrain (MMdir (BPM, delta), min_BPM, max_BPM);
      else
	BPM = int (constrain (BPM+delta, min_BPM, max_BPM)+0.5);
      oled_display_run_bpm();
      return;
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
  Timer1.initialize(100); // interrupt every 1 ms
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
      period = (60000000./BPM/PPB);  // period in usec
      ontime = period * duty_cycle * 0.01;
      start_it();
      started = true;
    }

  if (ext_clock)
    {
      int new_ec = digitalRead (CLOCK_IN) == HIGH;
      if (new_ec and !ec_on)
	{
	  cycle_on();
	  clock_state = true;
	}
      else if (!new_ec and ec_on)
	{
	  cycle_off();
	  clock_state = false;
	}
      ec_on = new_ec;
    }

  int dre = digitalRead (ENCPUSH);
  int drt = digitalRead (TACT);
  if (set_mode)
    set_mode_handler (dre, drt);
  else
    run_mode_handler (dre, drt);

  /* if (!set_mode) */
  /*   oled_display_run_ticks(); */
      
  // No taps for a while, cancel tap processing
  unsigned long mli = millis();
  unsigned long tpmdif = mli - tact_push_millis;
  if (tpmdif > 4000)
    tap_millis_prev_set = false;
  
  // Set period and on times
  period = (60000000./BPM/PPB);  // period in usec
  ontime = period * duty_cycle * 0.01;
}