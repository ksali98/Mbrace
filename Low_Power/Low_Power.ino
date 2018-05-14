#include <SD.h>
#include <SPI.h>
#include <LowPower.h>
#include <avr/io.h>

 
// CS pin   - 10  
// MOSI pin - 11
// MISO pin - 12
// SCK pin  - 13
 
int data_block = 0; //10 Second data Blocks
byte sensors[800];
File dataFile;
const String file_prefix = String("AAAAA"); // *** EDIT *** // first part of SD file name
int file_number = 0;
int readings_in_file = 0;
int sleep_time = 1; // *** EDIT *** // Enter reading frequency value here: Only 1, 2, 3 or 4.
int readings_per_file = 864000;  // 10*60*60*24 = 864000 is the default
                                 // 1 = 10Hz, 2 = 1Hz, 3 =  1min, 4 = 1hr.


void setup() {
  DDRD = DDRD | 0xFC;   //Declare D2 to D7 as OUTPUTS
  DDRB = DDRB | 0x03;    //Declare D8 and D9 as OUTPUTS
  PORTD = PORTD | 0xFC; //Set D2 to D7 HIGH
  PORTB = PORTB | 0x03;  //Set D8 and D9 HIGH
  pinMode(10, OUTPUT);
  SD.begin(10);
  dataFile = SD.open(file_prefix + file_number, FILE_WRITE);  // Set file name to be created on SD card
  set_readings_per_file();
}
    
void loop() {
  for(int i = 0; i < data_block; i++) {  // For a total of 800 bytes;
    if(i % 8 == 0 && i != 0) {
      // Sleep for selected time, after every 8 bytes
      readings_in_file++; // after reading sensors but before going to sleep, increment readings counter
      sleep();
    }
    sensors[i] = map(analogRead(i % 8), 0, 1023, 0, 255);  // Read in A0 to A7 into sensor array as single bytes
  }
  if(readings_in_file >= readings_per_file){
    dataFile.close();
    file_number++;
    readings_in_file = 0; 
    dataFile = SD.open(file_prefix + file_number, FILE_WRITE);  // Set file name to be created on SD card
  }
  dataFile.write("$$"); // write $$ and 800 bytes every 10 Seconds
  dataFile.write(sensors, data_block);
  dataFile.flush();
}

void sleep() {
    switch (sleep_time) {
    case 1:
      LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
      LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
      break;
    case 2:
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      break;
    case 3:
        for(int j = 0; j < 7; j++){
          LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  //Sleep for 7X8 = 56 seconds.
        }
          LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); //plus another 4 seconds.
      break;
    case 4:
        for(int j = 0; j < 450; j++){
          LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  //Sleep for 450X8 = 3600 seconds.
        }
  }
}

void set_readings_per_file(){
  switch (sleep_time) {
    case 1: // 10Hz  write every 800 bytes, 10 seconds
      readings_per_file = 864000;
      data_block = 800;
      break;
    case 2: // 1Hz writes every 80 bytes, 10 seconds
      readings_per_file = 86400;
      data_block = 80;

      break;
    case 3: // 1 minuet, writes every 80 bytes, 10 minutes
      readings_per_file = 1440;
      data_block = 80;

      break;
    case 4: // 1 hour, writes every 8 bytes, 1 hour.
      readings_per_file = 24;
      data_block = 8;

      break;
  }
}

