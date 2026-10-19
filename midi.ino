#include <Arduino.h>
#include <USB.h>
#include <USB-MIDI.h>
#include <FastLED.h>

// ==========================================
// 1. PIN CONFIGURATION (Change to match your KiCad layout!)
// ==========================================
// Capacitive Touch Keys
#define KEY_1 1
#define KEY_2 2
#define KEY_3 3
#define KEY_4 4
#define KEY_5 5
#define KEY_6 6
#define KEY_7 7

// Rotary Encoder (EC11)
#define ENC_A 8
#define ENC_B 9

// Slide Potentiometer (Must be an ADC pin)
#define SLIDE_POT 10

// Neopixels
#define LED_PIN 11
#define NUM_LEDS 8

// ==========================================
// 2. MIDI & LED SETUP
// ==========================================
// Native USB MIDI
USBMIDI_CREATE_DEFAULT_INSTANCE();

// Neopixels
CRGB leds[NUM_LEDS];

// Touch thresholds (Adjust these based on your PCB's copper pads)
// Higher value = needs more touch. You will need to calibrate this.
const int touchThreshold = 40; 
bool keyStates[7] = {false, false, false, false, false, false, false};
int touchPins[7] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7};
// MIDI Notes (C Major scale: C4, D4, E4, F4, G4, A4, B4)
int midiNotes[7] = {60, 62, 64, 65, 67, 69, 71};

// Encoder variables
int lastEncA = LOW;
int encoderPos = 0;

// ==========================================
// 3. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Start USB MIDI
  MIDI.begin(1); // Channel 1
  USB.begin();

  // Setup Neopixels
  FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50); // Don't blind yourself!
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Setup Encoder
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  lastEncA = digitalRead(ENC_A);
}

// ==========================================
// 4. MAIN LOOP
// ==========================================
void loop() {
  readTouchKeys();
  readEncoder();
  readSlidePot();
  
  delay(10); // Small delay to debounce
}

// ==========================================
// 5. HELPER FUNCTIONS
// ==========================================

// Read the 7 capacitive touch pads
void readTouchKeys() {
  for (int i = 0; i < 7; i++) {
    // touchRead returns a lower number when touched
    int touchValue = touchRead(touchPins[i]);
    bool isTouched = (touchValue < touchThreshold);

    if (isTouched && !keyStates[i]) {
      // Key was just pressed
      keyStates[i] = true;
      MIDI.sendNoteOn(midiNotes[i], 127, 1);
      
      // Light up the NeoPixel under the key
      leds[i] = CRGB::Cyan; 
      FastLED.show();
      
    } else if (!isTouched && keyStates[i]) {
      // Key was just released
      keyStates[i] = false;
      MIDI.sendNoteOff(midiNotes[i], 0, 1);
      
      // Turn off the NeoPixel
      leds[i] = CRGB::Black;
      FastLED.show();
    }
  }
}

// Read the rotary encoder to send Control Change (CC) messages
void readEncoder() {
  int currentEncA = digitalRead(ENC_A);
  if (currentEncA != lastEncA && currentEncA == LOW) {
    if (digitalRead(ENC_B) != currentEncA) {
      encoderPos--;
      MIDI.sendControlChange(1, 127, 1); // CC 1 (Mod Wheel) Down
    } else {
      encoderPos++;
      MIDI.sendControlChange(1, 1, 1);   // CC 1 (Mod Wheel) Up
    }
  }
  lastEncA = currentEncA;
}

// Read the slide potentiometer to send Pitch Bend
void readSlidePot() {
  int potValue = analogRead(SLIDE_POT);
  
  // Map the 12-bit ADC (0-4095) to the 14-bit Pitch Bend range (0-16383)
  int pitchBendValue = map(potValue, 0, 4095, 0, 16383);
  
  // Send Pitch Bend (0 is max down, 8192 is center, 16383 is max up)
  MIDI.sendPitchBend(pitchBendValue, 1);
}