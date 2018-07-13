// Use Wemos as a link to the GSM radio, mySerial.
// Code uses ~ to send a <CTRL><Z>.
// Passive code.

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 0); // RX, TX

const char* host = "mbrace.xyz";
const int byte_number = 6;  // # of bytes per sesnor array reading
const int sensor_group_readings = 10;  // # of readings we will group together before writing to sd card'

byte c;
int web_call_progress = 0;
byte sensor_payload[byte_number*sensor_group_readings+8];  // 8 bytes for !! + millis + $$
byte output_payload[(byte_number*sensor_group_readings+8)*4/3];  // encoded string with time stamp !!$$
unsigned long quiet_start_time;
unsigned long current_time;

volatile int payload_length = 0;
volatile bool interrupted = false;

void ICACHE_RAM_ATTR onTimerISR();
void send_payload(byte *payload, int payload_size);
int base64_encode(char *output, char *input, int inputLen);

void setup() {
  Serial.begin(115200);
  Serial.println("Goodnight moon!");
  mySerial.begin(115200);

  digitalWrite(2, HIGH);  // mySerialA_ON()
  delay(2000);//                            
  digitalWrite(2, LOW);
  delay(2000);

  gprs_setup();

  // ISR Setup
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(100); // interrupt after 100? to get to ISR
}

void loop() { // Pass key strokes to mySerialA
  serial_passthrough();  // Pass serial data between wemos and gprs

  if (payload_length == byte_number*sensor_group_readings+8) {
    base64_encode((char*)output_payload, (char*)sensor_payload, payload_length);
    send_payload(output_payload);
    payload_length = 0;
  }

//  if(interrupted){
//    if(payload_length == 0){
//      unsigned long current_time = millis();
//      sensor_payload[0] = '@';
//      sensor_payload[1] = '@';
//      sensor_payload[2] = 1;
//      sensor_payload[3] = 2;
//      sensor_payload[4] = 3;
//      sensor_payload[5] = 4;
//      sensor_payload[6] = '#';
//      sensor_payload[7] = '#';
//      payload_length = 8;
//    }

    if(interrupted){
    if(payload_length == 0){
      unsigned long current_time = millis();
      sensor_payload[0] = '@';
      sensor_payload[1] = '@';
//      memcpy(&(sensor_payload[2]), &current_time, 4);
      sensor_payload[2] = (current_time >> 24) & 0xFF;
      sensor_payload[3] = (current_time >> 16) & 0xFF;
      sensor_payload[4] = (current_time >> 8) & 0xFF;
      sensor_payload[5] = current_time & 0xFF;
      sensor_payload[6] = '#';
      sensor_payload[7] = '#';
      payload_length = 8;
    }
    for (int i = 0; i < byte_number; i++) {
      sensor_payload[payload_length] = i;
      payload_length++;
    }
    interrupted = false;
  }
}

void serial_passthrough(){
  if (mySerial.available()){
    Serial.write(mySerial.read());
  }
  if (Serial.available()){
    c = Serial.read();
    if(c == 0x7e){
      mySerial.write(0x1a);
    } else {
      mySerial.write(c);
    }
  }
}

void ICACHE_RAM_ATTR onTimerISR(){
  timer1_write(500000);// We have been interrupted, come back in 100ms time<<<<<<< Updated upstream
  interrupted = true;
}

void send_payload(byte *payload) {
  byte request_string[10000];
  String start_string = String("GET /gprs/?name=gprs_device&data=");  // "POST http(s)://host:port/api/v1/0AaRkVNSDhBSdSC56AnS/telemetry"
  String middle_string = String((char*)payload);
  middle_string.replace("+", "%2B"); // Php can not send a +, it has to be replaced by %2B (Major bug)
  String end_string   = String(" HTTP/1.1\r\nHost: ") + host + "\r\n\r\n";
  gprs_http_send(start_string + middle_string + end_string);
}

void gprs_setup(){
  mySerial.println("AT+CGSOCKCONT=1,\"IP\",\"wap.cingular\"");
  wait_for_serial_response();

  mySerial.println("AT+CSOCKSETPN=1");
  wait_for_serial_response();

  mySerial.println("AT+NETOPEN");
  wait_for_serial_response();
  
  mySerial.println("AT+IPADDR");
  wait_for_serial_response();
}

void gprs_http_send(String http_string){
  mySerial.println("AT+CIPOPEN=0,\"TCP\",\"mbrace.xyz\",80");
  wait_for_serial_response();

  mySerial.print("AT+CIPSEND=0,");
  mySerial.println(http_string.length());
  wait_for_serial_response();
 
  mySerial.write(http_string.c_str());
  delay(50);
  mySerial.write(0x1a);
  wait_for_serial_response();
 
  mySerial.println("AT+CIPCLOSE=0");
  wait_for_serial_response();
}

void wait_for_serial_response(){
  read_response:
  while(!mySerial.available()){}
  while(mySerial.available()){
    Serial.write(mySerial.read());
  }
//  delay(1000);
  quiet_start_time = millis();
  quiet_time:
  current_time = millis();
  if(current_time - quiet_start_time > 1000){
    goto no_response;
  }
  if(mySerial.available()){
    goto read_response;
  }
  no_response:
  Serial.println("no response");
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
