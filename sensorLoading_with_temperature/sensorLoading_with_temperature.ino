// Code for sensor and magnet placement checks.
// Code runs on Wemos Master.

// Nano reads data from 6 sensors and Teperature
// display the selected sample data on the screen.


#include <ESP8266WiFi.h> // Comment this line for Nano Master.
#include <Wire.h>

const int byte_number = 7;  // # of sesnors
byte sensor_value[7];
void setup() {
  Serial.begin(115200); // higher speed 
  
  // I2C Setup
  Wire.begin();
  Wire.setClockStretchLimit(40000); // Comment this line for Nano Master.
  delay(100);  // Short delay, wait for the Mate to send back CMD
}

void loop() {
    Serial.println("Enter the sensor number you want to load:  ");
    Serial.println("Or enter a 9 to quit");
    
    while(!Serial.available()){
      //Wait for input
    }
    int sensor_number = Serial.parseInt();
    Serial.println("got the number");
    while(sensor_number != 9){  
 //     Serial.println("asking for data from wire");
       Wire.requestFrom(1, byte_number);
        while (Wire.available()) {
     //     Serial.println("I have received data");
          for (int i = 0; i < 7; i++) {
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
