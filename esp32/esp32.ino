//New code, short interrupt, fully sequential. SD timestamp.
// and daily files. both in SD and MBRACE.xyz
// This CODE is fully functional. 10Hz data, from 6 sensors on a Nano
// Data collected by Esp32.
// Kamal Ali,  12/1/2019
// Fill in the *****EDIT******

#include <Ticker.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>
///////////Wi-Fi////////////////////////
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#include <WiFi.h> //Wifi library
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const char *  ssid = "jsumobilenet";
const char * wifi_username = "";
const char * wifi_password =  "";

WiFiUDP ntp_udp;
NTPClient timeClient(ntp_udp);

////////////end of Wi-Fi////////////////

const int byte_number = 6;  // # of bytes per sesnor array reading
const int sensor_group_readings = 10;  // # of readings we will group together before writing to sd card'
const String file_prefix = String("GCRL-");  // ******EDIT******

const char* host = "mbrace.xyz";
const int   port = 80;

File dataFile;
byte sensor_payload[byte_number * sensor_group_readings + 8]; // 8 bytes for @@ + millis + ##
byte output_payload[(byte_number * sensor_group_readings + 8) * 4 / 3]; // encoded string with time stamp @@##
int day_counter = 0;
long file_start_time = 0;

volatile int payload_length = 0;
volatile bool interrupted = false;

void IRAM_ATTR onTimerISR();
hw_timer_t * timer = NULL;
void send_payload(byte *payload, int payload_size);
int base64_encode(char *output, char *input, int inputLen);
int get_time_in_seconds();
void open_file();

void setup() {
  Serial.begin(115200); // higher speed
  Serial.println("start");

  //WIFI Setup
  while (WiFi.status() != WL_CONNECTED) {
    if(wifi_username == "")
    {
       WIFI_Connect_Normal();
    }
    else
    {
       WIFI_Connect_Enterprise(wifi_username, wifi_password); 
    }           
  }
  delay(100);
  
  // SD Card Setup
  SD.begin();
  dataFile = SD.open(file_prefix + String(day_counter) + ".DAT", FILE_WRITE);  // Set file name to be created on SD card
  //dataFile.write("Experiment Data: \r\n");
  //dataFile.write("Date: MM/dd/YYYY -- HH:mm \r\nLocation: GCRL \r\nCodeFile:esp32  \r\nDataFile: GCRL-nn \r\n");  // ******EDIT******
  //dataFile.write("Comments: Daily files, @@,millils(),## followed by 60 bytes per second..\r\n\r\n\r\n");  // ******EDIT******
  dataFile.flush();
  file_start_time = millis() - get_time_in_seconds() * 1000;

  // I2C Setup
  Wire.begin();
  delay(100);  // Short delay, wait for the Mate to send back CMD

  // ISR Setup
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimerISR, true);
  timerAlarmWrite(timer, 100000, true);
  timerAlarmEnable(timer);
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    if(wifi_username == "")
    {
       WIFI_Connect_Normal();
    }
    else
    {
       WIFI_Connect_Enterprise(wifi_username, wifi_password); 
    }           
  }
  
  if (payload_length == byte_number * sensor_group_readings + 8) {
    open_file();
    dataFile.write(sensor_payload, payload_length);
    dataFile.flush();   
    base64_encode((char*)output_payload, (char*)sensor_payload, payload_length);
    send_payload(output_payload);
    payload_length = 0;
  }
  
  // Read sensors when interrupted
  if (interrupted) {
    if (payload_length == 0) {
        unsigned long current_time = millis();
        sensor_payload[0] = '@';
        sensor_payload[1] = '@';
        sensor_payload[2] = (current_time >> 24) & 0xFF;
        sensor_payload[3] = (current_time >> 16) & 0xFF;
        sensor_payload[4] = (current_time >> 8) & 0xFF;
        sensor_payload[5] = current_time & 0xFF;       
        sensor_payload[6] = '#';
        sensor_payload[7] = '#';
        payload_length = 8;
    }
    Wire.requestFrom(1, byte_number);
    while (Wire.available()) {
      for (int i = 0; i < byte_number; i++)
      {
        sensor_payload[payload_length] = Wire.read();
        //sensor_payload[payload_length] = i;
        payload_length++;
      }
    }
    interrupted = false;
  }
}

//ISR
void IRAM_ATTR onTimerISR(){
  interrupted = true;
}

//Send WiFi data
void send_payload(byte *payload) {
  HTTPClient http;
  String data_string = String((char*)payload);
  data_string.replace("+", "%2B");
  http.begin("http://mbrace.xyz/data_collector/?mac=" + WiFi.macAddress() + "&data=" + data_string);
  http.GET();
  Serial.println("payload sent");
}

// Time of day in seconds (used once)
int get_time_in_seconds() {
  HTTPClient http;
  http.begin("http://mbrace.xyz/current_time.php");
  http.GET();
  return http.getString().toInt();
}

// New file name every day at Midnight, server time.
void open_file() {
  if ((millis() - file_start_time) > 86400000) { //Full day in ms
    //  if((millis() - file_start_time) > 20000){ // work only for 20 seconds before new filename.
    file_start_time = millis();
    day_counter++;
    Serial.println(day_counter);
    dataFile = SD.open(file_prefix + String(day_counter) + ".DAT", FILE_WRITE);  // Set file name to be created on SD card
  }
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

  while (inputLen--) {
    a3[i++] = *(input++);
    if (i == 3) {
      a3_to_a4(a4, a3);

      for (i = 0; i < 4; i++) {
        output[encLen++] = b64_alphabet[a4[i]];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) {
      a3[j] = '\0';
    }

    a3_to_a4(a4, a3);

    for (j = 0; j < i + 1; j++) {
      output[encLen++] = b64_alphabet[a4[j]];
    }

    while ((i++ < 3)) {
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
    if (*input == '=') {
      break;
    }

    a4[i++] = *(input++);
    if (i == 4) {
      for (i = 0; i < 4; i++) {
        a4[i] = b64_lookup(a4[i]);
      }

      a4_to_a3(a3, a4);

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

    for (j = 0; j < 4; j++) {
      a4[j] = b64_lookup(a4[j]);
    }

    a4_to_a3(a3, a4);

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
  for (i = inputLen - 1; input[i] == '='; i--) {
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
  for (i = 0; i < 64; i++) {
    if (b64_alphabet[i] == c) {
      return i;
    }
  }

  return -1;
}

void WIFI_Connect_Normal()
{
  WiFi.begin(ssid, wifi_password);
  Wait_For_Connection()
}

void WIFI_Connect_Enterprise(const char * username, const char * password) {
  Serial.print("Connecting to network: ");
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username)); //provide identity
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username)); //provide username --> identity and username is same
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password)); //provide password
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
  esp_wifi_sta_wpa2_ent_enable(&config); //set config settings to enable function
  WiFi.begin(ssid); //connect to wifi
  Wait_For_Connection()
}

void Wait_For_Connection()
{
   int counter = 0;
   while (WiFi.status() != WL_CONNECTED){
      if(counter >= 20)
      {
        Serial.println("No connection in 10 seconds. Retrying connection");
        WiFi.begin(ssid, wifi_password);
        counter = 0;
      }
      delay(500);
      Serial.print(".");
      counter += 1;
   }
  
    Serial.println("connected to " + String(ssid));
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address set: ");
    Serial.println(WiFi.localIP());
}
