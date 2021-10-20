/*
 * 
 * Lora Relay Controller based on Arduino Uno
 * It will command a relay and also will have 
 * a push button to control manually the relay.
 * It has a LED to indicate the relay status.
 * 
 */

#include <SPI.h>
#include <LoRa.h>
#include <ezButton.h>

ezButton button1(3);
const int LED = 4;          
const int RELAY = 5;          
const int DEBOUNCE_DELAY = 70;
const int TIMEOUT = 5;
const int LONG_PRESS_TIME  = 2000; // 2000 milliseconds

byte localAddress = 0xDF;     // address of this device
byte destination = 0xDA;      // destination to send to
boolean asked;
boolean relaystatus;
byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
long interval = 60000;          // interval between sends
byte counterTimer = 0;
boolean timerRunning = false;
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Relay Pump Terra Nostra");
  pinMode(RELAY, OUTPUT);     // Initialize the RELAY pin as an output
  pinMode(LED, OUTPUT);     // Initialize the RELAY pin as an output

  button1.setDebounceTime(DEBOUNCE_DELAY);     
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  digitalWrite(RELAY, LOW);    // after restart make sure relay is off
  digitalWrite(LED, LOW);    // after restart make sure led is off
  lastSendTime = millis(); 
}

void loop() {

   button1.loop();
   
   checkReceived();

  // if relay status changed or house asked for report, report the relay status
  if (relaystatus != digitalRead(RELAY) || asked){
     String message = "led off";
     if (digitalRead(RELAY) == HIGH){
        message = "led on";
     } 
     LoRa.beginPacket();                   // start packet
     LoRa.write(destination);              // add destination address
     LoRa.write(localAddress);             // add sender address
     LoRa.write(msgCount);                 // add message ID
     LoRa.write(message.length());        // add payload length
     LoRa.print(message);                 // add payload
     LoRa.endPacket(true);                     // finish packet and send it
     msgCount++;                           // increment message ID
     Serial.println("Reporting to house control: " + message);
     asked = false;
  }
  relaystatus = digitalRead(RELAY);

  if(button1.isPressed()){
    Serial.println("The button is pressed");
    digitalWrite(RELAY, !digitalRead(RELAY));   // Toggle the relay
    digitalWrite(LED, !digitalRead(LED));   // Toggle the relay

    pressedTime = millis();
    timerRunning = false;
  }
  if(button1.isReleased()){
    Serial.println("The button is released");
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration > LONG_PRESS_TIME ){
      Serial.println("A long press is detected, turn on Relay and time it");
      digitalWrite(RELAY, HIGH);
      digitalWrite(LED, HIGH);
      timerRunning = true;
    }
  }
  checkTimer();

}

void checkReceived(){

  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {

     int recipient = LoRa.read();          // recipient address
     byte sender = LoRa.read();            // sender address
     byte incomingMsgId = LoRa.read();     // incoming msg ID
     byte incomingLength = LoRa.read();    // incoming msg length

    // received a packet
    String incoming = "";
    // read packet
    while (LoRa.available()) {
      //Serial.print((char)LoRa.read());
      incoming += (char)LoRa.read();
    }

      // if the recipient isn't this device or broadcast,
    if (recipient != localAddress) {
      Serial.println("This message is not for me.");
      return;                             // skip rest of function
    }
  
    if (incoming == "toggle"){
        digitalWrite(RELAY, !digitalRead(RELAY));   // Toggle the relay
        digitalWrite(LED, !digitalRead(LED));   // Toggle the relay
    }
  
    if (incoming == "asking"){
      asked = true;
    }else{
      asked = false;
    }

    // if message is for this device, or broadcast, print details:
    Serial.println("Received from: 0x" + String(sender, HEX));
    Serial.println("Sent to: 0x" + String(recipient, HEX));
    Serial.println("Message ID: " + String(incomingMsgId));
    Serial.println("Message length: " + String(incomingLength));
    Serial.println("Message: " + incoming);
    Serial.println("RSSI: " + String(LoRa.packetRssi()));
    Serial.println("Snr: " + String(LoRa.packetSnr()));
    Serial.println();
    
  } // end read package
  
}

void checkTimer(){
  if (timerRunning){
    if ((millis() - lastSendTime) > interval) {
      lastSendTime = millis();
      counterTimer++;
      Serial.println("Timer in minutes " + counterTimer);
    }
    delay(250);
    digitalWrite(LED, !digitalRead(LED));   // Toggle the relay
    delay(250);
    digitalWrite(LED, !digitalRead(LED));   // Toggle the relay
    if (counterTimer == TIMEOUT){
      Serial.println("Timeout");
      counterTimer = 0;
      digitalWrite(RELAY, LOW);
      digitalWrite(LED, LOW);
      timerRunning = false;
    }
  }
}
