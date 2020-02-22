//Connects to WIFI networks and sends sensor data to Mbrace.xyz
//Can connect to WPA2 Enterprise if username is provided and the board is Esp32
//Writes data to SD card if available
//Written by Kamal S. Ali and James Curtis Addy
//Fill in the *****EDIT******

#include <Wire.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <string.h>
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#include <WiFi.h>
#include <HTTPClient.h>

//*****EDIT******
const char* experiment_start_date = "2020-02-16";//YYYY-MM-DD 
const char * ssid = "";//network name
const char * wifi_username = "";//For WPA2-Enterprise connections
const char * wifi_password =  "";
//***END EDIT****

const unsigned int byte_number = 6;  // # of bytes per sensor array reading
const unsigned int sensor_group_readings = 10;  // # of readings grouped together before sending
const unsigned int data_block_size = byte_number * sensor_group_readings + 8; // 8 bytes for @@ + millis() + ##
byte sensor_payload[data_block_size + 1]; //+1 for '\0'
const unsigned int day_byte_count = data_block_size * 3600 * 24; //Day's worth of bytes
unsigned int payload_length = 0;
const char* host = "mbrace.xyz";
const unsigned int   port = 80;
unsigned int wifi_connection_timer = 0;

volatile bool interrupted = false;
void IRAM_ATTR onTimerISR();
hw_timer_t* timer = NULL;

//SD
String sd_folder_name_string; 
unsigned int sd_file_counter = 0;
String sd_file_path_string;

File sd_file;

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  
  connect_to_wifi();
  
  //Begin SD object (mount to SD card)
  if (SD.begin(5))
  {
     Serial.println(F("SD Card Mount Succeeded"));
     set_sd_folder_name_string();
     //Create SD folder if it doesn't exist on the card
     if(!SD.exists(sd_folder_name_string.c_str()))
     {
        SD.mkdir(sd_folder_name_string.c_str());
     }
     //Set counter to the most recent file in the folder
     sd_file_counter = get_file_count(SD, sd_folder_name_string.c_str());
     if(sd_file_counter == 0) sd_file_counter = 1; //Start at 1
  }
  else
    Serial.println(F("SD Card Mount Failed"));

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

void loop() 
{
  //Write and send data block when ready
  if (payload_length == data_block_size) 
  {
    update_sd_file(SD);    
    
    sensor_payload[payload_length] = '\0';   
     
    //Attempt to connect if WiFi isn't connected.
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println(F("WiFi isn't connected."));  
          
      //Retry connecting if it has been 10 seconds without connection.
      if(wifi_connection_timer >= 10)
      {
        connect_to_wifi();
        wifi_connection_timer = 0;
      }      
      wifi_connection_timer += 1;
    }
    
    //Send the payload if connected to WiFi.
    if(WiFi.status() == WL_CONNECTED)
    {
      wifi_connection_timer = 0;
      send_payload(sensor_payload);
    }               
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

//SD functions
unsigned int get_file_count(fs::FS &fs, const char* folder_path)
{
  File root = fs.open(folder_path);
  if(!root)
  {
    Serial.print(F("Failed to open "));
    Serial.println(folder_path);
    return 0;
  }
  if(!root.isDirectory())
  {
    Serial.print(folder_path);
    Serial.println(F(" is not a directory"));
    return 0;
  }
  int count = 0;
  File current_file = root.openNextFile();

  while(current_file)
  {
    if(!current_file.isDirectory())
    {
      count++;
    }
    current_file.close();
    current_file = root.openNextFile();
  }
  return count;
}

void update_sd_file(fs::FS &fs)
{
  if(!sd_file)
  {
    //Initialize file
    update_sd_file_path_string();
    sd_file = SD.open(sd_file_path_string.c_str(), FILE_APPEND);
  } 
  
  //Create a new file when current file is at the limit
  if(sd_file.size() >= day_byte_count)
  {
    sd_file_counter += 1;
    sd_file.close();
    update_sd_file_path_string();
    sd_file = SD.open(sd_file_path_string.c_str(), FILE_APPEND);
  }
  sd_file.write(sensor_payload, data_block_size);
  sd_file.flush();
  Serial.print(sd_file.name());
  Serial.print(F("'s file size: "));
  Serial.println(sd_file.size());
}

void set_sd_folder_name_string()
{
   sd_folder_name_string = String("/" + WiFi.macAddress() + "_" + String(experiment_start_date));
   sd_folder_name_string.replace(":", "_");
}

void update_sd_file_path_string()
{
  sd_file_path_string = String(sd_folder_name_string + "/data-" + String(sd_file_counter) + ".bin");
}

//WIFI connection functions
void wifi_connect_normal()
{
  Serial.print(F("Connecting to network: "));
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.begin(ssid, wifi_password);
}

void wifi_connect_enterprise(const char * username, const char * password) {
  Serial.print(F("Connecting to network: "));
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username)); //provide identity
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username)); //provide username --> identity and username is same
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password)); //provide password
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
  esp_wifi_sta_wpa2_ent_enable(&config); //set config settings to enable function
  WiFi.begin(ssid, wifi_password);
}

void connect_to_wifi()
{
  //Choose appropriate WiFi type.
  if(wifi_username == "")
  {
     wifi_connect_normal();
  }
  else
  {
     wifi_connect_enterprise(wifi_username, wifi_password);
  } 
}
