//Wemos_mbrace.xyz_10Hz_SD_working

//This code works. Wemos reads data from 6 sensors @ 10Hz (I2C not tested yet)
//This data is encoeded base 64, satitized bu remove and replacing '+' and sent to MBRACE.xyz.
//Data is simultaniously saved to an SD card. 

// Code working, tested 03/18/2018. I2C not tested, and no SD card connected.
//  Kamal Ali

//Every time this code is used, a section will be added to Setup where experimt specific data
//will be added. That shuold include experiment date, location and specifics that should be
//written to the SD card.  Kamal Ali  03/27/2018



#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>

const int byte_number = 6;  // # of bytes per sesnor array reading
const int sensor_group_readings = 10;  // # of readings we will group together
const char* ssid     = "Ellipsis Jetpack 5999";
const char* password = "b0eaef67";
const char* host = "mbrace.xyz";
const int   port = 80;
byte val = 0;

File dataFile;
int payload_length = 0;
byte sensor_payload[byte_number*sensor_group_readings];
byte output_payload[byte_number*sensor_group_readings];
volatile bool interrupted = false;

void ICACHE_RAM_ATTR onTimerISR();
void send_payload(byte *payload, int payload_size);
int base64_encode(char *output, char *input, int inputLen);

void setup() {

  Serial.begin(9600); // higher speed 
  
    // WIFI Setup
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  // SD Card Setup
  SD.begin();
  dataFile = SD.open("WiFiA.txt", FILE_WRITE);  // Set file name to be created on SD card
  // This part writes experiemnt specific data to SD card, Uncomment only prior to upload.
  
    dataFile.write("Experiment specific Data:");
    dataFile.write("Date: 03/29/2018 | Location: GCRL |CodeFile:Wemos_interrupts   | DataFile: WiFiA.txt ");
    dataFile.write("Comments: First WiFi experiemnt sending data to MBRACE.xyz, data_collector, with MAC as filename");
    dataFile.flush();

  // I2C Setup
  Wire.begin();
  Wire.setClockStretchLimit(40000);  // In µs for Wemos D1 and Nano
  delay(100);  // Short delay, wait for the Mate to send back CMD
  // ISR Setup
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(100); // interrupt after 100? to get to ISR
}

void loop() {
  if (payload_length == byte_number*sensor_group_readings) {
    dataFile.write("$$");
    dataFile.write(sensor_payload, payload_length);
    dataFile.flush();
    base64_encode((char*)output_payload, (char*)sensor_payload, payload_length);
    send_payload(output_payload, (payload_length)*4/3);
    payload_length = 0;
  }
  if (interrupted) {
  Wire.requestFrom(1, byte_number);
  while (Wire.available()) {
    for (int i = 0; i < byte_number; i++) {
      sensor_payload[payload_length] = Wire.read();
      payload_length++;
    }
   }
  }
  interrupted = false;
}


// functions start here.

//ISR
void ICACHE_RAM_ATTR onTimerISR(){
  timer1_write(500000);// We have been interrupted, come back in 100ms time
  interrupted = true;
}

//Sending WiFi data..
void send_payload(byte *payload, int payload_size) {
//  Serial.println("Sending payload"); //  Debug string
  WiFiClient client;
    if (!client.connect(host, port)) {
      Serial.println("connection failed");
      return;
    }

  byte request_string[10000];
  String start_string = String("GET /data_collector/?mac="+WiFi.macAddress()+"&data=");  // "POST http(s)://host:port/api/v1/0AaRkVNSDhBSdSC56AnS/telemetry"
  String middle_string = String((char*)payload);
  middle_string.replace("+", "%2B"); //Php can not send a +, it has to bre replaced by %2B (Major bug)
  String end_string   = String(" HTTP/1.1\r\nHost: ") + host + "\r\n\r\n";

// Serial.println("sending data: " + start_string + middle_string + end_string);  //Debug string

  client.print(start_string + middle_string + end_string);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
//  Serial.println("sent data");
// Read all the lines of the reply from server and print them to Serial
// while(client.available()){
//    String line = client.readStringUntil('\r');
//    Serial.print(line);
//  }
}

// Base64 Encoding and Decoding.......
const char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "abcdefghijklmnopqrstuvwxyz"
                            "0123456789+/";

/* 'Private' declarations */
inline void a3_to_a4(unsigned char * a4, unsigned char * a3);
inline void a4_to_a3(unsigned char * a3, unsigned char * a4);
inline unsigned char b64_lookup(char c);

int base64_encode(char *output, char *input, int inputLen) {
  int i = 0, j = 0;
  int encLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];

  while(inputLen--) {
    a3[i++] = *(input++);
    if(i == 3) {
      a3_to_a4(a4, a3);

      for(i = 0; i < 4; i++) {
        output[encLen++] = b64_alphabet[a4[i]];
      }
      i = 0;
    }
  }

  if(i) {
    for(j = i; j < 3; j++) {
      a3[j] = '\0';
    }

    a3_to_a4(a4, a3);

    for(j = 0; j < i + 1; j++) {
      output[encLen++] = b64_alphabet[a4[j]];
    }

    while((i++ < 3)) {
      output[encLen++] = '=';
    }
  }
  output[encLen] = '\0';
  return encLen;
}

int base64_decode(char *output, char *input, int inputLen) {
  int i = 0, j = 0;
  int decLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];


  while (inputLen--) {
    if(*input == '=') {
      break;
    }

    a4[i++] = *(input++);
    if (i == 4) {
      for (i = 0; i <4; i++) {
        a4[i] = b64_lookup(a4[i]);
      }

      a4_to_a3(a3,a4);

      for (i = 0; i < 3; i++) {
        output[decLen++] = a3[i];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) {
      a4[j] = '\0';
    }

    for (j = 0; j <4; j++) {
      a4[j] = b64_lookup(a4[j]);
    }

    a4_to_a3(a3,a4);

    for (j = 0; j < i - 1; j++) {
      output[decLen++] = a3[j];
    }
  }
  output[decLen] = '\0';
  return decLen;
}

int base64_enc_len(int plainLen) {
  int n = plainLen;
  return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

int base64_dec_len(char * input, int inputLen) {
  int i = 0;
  int numEq = 0;
  for(i = inputLen - 1; input[i] == '='; i--) {
    numEq++;
  }

  return ((6 * inputLen) / 8) - numEq;
}

inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
  a4[0] = (a3[0] & 0xfc) >> 2;
  a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
  a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
  a4[3] = (a3[2] & 0x3f);
}

inline void a4_to_a3(unsigned char * a3, unsigned char * a4) {
  a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
  a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
  a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline unsigned char b64_lookup(char c) {
  int i;
  for(i = 0; i < 64; i++) {
    if(b64_alphabet[i] == c) {
      return i;
    }
  }

  return -1;
}


  


