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

const int relay = 5;          
const int DEBOUNCE_DELAY = 70;

byte localAddress = 0xDF;     // address of this device
byte destination = 0xDA;      // destination to send to
boolean asked;
boolean relaystatus;
byte msgCount = 0;            // count of outgoing messages

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Relay Pump Terra Nostra");
  pinMode(relay, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  button1.setDebounceTime(DEBOUNCE_DELAY);     
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  digitalWrite(relay, LOW);              // after restart make sure relay is off
}

void loop() {
   button1.loop();
   
   checkReceived();

  // if relay status changed or house asked for report, report the relay status
  if (relaystatus != digitalRead(relay) || asked){
     String message = "led off";
     if (digitalRead(relay) == HIGH){
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
  relaystatus = digitalRead(relay);

  if(button1.isPressed()){
    Serial.println("The button is pressed");
    digitalWrite(relay, !digitalRead(relay));   // Toggle the relay
  }
  if(button1.isReleased())
    Serial.println("The button is released");

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
        digitalWrite(relay, !digitalRead(relay));   // Toggle the relay
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
