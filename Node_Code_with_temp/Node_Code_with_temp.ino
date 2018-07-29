// Slave Nano with 6 sernsors. Sends 6 bytes of data plus one byte of temperature with wire requests.
// uses 5V I2C bus. Water temp is X5. 10K Pullup on temp sensor.
// Kamal S. Ali, 07/28/2018

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define byte_number 7
#define ONE_WIRE_BUS 2 //  edit this number
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int get_mapped_temperature();
 
byte sensor[byte_number]; 
byte temp;

void setup() {
  Wire.begin(1);                // join i2c bus with address 1
  Wire.onRequest(requestEvent); 
//  Serial.begin(9600);
  sensors.begin();
}

void loop() {
//  temp = get_mapped_temperature();
//  Serial.println(temp);
//  delay (200);
}

void requestEvent() {
  sensor[0] = get_mapped_analog(0);
  sensor[1] = get_mapped_analog(1);
  sensor[2] = get_mapped_analog(2);
  sensor[3] = get_mapped_analog(3);
  sensor[4] = get_mapped_analog(6);
  sensor[5] = get_mapped_analog(7);
  sensor[6] = get_mapped_temperature;
  Wire.write(sensor, byte_number); //write(array, length(in bytes))
}

int get_mapped_analog(int analog_pin) {
  return map(analogRead(analog_pin), 0, 1023, 0, 255);
}

int get_mapped_temperature() {
  sensors.requestTemperatures(); // Send the command to get temperatures 
  return (byte)(sensors.getTempCByIndex(0)*5);
}
