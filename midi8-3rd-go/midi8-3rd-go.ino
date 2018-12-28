#include <Wire.h>
#include <LCD.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
#include <Bounce.h>
#include <EEPROM.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define SSD1306_128_64
#define SSD1306_I2C_ADDRESS 0x3C
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

int cs_pin = 0;
int gate_pin = 14;
int amp_gain = 0; // gain for amplifier - 0 = 2x, 1 = 1x
int previous_pitch;
float pitch_change_value;
float pitch_value;
float maximum_voltage = 8.32; // voltage output after op amps
float maximum_pitch_value = maximum_voltage * 12.0; 
float number_of_steps = 4096; // DAC resolution

int buttonPin = 4;
Bounce pushbutton = Bounce(buttonPin, 10); 

long encoderPosn = 0;
//long encoderCounter = 0;
Encoder encoder(2, 3);

int activeTab = 0;
int screenMode = 1; // 1 is tabs, -1 is screen;
int tuning = EEPROM.read(0);


// ---------------//
// Main functions //
// ---------------//

void setup() {
  SPI.begin();
  setupMidi();
  setupControls();
  setupDisplay();
  //Serial.begin(9600);
}

void loop() {
  if (usbMIDI.read()) {
    //processMIDI();
  }
  onEncoderChange();
  onButtonClick();
}

// ------------------//
// Control functions //
// ------------------//

void setupControls() {
  pinMode(buttonPin, INPUT);
  attachInterrupt(4, onButtonClick, FALLING);
}

void onEncoderChange() {
  long newEncoderPosn;
  newEncoderPosn = encoder.read();
  if (newEncoderPosn != encoderPosn && newEncoderPosn % 4 == 0) { // if the 4th data point from quadrature encoder
    if (screenMode == 1) {
      activeTab += newEncoderPosn/4;
      activeTab = constrain(activeTab, 0, 2);
      renderTabs(activeTab);
    }
    if (activeTab == 1 && screenMode == -1) {
      tuning += newEncoderPosn/4;
      tuning = constrain(tuning, -10, 10);
      renderTuning(tuning, BLACK);
      EEPROM.write(0, tuning);
    }
    encoder.write(0);
  }
}

void onButtonClick() {
 if (pushbutton.update()) {
    if (pushbutton.fallingEdge() && activeTab != 0) {
      screenMode = -screenMode;
      Serial.println(screenMode);
    }
    if (activeTab == 1) {
      if (screenMode == -1) {
        renderTuning(tuning, BLACK);
      } else {
        renderTuning(tuning, WHITE);
      }
    }
  }
}


// ---------------//
// MIDI functions //
// ---------------//

void setupMidi() {
  pinMode(cs_pin, OUTPUT);
  pinMode(gate_pin, OUTPUT);
  digitalWriteFast(cs_pin, HIGH);

  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandlePitchChange(OnPitchChange);
}

void OnNoteOn (byte channel, byte pitch, byte velocity) {
  if (channel == 1) {
    if (velocity > 0) {

      previous_pitch = pitch;
      pitch_value = map(previous_pitch + pitch_change_value, 0.0, maximum_pitch_value, 0.0, number_of_steps);
      //pitch_value = pitch_value - 60;
        Serial.print("pitchVal: ");
        Serial.println(pitch);
      writeDAC(cs_pin, 0, pitch_value);
      writeDAC(cs_pin, 1, velocity << 5);
      digitalWriteFast(gate_pin, HIGH);
      String note = numberToString(pitch);
      if (activeTab == 0) {
        printToLCD("Vel:", 0, 18, 1, WHITE);
        printToLCD(velocity, 24, 18, 1, WHITE);
        printToLCD(note, 56, 18, 4, WHITE);
      }
      
    }

    else {
      digitalWriteFast(gate_pin, LOW);
    }
  }
}

void OnNoteOff (byte channel, byte pitch, byte velocity) {
  if (channel == 1) {
    digitalWriteFast(gate_pin, LOW);
  }
}

void OnPitchChange (byte channel, int pitch_change) {
  if (channel == 1) {
    pitch_change_value = map((float) pitch_change, 0.0, 16383.0, -12.0, 12.0);
    pitch_value = map(previous_pitch + pitch_change_value, 0.0, maximum_pitch_value, 0.0, number_of_steps);
    writeDAC(cs_pin, 0, pitch_value);
  }
}

