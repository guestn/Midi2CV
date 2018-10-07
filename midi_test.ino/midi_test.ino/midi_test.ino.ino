#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Addr, En, Rw, Rs, d4, d5, d6, d7, backlighpin, polarity
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define SSD1306_128_64
#define SSD1306_I2C_ADDRESS   0x3D
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

int data2;
int noteOn;

int cs_pin = 0;
int gate_pin = 14;
//#define gate_pin 14
int amp_gain = 0; // gain for amplifier - 0 = 2x, 1 = 1x
int led_pin = 13;
int previous_pitch;
float pitch_change_value;
float pitch_value;
float maximum_voltage = 8.32; // voltage output after op amps
float maximum_pitch_value = maximum_voltage * 12.0; 
float number_of_steps = 4096; // DAC resolution

void setup() {
  pinMode(cs_pin, OUTPUT);
  pinMode(gate_pin, OUTPUT);
  digitalWriteFast(cs_pin, HIGH);
  
  SPI.begin();

  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleControlChange(OnCC); // set handle for MIDI continuous controller messages
  usbMIDI.setHandlePitchChange(OnPitchChange);

  setupDisplay();
}

void loop() {
  usbMIDI.read();
  if (usbMIDI.read()) {
    //processMIDI();
  }
}


// ---------------//
// MIDI functions //
// ---------------//

// this function is called whenever a MIDI continuous controller messahe is received
// the function is executed using the received message bytes of channel, controller and value corresponding to the variable names below
// for example, a modulation wheel message the first channel with a half value will be called with OnCC(1, 1, 64);
// messaged can be filtered by controller value
void processMIDI(void) {
  byte type, channel, data1, data2;

  // fetch the MIDI message, defined by these 5 numbers (except SysEX)

  type = usbMIDI.getType();       // which MIDI message, 128-255
  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
  
  switch (type) {
    case usbMIDI.NoteOff: // 0x80
//      Serial.print("Note Off, ch=");
//      Serial.print(channel, DEC);
//      Serial.print(", note=");
//      Serial.print(data1, DEC);
//      Serial.print(", velocity=");
//      Serial.println(data2, DEC);
      break;

    case usbMIDI.NoteOn: // 0x90
     // printToLCD(data2, 60, 20, 1);
      //printToLCD(("NoteOn: " + data1), 0, 32, 1);
      //int bytes = 112131;
//      Serial.print("Note On, ch=");
//      Serial.print(channel, DEC);
//      Serial.print(", note=");
//      Serial.print(data1, DEC);
//      Serial.print(", velocity=");
//      Serial.println(data2, DEC);
      break;
  }

}


void OnCC(byte channel, byte controller, byte value) {
  if(value >= 64) {
  }

  if(value < 64) {
  }
}

void OnNoteOn (byte channel, byte pitch, byte velocity) {
  String note = numberToString(pitch);
  printToLCD(("NoteOn: " + note), 0, 32, 1);
  //plotActiveNote(pitch);
//  Serial.println(channel);
//  Serial.println(pitch);
//  Serial.println(velocity);


  if (channel == 1) {
    if (velocity > 0) {
      Serial.println("NOTE ON");
      previous_pitch = pitch;
      pitch_value = map(previous_pitch + pitch_change_value, 0.0, maximum_pitch_value, 0.0, number_of_steps);
      writeDAC(cs_pin, 0, pitch_value);
      writeDAC(cs_pin, 1, velocity << 5);
      //digitalWriteFast(gate_pin, HIGH);
      digitalWriteFast(gate_pin, HIGH);
    }

    else {
      digitalWriteFast(gate_pin, LOW);
    }
  }
}

void OnNoteOff (byte channel, byte pitch, byte velocity) {
   printToLCD(("NoteOn: ---"), 0, 32, 1);
   //plotKeyboard();
   Serial.println("NOTE OFF");


   if (channel == 1) {
    digitalWriteFast(gate_pin, LOW);

  }
}

void OnPitchChange (byte channel, int pitch_change) {
  Serial.print("Pitch change: ");
  //Serial.println(pitch_value);

  if (channel == 1) {
    pitch_change_value = map((float) pitch_change, 0.0, 16383.0, -12.0, 12.0);
    pitch_value = map(previous_pitch + pitch_change_value, 0.0, maximum_pitch_value, 0.0, number_of_steps);
    writeDAC(cs_pin, 0, pitch_value);
  }
}

void writeDAC (int cs, int dac, int val) {
  Serial.print(" DAC: ");
  Serial.print(cs, DEC);
  Serial.print(" dac: ");
  Serial.print(dac, DEC);
  Serial.print(" val: ");
  Serial.print(val, DEC);

  
  digitalWrite(cs, LOW);
  dac = dac & 1;
  val = val & 4095;
  SPI.transfer(dac << 7 | amp_gain << 5 | 1 << 4 | val >> 8);
  SPI.transfer(val & 255);
  digitalWriteFast(cs, HIGH);
}

// --------------//
// LCD functions //
// --------------//

String numberToString (byte pitch) {
  int octave = int (pitch / 12) - 2;
  String notes = "C C#D D#E F F#G G#A A#B ";
  String note = notes.substring((pitch % 12) * 2, (pitch % 12) * 2 + 2);
  String noteWithOctave = note + octave;
  return noteWithOctave;
}

void printToLCD (String value, int curX, int curY, int size) {
  display.fillRect(curX, curY, 100, 10, BLACK);

  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(curX, curY);
  display.println(value);
  display.display();

}

void setupDisplay()   {                
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
   // display.fillCircle(display.width()/2, display.height()/2, 10, WHITE);
  display.drawLine(0, 11, display.width()-1, 11, WHITE);
  printToLCD("MIDI to CV v1.0", 0, 0, 1);
  printToLCD("Velocity: ", 0, 20, 1);
  //display.invertDisplay(true);
  plotKeyboard();
  display.display();
}

void plotKeyboard () {
  //how to reset?
  //display.fillRect(0, 43, 128, 12, BLACK);

  // white keys
 for (int16_t i=0; i<7; i+=1) {
    display.drawRect(i*12, 43, 13, 20, WHITE);
    display.display();
    delay(1);
  }
  // black keys
  for (int16_t i=0; i<6; i+=1) {
    if (i !=2) {
      display.fillRoundRect(i*12 + 9, 43, 7, 12, 1, WHITE);
      display.display();
      delay(1);
    }
  } 
}

void plotActiveNote (byte pitch) {
  int KEYWIDTH = 12;
  int leftOffset = ((pitch + 9) % 12) * KEYWIDTH + KEYWIDTH * 0.5;
  Serial.println(((pitch % 12)+ 9));
  display.fillCircle(leftOffset, 50, KEYWIDTH/3, WHITE);
}
