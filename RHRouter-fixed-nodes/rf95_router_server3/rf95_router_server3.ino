//modified from rf22_router_server3.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, routed reliable messaging server
// with the RHRouter class.
// It is designed to work with the other example rf22_router_client

#include <RHRouter.h>
#include <RH_RF95.h>
#include <SPI.h>

// In this small artifical network of 4 nodes,
// messages are routed via intermediate nodes to their destination
// node. All nodes can act as routers
// CLIENT_ADDRESS <-> SERVER1_ADDRESS <-> SERVER2_ADDRESS<->SERVER3_ADDRESS

#define CLIENT_ADDRESS 1   //COM12
#define SERVER1_ADDRESS 2  //COM14
#define SERVER2_ADDRESS 3  //COM9
#define SERVER3_ADDRESS 4  //COM11

//client 1 white antenna COM12
//server 1 yellow antenna COM14
//server 2 red antenna COM9
//server 3 black antenna COM11

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);


// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, SERVER3_ADDRESS);

uint8_t data[] = "And hello back to you from server3";
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
    Serial.println("Server 3 Init OK");
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
  // assume network connectivity 1<->2<->3<->4 exclusive  (only neighbor nodes visible)
  // Manually define the routes for server3 network node
  manager.addRouteTo(CLIENT_ADDRESS, SERVER2_ADDRESS);
  manager.addRouteTo(SERVER1_ADDRESS, SERVER2_ADDRESS);
  manager.addRouteTo(SERVER2_ADDRESS, SERVER2_ADDRESS);

  // check neighbor node connectivity
  char test[] = "Test msg to server1";
  if (manager.sendtoWait((uint8_t *)test, sizeof(test), SERVER1_ADDRESS) == RH_ROUTER_ERROR_NONE)
  {
    // It has been reliably delivered to the next node.
    // Now wait for a reply from the ultimate server
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAckTimeout(buf, &len, 3000, &from))
    {
      Serial.print("Reply from ");
      Serial.print(from);
      Serial.print(": ");
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println("No reply from server 1");
    }
  }
  else
    Serial.println("sendtoWait failed. Are the intermediate router servers running?");
}

// listen for messages to this node, and respond to the originator, via other nodess

void loop()
{
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAck(buf, &len, &from))
  {
    Serial.print("request from ");
    Serial.print(from);
    Serial.print(": ");
    Serial.println((char*)buf);

    // Send a reply back to the originator client
    if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
      Serial.println("sendtoWait failed");
  }
}