void writeDAC (int cs, int dac, int val) {
  digitalWrite(cs, LOW);
  val = val* 1.1;
  dac = dac & 1;
  val = val & 4095;
  //val = val & 8191;
  if (dac == 0) {
        
//    Serial.print("DACval: ");
//    Serial.println(val);
//    Serial.println(val & 255);

  }

  SPI.transfer(dac << 7 | amp_gain << 5 | 1 << 4 | val >> 8);
  SPI.transfer(val & 255);
  digitalWrite(cs, HIGH);
}

//void processMIDI(void) {
//  byte type, channel, data1, data2;
//
//  // fetch the MIDI message, defined by these 5 numbers (except SysEX)
//  //
//  type = usbMIDI.getType();       // which MIDI message, 128-255
//  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
//  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
//  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
//  
//  switch (type) {
//    case usbMIDI.NoteOff: // 0x80
//      Serial.print("Note Off, ch=");
//      Serial.print(channel, DEC);
//      Serial.print(", note=");
//      Serial.print(data1, DEC);
//      Serial.print(", velocity=");
//      Serial.println(data2, DEC);
//      break;
//
//    case usbMIDI.NoteOn: // 0x90
//      
//      //int bytes = 112131;
//      Serial.print("Note On, ch=");
//      Serial.print(channel, DEC);
//      Serial.print(", note=");
//      Serial.print(data1, DEC);
//      Serial.print(", velocity=");
//      Serial.println(data2, DEC);
//      break;
//  }
//
//}

// --------------//
// LCD functions //
// --------------//

void setupDisplay()   {                
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
  renderTabs(activeTab);
  display.display();
}

void renderTabs(int encoderVal) {

  String tabs[3] = { "MAIN", "TUNE", "2" };
  for (int i = 0; i < 3; i++) {
    display.fillRect(1 + i * 42, 52, 41, 11, BLACK);
    if (i == activeTab) {
      display.fillRect(i * 43, 53, 41, 11, WHITE);
      printToLCD(tabs[i], 3 + i * 43, 55, 1, BLACK);
    } else {
      display.drawRect(i * 43, 53, 41, 11, WHITE);
      printToLCD(tabs[i], 3 + i * 43, 55, 1, WHITE);
    }
  }
  renderScreen(activeTab);
  encoder.write(activeTab * 4);
}

void renderScreen(int screen) {
  display.fillRect(0, 0, 128, 52, BLACK); // clear screen area

  switch(screen) {

    case 0  :
      display.drawLine(0, 11, display.width()-1, 11, WHITE);
      printToLCD("MIDI to CV v1.0", 0, 0, 1, WHITE);
      //printToLCD("Velo: ", 0, 20, 1, WHITE);;
      break;
  
    case 1  :
      //display.
      printToLCD("Tuning", 40, 0, 1, WHITE);
      renderTuning(tuning, WHITE);
     

      break;

    case 2  :
      printToLCD(screen, 0, 0, 2, WHITE);;
      break;

  }

}

void renderTuning(int value, char color) {
  int pixelScale = 5;
  display.fillRect(40, 30, 60, 20, BLACK);
  display.fillRect(0, 12, 128, 12, BLACK);
  if (tuning > 0)
    display.fillRect(64, 12, tuning * pixelScale, 12, WHITE);
  else {
    display.fillRect(64 + (tuning * pixelScale), 12, -(tuning * pixelScale), 12, WHITE);
  }
  display.drawRect(0, 12, 128, 12, WHITE);
  printToLCD(value, 40, 30, 2, color);

  display.display();
}

String numberToString (byte pitch) {
  int octave = int (pitch / 12) - 2;
  String notes = "C C#D D#E F F#G G#A A#B ";
  String note = notes.substring((pitch % 12) * 2, (pitch % 12) * 2 + 2);
  String noteWithOctave = note + octave;
  return noteWithOctave;
}

void printToLCD (String value, int curX, int curY, int size, char color) {
  char bgColor = BLACK;
  if (color == BLACK) {
    bgColor = WHITE;
  }
  int strLength = value.length();
  display.fillRect(curX, curY, strLength * 6 * size, 8 * size, bgColor);
  display.setTextSize(size);
  display.setTextColor(color);
  display.setCursor(curX, curY);
  display.println(value);
  display.display();

}
