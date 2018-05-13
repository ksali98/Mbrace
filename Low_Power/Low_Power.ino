#include <SD.h>
#include <SPI.h>
#include <LowPower.h>
#include <avr/io.h>

 
// CS pin   - 10  
// MOSI pin - 11
// MISO pin - 12cc
// SCK pin  - 13
 
#define DATA_LENGTH 800 //10 Second data Blocks
byte sensors[DATA_LENGTH];
File dataFile;

void setup() {
  DDRD = DDRD | 0xFC;   //Declare D2 to D7 as OUTPUTS
  DDRB = DDRB | 0x03;    //Declare D8 and D9 as OUTPUTS
  PORTD = PORTD | 0xFC; //Set D2 to D7 HIGH
  PORTB = PORTB | 0x03;  //Set D8 and D9 HIGH
  pinMode(10, OUTPUT);
  SD.begin(10);
  dataFile = SD.open("Testing.txt", FILE_WRITE);
}
    
void loop() {
  for(int i = 0; i < DATA_LENGTH; i++) {  // For a total of 800 bytes;
    if(i % 8 == 0 && i != 0) { // Sleep for 90ms, after every 8 bytes
      LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
      LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
    }
    sensors[i] = map(analogRead(i % 8), 0, 1023, 0, 255);  // Read in A0 to A7 into sensor array as single bytes
  }
  dataFile.write("$$"); // write $$ and 800 bytes every 10 Seconds
  dataFile.write(sensors, DATA_LENGTH);
  dataFile.flush();
}
