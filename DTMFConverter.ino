// DTMF to loop Disconnect files for Arduino. 
// Hardware: Kevin Dodman,
// Software: Adam Dodman 30/12/18

// Timer library for easy interrupt management
#include "TimerOne.h"

// Define this so we can safely use interrupts with CircularBuffer
#define CIRCULAR_BUFFER_INT_SAFE

// This  needs to be installed by going to Tools -> Manage Libraries... -> Search for Circular Buffer by AgileWare
#include <CircularBuffer.h>

// Set these to match the pins driving the relay and DTMF board.
const int stqPin = 8;
const int q1Pin = 9;
const int q2Pin = 10;
const int q3Pin = 11;
const int q4Pin = 12;
const int pulsePin = 4;

// Pulse timings,  defaults should work fine. All in milliseconds. 
const long offTime = 67;
const long onTime = 33;
const long digitTime = 1000;

//Store up to 50 digits, should be more than enough!
CircularBuffer<int,50> buffer;
bool stqRead = false;
// Volatile tells the compiler to keep  this in RAM at all times, needed for using it in interrupts.
volatile int dialPulses = 0;

void setup() {
  // Prep the timer, and then pause it
  Timer1.initialize(digitTime * 1000);
  Timer1.stop();
  // Init pins
  pinMode(q1Pin, INPUT);
  pinMode(q2Pin, INPUT);
  pinMode(q3Pin, INPUT);
  pinMode(q4Pin, INPUT);
  pinMode(pulsePin, OUTPUT);
  
  // If this is LOW, relay will start active
  digitalWrite(pulsePin, HIGH);
}

void pulseOff () {
  // Drive relay low, then set timer to turn it back on
  digitalWrite(pulsePin, HIGH);
  Timer1.attachInterrupt(pulseOn);
  Timer1.setPeriod(offTime * 1000);
}

void pulseOn () {
  // Turn the relay back on, then work out if we need to pulse again
  digitalWrite(pulsePin, LOW);
  dialPulses = dialPulses - 1;
  if (dialPulses > 0) {
    Timer1.attachInterrupt(pulseOff);
    Timer1.setPeriod(onTime * 1000);
  }
  else {
    Timer1.stop();
    Timer1.detachInterrupt();
  }
}
int getDTMFValue() {
  // Convert the 4 binary pins to an interger.
  int DTMFRead = 0;
  if (digitalRead(q1Pin) == HIGH) {
    DTMFRead += 1;
  }
  if (digitalRead(q2Pin) == HIGH) {
    DTMFRead += 2;
  }
  if (digitalRead(q3Pin) == HIGH) {
    DTMFRead += 4;
  }
  if (digitalRead(q4Pin) == HIGH) {
    DTMFRead += 8;
  }
  if (DTMFRead == 0) {
    // D is all off, so instead return 16 pulses. 
    DTMFRead = 16;
  }
  return DTMFRead;
}

void loop() {
  // Check if we need to pulse out a new number
  if (!buffer.isEmpty() && dialPulses == 0) {
    dialPulses = buffer.shift();
    // Hold the relay on for 1 second before dialing starts, this is the inter-digit pulse
    digitalWrite(pulsePin, LOW);
    Timer1.attachInterrupt(pulseOff);
    Timer1.setPeriod(digitTime * 1000);
  }
  
  // Look at stq pin first, if its now low, reset stqRead for new number
  if(stqRead) {
    stqRead = digitalRead(stqPin);
  }

  // If Read isn't set, read the buffer and set the flag to true
  if (stqRead == false && digitalRead(stqPin) == HIGH) {
    buffer.push(getDTMFValue());
    stqRead = true;
  }
}
