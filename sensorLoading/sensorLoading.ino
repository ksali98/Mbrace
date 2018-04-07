//Code for sensor and magnet placement checks.

//This code works. Wemos reads data from 6 sensors @ 10Hz and 
//display the selected sample data on the screen.


#include <ESP8266WiFi.h>
#include <Wire.h>

const int byte_number = 6;  // # of bytes per sesnor array reading
byte sensor_value[6];
void setup() {
  Serial.begin(9600); // higher speed 
  
  // I2C Setup
  Wire.begin();
  Wire.setClockStretchLimit(40000);  // In µs for Wemos D1 and Nano
  delay(100);  // Short delay, wait for the Mate to send back CMD
}

void loop() {
    Serial.println("Enter the sensor number you want to load:  ");
    Serial.println("Or enter a 9 to quit");
    
    while(!Serial.available()){
      //Wait for input
    }
    int sensor_number = Serial.parseInt();
    while(sensor_number != 9){  
       Wire.requestFrom(1, byte_number);
        while (Wire.available()) {
          for (int i = 0; i < 6; i++) {
            sensor_value[i] = Wire.read();
          }
          Serial.print("sensor number ");
          Serial.print(sensor_number);
          Serial.print(":  ");
          Serial.println(sensor_value[sensor_number]);
         }
         if(Serial.available()){
          sensor_number = Serial.parseInt();
         }
    }

}
