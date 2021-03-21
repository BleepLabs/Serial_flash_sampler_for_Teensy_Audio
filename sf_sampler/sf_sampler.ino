/*
  serial flash sampler with speed and direction control
  Find out more about Teensy memory here https://github.com/BleepLabs/dadageek-Feb21/wiki/Memory-types-and-sampling-to-serial-flash

  In this example one button is dedicated to erasing, one for recording and one playing.
  The bank they are all address is controlled by the pot.
  A second sampler is started but unused

  The samplers really just play back sections of memory. You can have multiple ones looking at the same part of memory ie the same sample


  queue_left and queue_right are AudioRecordQueue object and are what take the audio data
   and get it into the sampler. Don't change their names

  The sampler_helpers.h is necessary to deal with SPI and the AudioRecordQueue, this why it's not all in sf_sampler
  The EEPROM is used to store the length and starting point of the samples rather than worry about having the flash do it

  Sampler functions:
  begin(1, 1.0, bankstart[bank_sel], samplelen[bank_sel]); amplitude,frequency,start location,length. location and length are always bankstart[bank_sel] and samplelen[bank_sel], cagme out the bank_sel with a independent vslue for weach one
  sample_play_loc(bankstart[bank_sel], samplelen[bank_sel]); same as begin, jsut switch out the number for bank_sel
  sample_stop() play is pneshot so use this to stop it
  frequency(float 0-4.0) 1.0 plays it back at the rate is was recorded. .5 is half speed, 2 is 2x speed
  sample_loop(0 or 1) 1 to loop this sampler
  sample_reverse(0 or 1) 1 to play it backwards

*/

//include this before the audio initialization
#include "sf_sampler.h"
#include <Bounce2.h>
////////////////////////////////////////

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSampler           sampler1;     //xy=81,443
AudioSampler           sampler0;     //xy=82,484
AudioInputI2S            line_in;           //xy=156,246
AudioMixer4              sampler_mixer_right;         //xy=289,514
AudioMixer4              sampler_mixer_left;         //xy=291,430
AudioAmplifier           amp_in_left;           //xy=338,216
AudioAmplifier           amp_in_right;           //xy=341,262
AudioRecordQueue         queue_left;         //xy=484,188
AudioRecordQueue         queue_right;         //xy=498,241
AudioMixer4              final_mixer_right;         //xy=505,470
AudioMixer4              final_mixer_left;         //xy=518,380
AudioOutputI2S           i2s2;           //xy=682,409
AudioConnection          patchCord1(sampler1, 0, sampler_mixer_left, 0);
AudioConnection          patchCord2(sampler1, 1, sampler_mixer_right, 0);
AudioConnection          patchCord3(sampler0, 0, sampler_mixer_left, 1);
AudioConnection          patchCord4(sampler0, 1, sampler_mixer_right, 1);
AudioConnection          patchCord5(line_in, 0, amp_in_left, 0);
AudioConnection          patchCord6(line_in, 1, amp_in_right, 0);
AudioConnection          patchCord7(sampler_mixer_right, 0, final_mixer_right, 0);
AudioConnection          patchCord8(sampler_mixer_left, 0, final_mixer_left, 0);
AudioConnection          patchCord9(amp_in_left, 0, final_mixer_left, 1);
AudioConnection          patchCord10(amp_in_left, queue_left);
AudioConnection          patchCord11(amp_in_right, 0, final_mixer_right, 1);
AudioConnection          patchCord12(amp_in_right, queue_right);
AudioConnection          patchCord13(final_mixer_right, 0, i2s2, 1);
AudioConnection          patchCord14(final_mixer_left, 0, i2s2, 0);
AudioControlSGTL5000     sgtl5000_1;
// GUItool: end automatically generated code

//the order of the following is important. You can change number of banks but leave the rest alone
#define FlashChipSelect 6 //for the audio adapter board chip

//each sfblocks is 256 kilobytes
//In 44.1kHz stereo that's just under 1.5 seconds
// a 127 Mbit chip has 64 blocks
#define number_of_banks 8
#define sfblocks  64/number_of_banks //evenly distributes the blocks in the set number of banks. Odd banks will make the last bank length a little differnt

#include "sf_sampler_helpers.h"
//end

unsigned long cm;
unsigned long prev[4];
int bank_sel;
float freq[4];
float vol;
float vol_rec_mon;

#define button1_pin 0
#define button2_pin 1
#define button3_pin 2


#define BOUNCE_LOCK_OUT //this tells it what mode to be in. I think it's the better one for music
Bounce button1 = Bounce();
Bounce button2 = Bounce();
Bounce button3 = Bounce();


