// modified from example rf22_reliable_datagram_server2.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging repeater
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_client
// Tested with Feather M0 LoRa

// Simple example restructured to accommodate two client nodes (see client node list below)
// To generalize this to multiple nodes, the client message would have to contain the target node

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#define REPEATER_ADDRESS 1
// client node list
int client_nodes[2] = {2, 3}; 

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(rf95, REPEATER_ADDRESS);


void setup()
{
  Serial.begin(115200);
  while (!Serial) ; // Wait for serial port to be available

  // reset radio
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager.init())
    Serial.println("init failed");

  rf95.setTxPower(0, false);  //0 dBm for bench tests, 20 max
  rf95.setFrequency(915.0);
  rf95.setCADTimeout(500);

  // predefined configurations: see RH_RF95.h source code for the details
  // Bw125Cr45Sf128 (default, medium range, CRC on, AGC enabled)
  // Bw500Cr45Sf128
  // Bw31_25Cr48Sf512
  // Bw125Cr48Sf4096  long range, CRC and AGC on, low data rate
  /*
    // example long range configuration
      if (!rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
        Serial.println("Set config failed");
        while (1); //hang
        }
  */
}

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

void loop()
{
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      Serial.print("From ");
      Serial.print(from);
      Serial.print(": ");
      Serial.println((char*)buf);
     
      // forward the message to other client

      int forward = 0; //default = unrecognized node
      if (from == client_nodes[0]) forward = client_nodes[1];
      if (from == client_nodes[1]) forward = client_nodes[0];

      if (forward)
        if (manager.sendtoWait(buf, sizeof(buf), forward)) {
          Serial.print("Successfully forwarded to ");
          Serial.println(forward);
        }
        else {
          Serial.print("Forwarding failed to node ");
          Serial.println(forward);
        }
    }
  }
}
