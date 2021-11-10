/*
 * CW flea transmitter, Ver 0.3 Copyright 2021 Kevin Loughin, KB9RLW
 * 
 * This program is free software, you can redistribute in and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation.
 * http://www.gnu.org/licenses
 * 
 * This software is provided free of charge but without any warranty.
 * 
 */

// Include the etherkit si5351 library.  You may have to add it using the library
// manager under the sketch/Include Library menu.

#include "si5351.h"
#include "Wire.h"
Si5351 si5351;

// Declare variables and flags we will use in the program

int tuneup = 2; //pin number used for tuning up in freq
int tunedn = 3; //pin number used for tuning down in freq
int keypin = 7; // pin number used for key input
int relaypin = 4; //pin number used for relay control
int tunecount = 500; //delay between slow tuning and fast tuning
int tunestep = 5000; //minimum tuning step in hundreths Hertz
int tuning = 0; //flag to indicate tuning state
int tunevfo = 0; //flag to indicate tuning vfo state
int buttonup = 1; //flag for tune up button
int buttondn = 1; //flag for tune down button
int tail = 0;  // The counter for the keying tail delay
int taildefault = 1000; // Default to 1 second tail, change if you want.
int keyed = 1; //flag to indicate keyed state
int keyedprev = 1; //flag to indicate previous key state after a change
int unkeyflag = 0; //flag for end of timeout switch back to RX
long freq = 1403000000; //current frequency in hundreths Hertz.
long bandbottom = 1400000000; //Lowest frequency in hundreths Hertz
long bandtop = 1407000000; //Highest frequency in hundreths Hertz


void setup() {

// Setup the I/O pins and turn the relay off
  pinMode (relaypin, OUTPUT); //TR relay control line
  digitalWrite(relaypin, LOW); // turn off relay
  pinMode (keypin, INPUT); // Key input line
  digitalWrite(keypin, HIGH); // turn on internal pullup resistor
  pinMode (tuneup, INPUT); // tune switch input line
  digitalWrite(tuneup, HIGH); // turn off internal pullup resistor
  pinMode (tunedn, INPUT); // tune switch input line
  digitalWrite(tunedn, HIGH); // turn off internal pullup resistor
//  Serial.begin(9600); // enable serial for debugging output

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); //initialize the VFO
  si5351.set_freq(freq, SI5351_CLK0);  //set initial freq
  si5351.set_freq(freq, SI5351_CLK1);
  si5351.output_enable(SI5351_CLK0, 0); //turn off outputs
  si5351.output_enable(SI5351_CLK1, 0);
  
  // Setup the interrupt for the timer interrupt routine
   OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
// 
}

// interrupt handler routine.  Handles the timing counters, runs 1000 times per second
SIGNAL(TIMER0_COMPA_vect) 
{
  if ( (tail > 0) && keyed ) // If we're in the tail period, count down
   {
    tail--;
   }
   if ( (tunecount > 0) && tunevfo ) // If we're tuning, count down before increase tuning speed
   {
    tunecount--;
   }
}

  // This is the main program loop. Runs continuously:
void loop() 
{
// Read state of all inputs. note: 0=key or button pressed, 1=not pressed
keyed = digitalRead (keypin); // Read the state of the key line
buttonup = digitalRead (tuneup); //read tuneup button state
buttondn = digitalRead (tunedn); //read tunedn button state

// Keying section, check key state, turn output on and off

// Checking the timer if we're in the tail period
if ( !tail && unkeyflag ) // timeout counted down, switch back to RX mode
    {
      digitalWrite (relaypin, LOW); // turn off TR relay
      unkeyflag = 0; //reset the flag
    }
    
// Keying logic next    
  
  if ( keyed && !keyedprev) // did we just unkey? set the timeout value, turn off VFO
    {
      tail = taildefault;
      keyedprev = 1;
     si5351.output_enable(SI5351_CLK0, 0); // turn off VFO
      unkeyflag = 1; // flag for tail routine that we just unkeyed
    }
    
  if (!keyed && keyedprev) // did we just key down?
    {
      digitalWrite(relaypin, HIGH); // switch TR relay
      delay (10); // wait 10 millisecond
      si5351.output_enable(SI5351_CLK0, 1); //turn on VFO
      keyedprev = 0; //set the flag so we know we keyed last time through.
    }

// Tuning logic next

  if (buttonup && buttondn && tuning) //if no buttons are pressed, reset tuning flag
    {
      tuning=0;
    }
  if (!buttonup || !buttondn) //either tune button pressed?
    {
      tuning = 1; //set flag to indicate button pressed
      if (!tunevfo) //turn on VFO if its off and set vfo flag
        {
          si5351.output_enable(SI5351_CLK1, 1);
          tunevfo = 1;
        }
      if (!buttonup) //tuning up in freq
        {
         if (tunecount) // Move freq in low speed step
           {
             freq = freq + tunestep ;            
           }
         if (!tunecount) // Move freq in high speed step
           {
             freq = freq + (tunestep * 3) ;
           } 
         if (freq > bandtop) // If we reached band edge, roll over
            {
              freq = bandbottom ;
            }
         si5351.set_freq(freq, SI5351_CLK0);  //set vfo freq
         si5351.set_freq(freq, SI5351_CLK1);
        }
      if (!buttondn) //tuning down in freq
        {
         if (tunecount) // Move freq in low speed step
           {
             freq = freq - tunestep ;            
           }
         if (!tunecount) // Move freq in high speed step
           {
             freq = freq - (tunestep * 3) ;
           } 
         if (freq < bandbottom) // If we reached band edge, roll over
            {
              freq = bandtop ;
            }
         si5351.set_freq(freq, SI5351_CLK0);  //set vfo freq
         si5351.set_freq(freq, SI5351_CLK1);
        }
    }
  
    
if (tunevfo && !tunecount && !tuning) //buttons released, delay done, turn off VFO
    {
      delay (1000);
      si5351.output_enable(SI5351_CLK1, 0); //turn off VFO
      tunecount = 500 ;
      tunevfo = 0;
    }
// Serial.println(tunevfo); //debugging line, ignore.
}