void setup() {
  delay(500); //wait a tiny bit so the chip is ready to go
  SPI.setSCK(14);  // Audio shield has SCK on pin 14
  SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
  SPI.setCS(FlashChipSelect);
  delay(500); //wait a tiny bit so the chip is ready to go

  if (!SerialFlash.begin(FlashChipSelect)) {
    while (1) { //don't continue if the chip isn't there
      Serial.println("Unable to access SPI Flash chip");
      delay(1000);
    }
  }

  AudioMemory(15);

  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.lineInLevel(13);
  sgtl5000_1.volume(0.25);


  load_sample_locations(); //must be done before other sampler stuff. it gets the bankstart[n] samplelen[n] from eeprom
  //amplitude,frequency,start location,length
  //just copy these and change out the bankstart and samplelen array addressing number
  sampler0.begin(1, 1.0, bankstart[0], samplelen[0]);
  sampler1.begin(1, 1.0, bankstart[1], samplelen[1]);

  amp_in_left.gain(1);//can be used to amplify the incoming signal before the sampler
  amp_in_right.gain(1);

  sampler_mixer_left.gain(0, 1);
  sampler_mixer_left.gain(1, 1);
  sampler_mixer_left.gain(2, 0);
  sampler_mixer_left.gain(3, 0);

  sampler_mixer_right.gain(0, 1);
  sampler_mixer_right.gain(1, 1);
  sampler_mixer_right.gain(2, 0);
  sampler_mixer_right.gain(3, 0);

  final_mixer_left.gain(0, 1);
  final_mixer_left.gain(1, 0);
  final_mixer_left.gain(2, 0);
  final_mixer_left.gain(3, 0);

  final_mixer_right.gain(0, 1);
  final_mixer_right.gain(1, 0);
  final_mixer_right.gain(2, 0);
  final_mixer_right.gain(3, 0);

  analogReadResolution(12); //even though teensy 4 is 10b. Doesn't seem to cause issues and makes it so the 3.x used the whole range
  analogReadAveraging(64);

  Serial.println("Ready");

}

void loop() {
  cm = millis();

  button1.update();
  button2.update();
  button3.update();


  if (button1.fell()) {
    erase_bank(bank_sel); //will print out the erase progress. Takes a few seconds
  }

  if (button2.fell()) {
    startRecording(bank_sel);  //record must be done in these 3 steps. First start is called jsut once...
  }

  if (button2.read() == 0) {
    continueRecording(); ///...then continue while the button is down...
    //final_mixer_left.gain(1, 1); //listen to the incoming audio
    //final_mixer_right.gain(1, 1);
  }
  if (button2.rose()) {
    stopRecording();//...then stop
    //final_mixer_left.gain(1, 0); //mute incoming audio
    //final_mixer_right.gain(1, 0);
  }

  if (button3.fell()) {
    // only do this once like noteOn
    //start location in memory in bytes, length in bytes
    //you can modify these to get some granular type stuff going on.
    sampler0.sample_play_loc(bankstart[bank_sel], samplelen[bank_sel]);
  }

  if (button3.rose()) {
    //if you want the sample to only play when the button is down uncomment this
    // sampler0.sample_stop();

  }
  if (cm - prev[0] > 10) { //we don't need to all this screamingly fast.
    prev[0] = cm;
    prev_bank_sel = bank_sel;

    vol = analogRead(A3) / 4095.0;
    vol_rec_mon = (analogRead(A6) / 4095.0);
    final_mixer_left.gain(0, vol);
    final_mixer_right.gain(0, vol);
    final_mixer_left.gain(1, vol_rec_mon);
    final_mixer_right.gain(1, vol_rec_mon);


    int raw_freg_pot = analogRead(A1);
    if (raw_freg_pot <= 2048) { //if the pot is on the left side
      freq[0] = raw_freg_pot / 2048.0; // 0-1.0
    }
    if (raw_freg_pot > 2048) { //if the pot is on the right side
      freq[0] = (((raw_freg_pot - 2048.0) / 2048.0) * 3.0) + 1.0; //1 - 4.0
    }


    bank_sel = map(analogRead(A2), 0, 4095, 0, number_of_banks - 1);
    sampler0.frequency(freq[0]);
    if (prev_bank_sel != bank_sel) {
      Serial.println(bank_sel);
    }
  }

  if (cm - prev[1] > 500) {
    prev[1] = cm;
    //Serial.println(freq[0]);

    if (1) {
      Serial.print("proc: ");
      Serial.print(AudioProcessorUsageMax());
      Serial.print("%    Mem: ");
      Serial.print(AudioMemoryUsageMax());
      Serial.println();
      AudioProcessorUsageMaxReset(); //We need to reset these values so we get a real idea of what the audio code is doing rather than just peaking in every half a second
      AudioMemoryUsageMaxReset();
    }
  }

}
