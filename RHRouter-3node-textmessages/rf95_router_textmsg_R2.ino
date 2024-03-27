// modified from rf22_router_REPEATER1.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple 3 node, addressed, hard-routed reliable messaging REPEATER
// with the RHRouter class.
//
// exchange text messages between CLIENT1 and CLIENT3, via REPEATER2
// CLIENT1_ADDRESS <-> REPEATER2_ADDRESS <-> CLIENT3_ADDRESS

#include <RHRouter.h>
#include <RH_RF95.h>
#include <SPI.h>

#define CLIENT1_ADDRESS 1
#define REPEATER2_ADDRESS 2
#define CLIENT3_ADDRESS 3

//client1 yellow antenna COM14
//repeater2 red antenna COM9
//client3 white antenna COM12

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, REPEATER2_ADDRESS);

uint8_t data[] = "Hello from repeater2";  //repeater node check message
uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

void setup()
{
  Serial.begin(115200);
  delay(2000); //startup pause
  // reset radio
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager.init()) {
    Serial.println("Init failed");
    //   rf95.printRegisters();    //uncomment to print radio registers for debugging
    while (1); //hang
  } else {
    Serial.println("Repeater2 init OK");
  }

  driver.setTxPower(0, false);  //0 dBm for bench tests, 20 max
  driver.setFrequency(915.0);
  driver.setCADTimeout(500);

  // predefined configurations: see RH_RF95.h source code for the details
  // Bw125Cr45Sf128 (default, medium range, CRC on, AGC enabled)
  // Bw500Cr45Sf128
  // Bw31_25Cr48Sf512
  // Bw125Cr48Sf4096  long range, CRC and AGC on, low data rate
  /*

    // example long range configuration
      if (!driver.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
        Serial.println("Set config failed");
        while (1); //hang
        }
  */
  // assume network connectivity is 1<->2<->3 exclusive  (only neighbor nodes visible)
  manager.addRouteTo(CLIENT1_ADDRESS, CLIENT1_ADDRESS);
  manager.addRouteTo(CLIENT3_ADDRESS, CLIENT3_ADDRESS);
}

// loop() checks for messages and forwards them if not addressed to this node
// if addressed to this node, the message is logged and a text response is sent.

void loop()
{
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAck(buf, &len, &from))
  { 
    //log messages addressed to this node. Other messages forwarded silently
    Serial.print("<");
    Serial.print(from);
    Serial.print(": ");
    Serial.print((char*)buf);

    // Send a reply back to the originator client
    if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
      Serial.println(" !reply failed");
      else Serial.println(" replied");
  }
}
