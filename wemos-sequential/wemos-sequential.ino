// New code, short iterrupt, fully sequential. SD and timestamp.
// and daily files. both in SD and MBRACE.xyz
// This is the testing phase of the code. This code supports WPA2 Enterprise as well
// as the regular WiFi Authentication. 
// Today is 09/26/2019
// Hardware is Wemos D1, SD Card holder, Nano and !Sensors, Logic Level Converter.
// Test will last for 3 days.  Kamal Ali
// Fill in the *****EDIT******

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>  // This is used to parse the response from time server
extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}
// [ssid:password]
// [printers:printers@187704] [iPhone:Okeafa@2015] [jsumobilenet:] [Mbrace_JSU:alialiali] [Alta Vista:alialiali]

// SSID to connect to
static const char* ssid = "printers"; //*****EDIT******
// Username for authentification
static const char* username = "";  //*****EDIT****** if Enterprise only
// Password for authentication
static const char* password = "printers@187704";  //*****EDIT******

const int sd_cs_pin = 8;  // CS pin on the WEMOS
const int byte_number = 6;  // # of bytes per sesnor array reading
const int sensor_group_readings = 10;  // # of readings we will group together before writing to sd card'
const String file_prefix = String("GCRL-");  // ******EDIT******

<<<<<<< Updated upstream
//const char* ssid     = "printers";
//const char* password = "printers@187704";
const char* ssid     = "iPhone";
const char* password = "Okeafa@2015";
//const char* ssid     = "jsumobilenet";
//const char* password = "";
//const char* ssid     = "Mbrace_JSU";
//const char* password = "alialiali1";
//const char* ssid     = "Alta Vista";
//const char* password = "alialiali";
=======
>>>>>>> Stashed changes
const char* host = "mbrace.xyz";
const int   port = 80;

File dataFile;
byte sensor_payload[byte_number*sensor_group_readings+8];  // 8 bytes for !! + millis + $$
byte output_payload[(byte_number*sensor_group_readings+8)*4/3];  // encoded string with time stamp !!$$
int day_counter = 0;
long file_start_time = 0;

volatile int payload_length = 0;
volatile bool interrupted = false;

void ICACHE_RAM_ATTR onTimerISR();
void send_payload(byte *payload, int payload_size);
int base64_encode(char *output, char *input, int inputLen);
int get_time_in_seconds();
void open_file();

String start_string;
String end_string;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);

  Serial.println("starting...");
  if(username != ""){
    Serial.println("using WPA2 Enterprise...");
    wpa_enterprise_connect();
  } else {
    Serial.println("using WPA2...");
    WiFi.begin(ssid, password);
  }
  // Wait for connection AND IP address from DHCP
  Serial.println();
  Serial.println("Waiting for connection and IP Address from DHCP");
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // SD Card Setup
  SD.begin(sd_cs_pin);
  dataFile = SD.open(file_prefix + String(day_counter) + ".DAT", FILE_WRITE);  // Set file name to be created on SD card
  dataFile.write("Experiment specific Data: \r\n");
  dataFile.write("Date: 07/02/2018 -- 13:35 \r\nLocation: GCRL \r\nCodeFile:wemos_sequential  \r\nDataFile: GCRL-nn \r\n");  // ******EDIT******
  dataFile.write("Comments: Daily files, @@,millils(),!! followed by 60 bytes per second..\r\n\r\n\r\n");  // ******EDIT******
  dataFile.flush();
  file_start_time = millis() - get_time_in_seconds() * 1000;

  // I2C Setup
//  Wire.begin();
//  Wire.setClockStretchLimit(40000);  // In µs for Wemos D1 and Nano
//  delay(100);  // Short delay, wait for the Mate to send back CMD

  // ISR Setup
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(100); // interrupt after 100? to get to ISR

  sensor_payload[0] = '@';
  sensor_payload[1] = '@';
  sensor_payload[6] = '#';
  sensor_payload[7] = '#';

  start_string = String("GET /data_collector/?mac="+WiFi.macAddress()+"&data=");  // "POST http(s)://host:port/api/v1/0AaRkVNSDhBSdSC56AnS/telemetry"
  end_string = String(" HTTP/1.1\r\nHost: ") + host + "\r\n\r\n";
}

void loop(){
 if (payload_length == byte_number*sensor_group_readings+8) {
    open_file();
    dataFile.write(sensor_payload, payload_length);
    dataFile.flush();
    base64_encode((char*)output_payload, (char*)sensor_payload, payload_length);
    send_payload(output_payload);
    payload_length = 0;
  }
  // Read sensors when interrupted
  if(interrupted){
    if(payload_length == 0){
      unsigned long current_time = millis();    
//      memcpy(&(sensor_payload[2]), &current_time, 4);
      sensor_payload[2] = (current_time >> 24) & 0xFF;
      sensor_payload[3] = (current_time >> 16) & 0xFF;
      sensor_payload[4] = (current_time >> 8) & 0xFF;
      sensor_payload[5] = current_time & 0xFF;
     
      payload_length = 8;
    }
<<<<<<< Updated upstream
      Wire.requestFrom(1, byte_number);
      while (Wire.available()) {
      for (int i = 0; i < byte_number; i++)
      {
        sensor_payload[payload_length] = Wire.read();
=======
//      Wire.requestFrom(1, byte_number);
//      while (Wire.available()) {
      for (int i = 0; i < byte_number; i++)
      {
        sensor_payload[payload_length] = i //Wire.read();
>>>>>>> Stashed changes
        payload_length++;
      }
    }
    interrupted = false;
  }
}

//ISR
void ICACHE_RAM_ATTR onTimerISR(){
  timer1_write(500000);// We have been interrupted, come back in 100ms time<<<<<<< Updated upstream
  interrupted = true;
}

//Sending WiFi data..
void send_payload(byte *payload) {
  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  String middle_string = String((char*)payload);
  middle_string.replace("+", "%2B"); // Php can not send a +, it has to be replaced by %2B (Major bug)
  client.print(start_string + middle_string + end_string);
}

// Time of day in seconds (used once)
int get_time_in_seconds(){
  WiFiClient wifi;
  HttpClient client = HttpClient(wifi, host, port);
  client.get("/current_time.php");
  return client.responseBody().toInt();
}

// New file name every day at Midnight, server time.
void open_file(){
  if((millis() - file_start_time) > 86400000){ //Full day in ms
//  if((millis() - file_start_time) > 20000){ // work only for 20 seconds before new filename.
    file_start_time = millis();
    day_counter++;
    Serial.println(day_counter);
    dataFile = SD.open(file_prefix + String(day_counter) + ".DAT", FILE_WRITE);  // Set file name to be created on SD card
  }
}

void wpa_enterprise_connect(){
  // WPA2 Connection starts here
  // Setting ESP into STATION mode only (no AP mode or dual mode)

  wifi_set_opmode(STATION_MODE);

  struct station_config wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  strcpy((char*)wifi_config.ssid, ssid);
 
  wifi_station_set_config(&wifi_config);
 
  wifi_station_clear_cert_key();
  wifi_station_clear_enterprise_ca_cert();
  wifi_station_set_wpa2_enterprise_auth(1);
  wifi_station_set_enterprise_identity((uint8*)username, strlen(username));
  wifi_station_set_enterprise_username((uint8*)username, strlen(username));
  wifi_station_set_enterprise_password((uint8*)password, strlen(password));
  wifi_station_connect();
  // WPA2 Connection ends here
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

