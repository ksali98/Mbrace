// Slave Nano with 6 sernsors. Sends 6 bytes with wire requests.
// uses 5V I2C bus.
// Kamal S. Ali, 06/08/2018

#include <Wire.h>

#define byte_number 6

byte sensor[byte_number]; 

void setup() {
  Wire.begin(1);                // join i2c bus with address 1
  Wire.onRequest(requestEvent); 
}

void loop() {
  // wait for I2C request....
}

void requestEvent() {
  sensor[0] = get_mapped_analog(0);
  sensor[1] = get_mapped_analog(1);
  sensor[2] = get_mapped_analog(2);
  sensor[3] = get_mapped_analog(3);
  sensor[4] = get_mapped_analog(6);
  sensor[5] = get_mapped_analog(7);
  Wire.write(sensor, byte_number); //write(array, length(in bytes))
}

int get_mapped_analog(int analog_pin)
{
    int sensor_value;
    sensor_value = map(analogRead(analog_pin), 0, 1023, 0, 255);
    return sensor_value;
}
