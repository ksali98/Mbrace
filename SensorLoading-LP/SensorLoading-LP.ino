//Code for sensor and magnet placement checks.
//This code works. Nano reads data from 8 sensors @ 10Hz and 
//display the selected sample data on the screen.

#include <LowPower.h>
#include <avr/io.h>

const int byte_number = 8;  // # of bytes per sesnor array reading
byte sensor[8];
int sensor_number;

int get_mapped_analog(int analog_pin);

void setup() {
  Serial.begin(115200); // higher speed 
  DDRD = DDRD | 0xFC;    //Declare D2 to D7 as OUTPUTS
  DDRB = DDRB | 0x03;    //Declare D8 and D9 as OUTPUTS
  PORTD = PORTD | 0xFC;  //Set D2 to D7 HIGH
  PORTB = PORTB | 0x03;  //Set D8 and D9 HIGH
  pinMode(10, OUTPUT);

  Serial.println("Enter the sensor number you want to load:  ");
  Serial.println("Or enter a 9 to quit");  
}

void loop() {
  if(Serial.available()){
    sensor_number = Serial.parseInt();
    if(sensor_number >= 0 && sensor_number < 8)
    {
      Serial.print("Changing to ");
      Serial.println(sensor_number);
    } 
    else
      Serial.println("Off");
  }
  
  for(int i = 0; i<8 ; i++){
  sensor[i] = get_mapped_analog(i);
  }
    if(sensor_number >= 0 && sensor_number < 8){
      Serial.print("sensor number ");
      Serial.print(sensor_number);
      Serial.print(":  ");
      Serial.println(sensor[sensor_number]);
      delay(30);
     }      
    LowPower.idle(SLEEP_60MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_ON, TWI_OFF);
    LowPower.idle(SLEEP_30MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_ON, TWI_OFF);
     //LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
     //LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);      
}

int get_mapped_analog(int analog_pin){
      int sensor_value;
      sensor_value = map(analogRead(analog_pin), 0, 1023, 0, 255);
      return sensor_value;
    }
