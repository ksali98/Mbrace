//Code for sensor and magnet placement checks.

//This code works. Nano reads data from 8 sensors @ 10Hz and 
//display the selected sample data on the screen.

//  NANO Low Power

#include <LowPower.h>
#include <avr/io.h>


const int byte_number = 8;  // # of bytes per sesnor array reading
byte sensor[8];

void setup() {
  Serial.begin(115200); // higher speed 
  DDRD = DDRD | 0xFC;    //Declare D2 to D7 as OUTPUTS
  DDRB = DDRB | 0x03;    //Declare D8 and D9 as OUTPUTS
  PORTD = PORTD | 0xFC;  //Set D2 to D7 HIGH
  PORTB = PORTB | 0x03;  //Set D8 and D9 HIGH
  pinMode(10, OUTPUT);
}

void loop() {
    Serial.println("Enter the sensor number you want to load:  ");
    Serial.println("Or enter a 9 to quit");
    
    while(!Serial.available()){
      //Wait for input
    }
    int sensor_number = Serial.parseInt();
    while(sensor_number != 9){ 
      for(int i = 0; i<8 ; i++){
        sensor[i] = get_mapped_analog(i);
      }
      Serial.print("sensor number ");
      Serial.print(sensor_number);
      Serial.print(":  ");
      Serial.println(sensor[sensor_number]);
      delay(10);
       if(Serial.available()){
        sensor_number = Serial.parseInt();
       }
       LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
       LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
    }
}

int get_mapped_analog(int analog_pin){
      int sensor_value;
      sensor_value = map(analogRead(analog_pin), 0, 1023, 0, 255);
      return sensor_value;
    }
