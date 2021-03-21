# Serial flash sampler for Teensy Audio  
Sampler with speed and direction control  

This is a buttoned up version of how I did the sampleing on the Delaydelus 2.  
Todo:    
Flash card save and load    
RAM version  
  
I've tried it with standard 128Mbit chips such as [this](https://www.mouser.com/ProductDetail/Winbond/W25Q128JVSIQ-TR?qs=%2Fha2pyFadug2TUVEhbfM5KgZ8P1%252BvsVof3yBZh8KkRNeKbu4RZ%2FjMQexLBohtVeA) but it can address any size chip.  

In this example one button is dedicated to erasing, one for recording and one playing.  The bank they are all address is controlled by the pot. A second sampler is started but unused.  
The samplers really just play back sections of memory. You can have multiple ones looking at the same part of memory ie the same sample.   
  
The sampler_helpers.h is necessary to deal with SPI and the AudioRecordQueue, this why it's not all in sf_sampler.    
The EEPROM is used to store the length and starting point of the samples rather than worry about having the flash do it.    
  
Sampler functions:
`begin(1, 1.0, bankstart[bank_sel], samplelen[bank_sel])` amplitude,frequency,start location,length. location and length are always bankstart[bank_sel] and samplelen[bank_sel], cagme out the bank_sel with a independent vslue for weach one  
`sample_play_loc(bankstart[bank_sel], samplelen[bank_sel])` same as begin, jsut switch out the number for bank_sel  
`sample_stop()` play is oneshot so use this to stop it  
`frequency(float)` 1.0 plays it back at the rate is was recorded. .5 is half speed, 2 is 2x speed  
`sample_loop(0 or 1)` 1 to loop this sampler  
`sample_reverse(0 or 1)` 1 to play it backwards  

