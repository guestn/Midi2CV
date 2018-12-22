#include <Wire.h>
#include <LCD.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define SSD1306_128_64
#define SSD1306_I2C_ADDRESS   0x3C
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

void setup() {
  SPI.begin();
  setupMidi();
  setupDisplay();
}

void loop() {
    if (usbMIDI.read()) {
    processMIDI();
  }
}

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
      printToLCD(("NoteOn: " + note), 0, 32, 1);
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
   //val = val * 4.0;
  dac = dac & 1;
  val = val & 4095;
  //val = val & 8191;
  if (dac == 0) {
        
    Serial.print("DACval: ");
    Serial.println(val);
    Serial.println(val & 255);

  }

  SPI.transfer(dac << 7 | amp_gain << 5 | 1 << 4 | val >> 8);
  SPI.transfer(val & 255);
  digitalWrite(cs, HIGH);
}

void processMIDI(void) {
  byte type, channel, data1, data2;

  // fetch the MIDI message, defined by these 5 numbers (except SysEX)
  //
  type = usbMIDI.getType();       // which MIDI message, 128-255
  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
  
  switch (type) {
    case usbMIDI.NoteOff: // 0x80
      Serial.print("Note Off, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      break;

    case usbMIDI.NoteOn: // 0x90
      printToLCD(data2, 60, 20, 1);
      //int bytes = 112131;
      Serial.print("Note On, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      break;
  }

}



// --------------//
// LCD functions //
// --------------//

void setupDisplay()   {                
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  display.clearDisplay();
   // display.fillCircle(display.width()/2, display.height()/2, 10, WHITE);
  display.drawLine(0, 11, display.width()-1, 11, WHITE);
  printToLCD("MIDI to CV v1.0", 0, 0, 1);
  printToLCD("Velocity: ", 0, 20, 1);
  //display.invertDisplay(true);

  //display.fillRect(curX, curY, 100, 10, BLACK);


  display.drawRect(0, 53, 41, 11, WHITE);
  printToLCD("MAIN", 3, 55, 1);
  display.drawRect(43, 53, 41, 11, WHITE);
  printToLCD("2", 47, 55, 1);
  display.drawRect(86, 53, 41, 11, WHITE);
  printToLCD("3", 90, 55, 1);
  
  display.display();
}

String numberToString (byte pitch) {
  int octave = int (pitch / 12) - 2;
  String notes = "C C#D D#E F F#G G#A A#B ";
  String note = notes.substring((pitch % 12) * 2, (pitch % 12) * 2 + 2);
  String noteWithOctave = note + octave;
  return noteWithOctave;
}

void printToLCD (String value, int curX, int curY, int size) {
  //display.fillRect(curX, curY, 100, 10, BLACK);

  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(curX, curY);
  display.println(value);
  display.display();

}
