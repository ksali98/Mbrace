#include <SD.h>
#include <SPI.h>
#include <LowPower.h>
#include <avr/io.h>

 
// CS pin   - 10  
// MOSI pin - 11
// MISO pin - 12
// SCK pin  - 13
 
#define DATA_LENGTH 800 //10 Second data Blocks
byte sensors[DATA_LENGTH];
File dataFile;
const String file_prefix = String("AAAAA");
int file_number = 0;
int readings_in_file = 0;
int sleep_time = 1; // Enter reading frequency value here: Only 1, 2, 3 or 4.
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
  set_readings_per_file()
}
    
void loop() {
  for(int i = 0; i < DATA_LENGTH; i++) {  // For a total of 800 bytes;
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
  dataFile.write(sensors, DATA_LENGTH);
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
      break;
    default: 
      // if nothing else matches, do the default
      // default is optional
  }
}

void set_readings_per_file(){
  switch (sleep_time) {
    case 1:
      readings_per_file = 864000;
      break;
    case 2:
      readings_per_file = 86400;
      break;
    case 3:
      readings_per_file = 1440;
      break;
    case 4:
      readings_per_file = 24;
      break;
    default:
      //nothing matched. you set an invalid sleep_time
  }
}

