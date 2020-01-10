//Connects to WIFI networks and sends sensor data to Mbrace.xyz
//Can connect to WPA2 Enterprise if username is provided and the board is Esp32
//Writes data to SD card if available
//Written by Kamal S. Ali and James Curtis Addy
//Fill in the *****EDIT******

#include <Ticker.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>

///////////Wi-Fi////////////////////////
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#include <WiFi.h> 
#include <HTTPClient.h>

const char * ssid = "";
const char * wifi_username = "";
const char * wifi_password =  "";
////////////end of Wi-Fi////////////////

const int byte_number = 6;  // # of bytes per sensor array reading
const int sensor_group_readings = 10;  // # of readings grouped together before sending
const int data_block_size = byte_number * sensor_group_readings + 8; // 8 bytes for @@ + millis() + ##
byte sensor_payload[data_block_size + 1]; //+1 for '\0'
int payload_length = 0;

const String file_prefix = String("GCRL-");  // ******EDIT******
File data_file;
long file_start_time = 0;
int day_counter = 0;

const char* host = "mbrace.xyz";
const int   port = 80;

volatile bool interrupted = false;
void IRAM_ATTR onTimerISR();
hw_timer_t* timer = NULL;

void setup() {
  Serial.begin(115200); 
  Serial.println("Start");

  //Connect to WIFI 
  if(wifi_username == "")
  {
     WIFI_Connect_Normal();
  }
  else
  {
     WIFI_Connect_Enterprise(wifi_username, wifi_password); 
  }           

  delay(100);
  
  // SD Card Setup
  SD.begin();
  data_file = SD.open(file_prefix + String(day_counter) + ".DAT", FILE_WRITE);  // Set file name to be created on SD card
  //data_file.write("Experiment Data: \r\n");
  //data_file.write("Date: MM/dd/YYYY -- HH:mm \r\nLocation: GCRL \r\nCodeFile:esp32  \r\nDataFile: GCRL-nn \r\n");  // ******EDIT******
  //data_file.write("Comments: Daily files, @@,millis(),## followed by 60 bytes per second..\r\n\r\n\r\n");  // ******EDIT******
  data_file.flush();
  file_start_time = millis() - get_time_in_seconds() * 1000;

  // I2C Setup
  Wire.begin();
  delay(100);  // Short delay, wait for the Mate to send back CMD

  // ISR Setup for timer interrupt
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimerISR, true);
  timerAlarmWrite(timer, 100000, true);
  timerAlarmEnable(timer);

  //@@ and ## are delimiters for millis() timestamps
  sensor_payload[0] = '@';
  sensor_payload[1] = '@';
  sensor_payload[6] = '#';
  sensor_payload[7] = '#';
}

void loop() {
  //Reconnect if connection is lost
  if(WiFi.status() != WL_CONNECTED) {
    if(wifi_username == "")
    {
       WIFI_Connect_Normal();
    }
    else
    {
       WIFI_Connect_Enterprise(wifi_username, wifi_password); 
    }     
  }

  //Write and send data block when ready
  if (payload_length == data_block_size) {
    open_file();
    data_file.write(sensor_payload, payload_length);
    data_file.flush();
    sensor_payload[payload_length] = '\0'; //Not sent to server
    send_payload(sensor_payload);
    payload_length = 0;
  }
  
  // Read sensors when interrupted
  if (interrupted) {
    //Create a timestamp for the number of milliseconds the device has been running
    if (payload_length == 0) {
        //Converts the millis() value into 4 separate bytes
        unsigned long current_time = millis();       
        sensor_payload[2] = (current_time >> 24) & 0xFF;
        sensor_payload[3] = (current_time >> 16) & 0xFF;
        sensor_payload[4] = (current_time >> 8) & 0xFF;
        sensor_payload[5] = current_time & 0xFF;       
        payload_length = 8;
    }
    
    Wire.requestFrom(1, byte_number);
    while (Wire.available()) {
      for (int i = 0; i < byte_number; i++)
      {
        sensor_payload[payload_length] = Wire.read();     
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

//Send data to server
void send_payload(byte *payload) {
  HTTPClient http;
  char output_payload[base64_encoded_length(payload_length)];
  base_64_encode(output_payload, (char*)payload, payload_length); 
  String data_string = String((char*)output_payload); 
  data_string.replace("+", "%2B"); 
  http.begin("http://mbrace.xyz/data_collector/?mac=" + WiFi.macAddress() + "&data=" + data_string);  
  http.GET();
  Serial.println("payload sent");
}

// Time of day in seconds from server (GMT time - 6 hrs ahead of CST in U.S)
int get_time_in_seconds() {
  HTTPClient http;
  http.begin("http://mbrace.xyz/current_time.php");
  http.GET();
  return http.getString().toInt();
}

// New file name every day at midnight, server time (GMT).
void open_file() {
  if ((millis() - file_start_time) > 86400000) { //Full day in ms
    file_start_time = millis();
    day_counter++;
    Serial.println(day_counter);
    data_file = SD.open(file_prefix + String(day_counter) + ".DAT", FILE_WRITE);  // Set file name to be created on SD card
  }
}

//Base 64 encoding 
//Algorithm implemented by James Curtis Addy
const char* base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base_64_encode(char* output_string, char* input_string, int input_length)
{
  char bytes_3[3];
  char bytes_4[4];
  int padding;  
  int output_index = 0;
  
  for(int i = 0; i < input_length; i += 3)
  {
    int bytes_count = 0;
    char bytes_3[3] = "";
    char bytes_4[4] = "";
    for(int j = 0; j < 3 && i + j < input_length; j++)
    {
      bytes_3[j] = input_string[i + j];
      bytes_count += 1;
    }
      
    padding = 3 - bytes_count;
    
    bytes_4[0] = (bytes_3[0] >> 2) & 63;
    bytes_4[1] = ((bytes_3[0] & 3) << 4) | ((bytes_3[1] >> 4) & 15);
    bytes_4[2] = ((bytes_3[1] & 15) << 2) | ((bytes_3[2] >> 6) & 3);
    bytes_4[3] = bytes_3[2] & 63;
    
    for(int k = 0; k < (4-padding); k++)
    {
      output_string[output_index] = base64_table[bytes_4[k]];
      output_index += 1;
    }
  }
  
  for(int i = 0; i < padding; i++)
  {
    output_string[output_index] = '=';
    output_index += 1;
  }
  
  output_string[output_index] = '\0';
  return output_index;
}

int base64_encoded_length(int length)
{
  int encoded_length = (length * 4) / 3; //4 bytes for every 3
  return encoded_length + 2; //1 extra byte in case of int truncation and 1 for '\0'
}

//WIFI connection functions
void WIFI_Connect_Normal()
{
  Serial.print("Connecting to network: ");
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  Wait_For_Connection();
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
  Wait_For_Connection();
}

void Wait_For_Connection()
{
  WiFi.begin(ssid, wifi_password);
  int counter = 0;
  while(WiFi.status() != WL_CONNECTED){
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
