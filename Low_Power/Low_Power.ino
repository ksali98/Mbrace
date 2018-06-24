// Low power system supporting 8 sensors and 4 data collection speeds.
// Arduino Nano and Hall 2425 system.
// Kamal Ali, 06/22/2018

#include <SD.h>
#include <SPI.h>
#include <LowPower.h>
#include <avr/io.h>

// Arduino Nano Low power sensor system 
// CS pin   - 10  
// MOSI pin - 11
// MISO pin - 12
// SCK pin  - 13
 
int data_block = 0; //  10 Second data Blocks
byte sensors[400]; // 
File dataFile;
const String file_prefix = String("ww4"); // *** EDIT *** // first part of SD file name
int file_number = 0;
long readings_in_file = 0;
int sleep_time = 1; // *** EDIT *** // Enter reading frequency value here: Only 1, 2, 3 or 4.
long readings_per_file = 0;  // 10*60*60*24 = 864000 is the default
                                 // 1 = 10Hz, 2 = 1Hz, 3 =  1min, 4 = 1hr.
void open_file();

void setup() {
  DDRD = DDRD | 0xFC;    //Declare D2 to D7 as OUTPUTS
  DDRB = DDRB | 0x03;    //Declare D8 and D9 as OUTPUTS
  PORTD = PORTD | 0xFC;  //Set D2 to D7 HIGH
  PORTB = PORTB | 0x03;  //Set D8 and D9 HIGH
  Serial.begin(9600);
  pinMode(10, OUTPUT);
  SD.begin(10);
  dataFile = SD.open(file_prefix + file_number + ".txt", FILE_WRITE);  // Set file name to be created on SD card
  dataFile.write("Experiment specific Data: date time and location of experiment. \r\n"); //  *** EDIT  ***
  dataFile.flush();
  set_readings_per_file();
}
    
void loop() {
  for(int i = 0; i < data_block; i++) { 
    if(i % 8 == 0 && i != 0) {
      sleep();
    }
    readings_in_file++; // number of bytes in file..
    sensors[i] = map(analogRead(i % 8), 0, 1023, 0, 255);  // Read in A0 to A7 into sensor array as single bytes
  }
  dataFile.write("$$"); // write $$ and data block
  dataFile.write(sensors, data_block);
  dataFile.flush();
  open_file();
}

// Functions start here. *****************************

void open_file() {
//  Serial.print(readings_in_file);
//  Serial.print(" : ");
//  Serial.println(readings_per_file);
//  delay(100);
  if(readings_in_file >= readings_per_file){
    dataFile.close();
    file_number++;
    readings_in_file = 0; 
    dataFile = SD.open(file_prefix + file_number + ".txt", FILE_WRITE);  // Set file name to be created on SD card
  } 
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
    case 1: // 10Hz  write every 400 bytes, 5 seconds
      readings_per_file = 6912000 ;  // bytes per day
      data_block = 400;
      break;
    case 2: // 1Hz writes every 80 bytes, 10 seconds
      readings_per_file = 691200;
      data_block = 80;
      break;
    case 3: // 1 minuet, writes every 80 bytes, 10 minutes
      readings_per_file = 11520;
      data_block = 80;
      break;
    case 4: // 1 hour, writes every 8 bytes, 1 hour.
      readings_per_file = 192;
      data_block = 8;
      break;
  }
}

