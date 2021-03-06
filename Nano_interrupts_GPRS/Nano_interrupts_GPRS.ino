// GPRS code.
// 

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>  // This is used to help parse the response from time server


const int byte_number = 6;  // # of bytes per sesnor array reading
const int sensor_group_readings = 10;  // # of readings we will group together before writing to sd card'
const int readings_per_file = 864000;  // 10*60*60*24 = 864000
const String file_prefix = String("0528G");

const char* ssid     = "Mbrace_JSU";
const char* password = "alialiali1";
const char* host = "mbrace.xyz";
const int   port = 80;

File dataFile;
byte sensor_payload[byte_number*sensor_group_readings];
byte output_payload[byte_number*sensor_group_readings];
int day_counter = 0;

volatile int readings_in_file = 0;
volatile int payload_length = 0;
volatile bool payload_sent = false;

void send_payload(byte *payload, int payload_size);
int base64_encode(char *output, char *input, int inputLen);

void setup() {
  Serial.begin(9600); // higher speed 
  
  // WIFI Setup
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  readings_in_file = get_time_in_seconds() * 10;
  Serial.println("Connected") //Debug string hppens only once 
  
  // SD Card Setup
  SD.begin();
  dataFile = SD.open(file_prefix + String(day_counter), FILE_WRITE);  // Set file name to be created on SD card
  dataFile.write("Experiment specific Data: \r\n");
  dataFile.write("Date: 03/29/2018 \r\nLocation: GCRL \r\nCodeFile:Wemos_interrupts  \r\nDataFile: WiFiA.txt \r\n");
  dataFile.write("Comments: First WiFi experiemnt sending data to MBRACE.xyz, data_collector, with MAC as filename.\r\n\r\n\r\n");
  dataFile.flush();

  // I2C Setup
  Wire.begin();
  Wire.setClockStretchLimit(40000);  // In µs for Wemos D1 and Nano
  delay(100);  // Short delay, wait for the Mate to send back CMD
  
  // TIMER 1 for interrupt frequency 10 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 10 Hz increments
  OCR1A = 24999; // = 16000000 / (64 * 10) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 64 prescaler
  TCCR1B |= (0 << CS12) | (1 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts
}

void loop() {
  // If the payload is full, make a base64 encoded copy and send it over WIFI
  if (payload_length == byte_number*sensor_group_readings and !payload_sent) {
    // marking the current full payload as sent so that we don't send it again if
    //   this loop is called before the next interrupt wipes the payload array
    payload_sent = true;
    base64_encode((char*)output_payload, (char*)sensor_payload, payload_length);
    send_payload(output_payload, (payload_length)*4/3);
  }
}

// functions start here.

//ISR
ISR(TIMER1_COMPA_vect){
  // Writing sensor data to the SD card if the payload array is full.
  //   Once we are done, we reset the payload_length to 0 so the wifi send code
  //   needs to run before that happens. (approx 100ms window between the end of the
  //   interrupt(payload_length++) and the next call to the interrupt)
  if (payload_length == byte_number*sensor_group_readings) {
    open_file();
    dataFile.write("$$");
    dataFile.write(sensor_payload, payload_length);
    dataFile.flush();

    payload_length = 0;
    payload_sent = false;
  }
  // Reading sensors
  Wire.requestFrom(1, byte_number);
  while (Wire.available()) {
    for (int i = 0; i < byte_number; i++) {
      sensor_payload[payload_length] = Wire.read();
      payload_length++;
    }
    readings_in_file++;
  } 
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
  String start_string = String("GET /data_collector/?mac="+/*WiFi.macAddress()+*/"&data=");  // "POST http(s)://host:port/api/v1/0AaRkVNSDhBSdSC56AnS/telemetry"
  String middle_string = String((char*)payload);
  middle_string.replace("+", "%2B"); //Php can not send a +, it has to bre replaced by %2B (Major bug)
  String end_string   = String(" HTTP/1.1\r\nHost: ") + host + "\r\n\r\n";

// Serial.println("sending data: " + start_string + middle_string + end_string);  //Debug string

  client.print(start_string + middle_string + end_string);
}

int get_time_in_seconds(){
  WiFiClient wifi;
  HttpClient client = HttpClient(wifi, host, port);
  client.get("/current_time.php");
  return client.responseBody().toInt();
}

void open_file(){
  if(readings_in_file >= readings_per_file){
    day_counter++;
    readings_in_file = 0;
    dataFile = SD.open(file_prefix + String(day_counter), FILE_WRITE);  // Set file name to be created on SD card
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


  


