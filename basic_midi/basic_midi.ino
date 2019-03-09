#include <SPI.h>

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
  pinMode(cs_pin, OUTPUT);
  pinMode(gate_pin, OUTPUT);
  digitalWriteFast(cs_pin, HIGH);

  SPI.begin();
  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandlePitchChange(OnPitchChange);

}

void loop() {
  usbMIDI.read();
}

void OnNoteOn (byte channel, byte pitch, byte velocity) {
  if (channel == 1) {
    if (velocity > 0) {

      previous_pitch = pitch;
      pitch_value = map(previous_pitch + pitch_change_value, 0.0, maximum_pitch_value, 0.0, number_of_steps);
      writeDAC(cs_pin, 0, pitch_value);
      writeDAC(cs_pin, 1, velocity << 5);
      digitalWriteFast(gate_pin, HIGH);
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
  dac = dac & 1;
  val = val & 4095;
  Serial.println(val);
  SPI.transfer(dac << 7 | amp_gain << 5 | 1 << 4 | val >> 8);
  SPI.transfer(val & 255);
  digitalWrite(cs, HIGH);
}
