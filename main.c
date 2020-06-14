#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <PWMServo.h>

// Configuration
// Connections
#define SERVO_PIN 10 // Jaw servo
#define PING_PIN 5 // ping / echo for sonar distance sensor
#define ECHO_PIN 6
#define MOSFETPin 9 // mostfet for servo power control

// How long until the ping returns to consider it a "trigger"
#define  SONAR_THRESHOLD 900 // microseconds.
// Speed of sound in air is about 343 m/s, so this is about 15 cm (~6 in), or a 30cm round trip.
// Quick calculators:
// const SONAR_THRESHOLD (148 * 6) // ~inches
// const SONAR_THRESHOLD (58 * 15) // ~cm

// Servo range configuration
#define JAW_OPEN 70
#define JAW_CLOSED 0

// Length of time for `loop()` to run.  1000/25 Hz (40ms) should be pretty fluid
#define  SAMPLE_TIME 200 // ms

// Sound file to play when sonar triggers
#define  SAMPLE_FILE "01.wav"

// Delay playback to compensate for the jaw servo's lag.
#define SERVO_DELAY 50 // ms

// To play with this, import the below block into https://www.pjrc.com/teensy/gui/

// playSdWav1 will use the built-in sd card slot, assumed to have a fat32 filesystem.
// If you're using some other way to attach SD, or you'd like to use in-memory or
// whatever, there are other options.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=329,371
AudioMixer4              mixer1;         //xy=548,386
AudioEffectDelay         delay1;         //xy=749,424
AudioOutputAnalog        dac1;           //xy=951,421
AudioAnalyzePeak         peak1;          //xy=973,360
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav1, 1, mixer1, 1);
AudioConnection          patchCord3(mixer1, delay1);
AudioConnection          patchCord4(delay1, 0, peak1, 0);
AudioConnection          patchCord5(delay1, 1, dac1, 0);
// GUItool: end automatically generated code

#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13

PWMServo jaw;

// Entirely optional.  For stable sample management.  `elapsedMillis` is a teensy type that
// automatically counts up, so it's useful for making sure if you want 25Hz, you _get_ 25Hz.
elapsedMillis timing;

int ping() {
  digitalWrite(PING_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(PING_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(PING_PIN, LOW);
  if (pulseIn(ECHO_PIN, HIGH, SONAR_THRESHOLD) != 0) {
    return HIGH;
  }
  return LOW;
}


uint8_t mosfet_state = LOW;
void set_mosfet(uint8_t state) {
  if (state != mosfet_state) {
    Serial.print("Setting MOSFET to ");
    Serial.println(state == LOW ? "LOW" : "HIGH");
    mosfet_state = state;
    digitalWrite(MOSFETPin, mosfet_state);
  }
}


void setup() {
  Serial.begin(9600);
  Serial.println("initializing audio memory");
 
  // Set up audio memory.  You need at least 1 block for each 2.9 ms of delay, plus
  // a minimum of 8 blocks for the player.
  AudioMemory((int) (ceil(SERVO_DELAY / 2.9) + 8));
  Serial.println("initializing pins");
  jaw.attach(SERVO_PIN);
  pinMode(PING_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOSFETPin, OUTPUT);
  delay1.delay(0, 0);    
  delay1.delay(1, SERVO_DELAY);
  delay1.disable(2);
  delay1.disable(3);
  delay1.disable(4);
  delay1.disable(5);
  delay1.disable(6);
  delay1.disable(7);
 
  mixer1.gain(0, 1.0);
  mixer1.gain(1, 1.0);
  mixer1.gain(2, 0);
  mixer1.gain(3, 0);
 
  Serial.println("initializing SD access");
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println(SD.begin(SDCARD_CS_PIN));
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

void loop_state_waiting() {
  if (mosfet_state == HIGH) {
    Serial.println("playback ended; returning to sleep.");
  }
  set_mosfet(LOW);
  if (ping() == HIGH) {
    Serial.println("sonar ping!  Starting sound/animation.");
    set_mosfet(HIGH);
    playSdWav1.play(SAMPLE_FILE);
    delay(5);
  }
}

void loop_state_playing() {
  if (peak1.available()) {
    float level = peak1.readPeakToPeak();
    int new_servo_pos = (int) (level * (JAW_OPEN - JAW_CLOSED) / 2.0 + JAW_CLOSED);
    Serial.print("Level: ");
    Serial.print(level);
    Serial.print("; servo: ");
    Serial.println(new_servo_pos);
    jaw.write(new_servo_pos);
  }
}

void loop() {
  // resets the counter
  timing = 0;
  if (!playSdWav1.isPlaying()) {
    loop_state_waiting();
  } else {  
    loop_state_playing();
  }
  // Only delay for the remaining time in the sample.
  delay(SAMPLE_TIME - timing);
}

