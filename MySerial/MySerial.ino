// Use Wemos as a link to the GSM radio, SIM5320.
// Code uses ~ to send a <CTRL><Z>.
// Passive code.

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 0); // RX, TX
byte c;

void setup() {
  Serial.begin(115200);
  Serial.println("Goodnight moon!");
  mySerial.begin(115200);

  mySerial.println("at"); // Show AT, OK.
  delay(1000);
  
  digitalWrite(2, HIGH);  // SIM5320A_ON()
  delay(2000);//                            
  digitalWrite(2, LOW);
  delay(2000);
   
//  mySerial.println("AT+cmgf=1");
//  delay(500);
//  mySerial.println("AT+CMGS=\"+16013293217\"");
//  delay(500);
//  mySerial.println("Amin, Do you recognize this number?????"); 
//  mySerial.write( 0x1a ); // ctrl+Z character
//  delay(500);
}

void loop() { // Pass key strokes to SIM5320A
  if (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  if (Serial.available()) {
    byte b = Serial.read();
    if(b == 0x7e)
      mySerial.write(0x1a);
    else
      mySerial.write(b);  
  }
}
