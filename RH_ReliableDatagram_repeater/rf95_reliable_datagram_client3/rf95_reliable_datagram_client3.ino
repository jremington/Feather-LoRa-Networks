// modified from example rf95_reliable_datagram_client
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_server
// Tested with Feather M0 LoRa

// Restructured client example to allow multiple client nodes. Repeater works with only two nodes at present

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#define REPEATER_ADDRESS 1
#define CLIENT_ADDRESS 3

//client 2 yellow antenna COM14
//client 3 red antenna COM9

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(rf95, CLIENT_ADDRESS);

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

  if (!manager.init()) {
    Serial.println("Init failed");
    //   rf95.printRegisters();    //uncomment to print radio registers for debugging
    while (1); //hang
  } else {
    Serial.println("Init OK");
  }

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

char buf[RH_RF95_MAX_MESSAGE_LEN];

void loop()
{
  Serial.print("To repeater: ");
  snprintf(buf, sizeof(buf), "Message from node %d", CLIENT_ADDRESS);
  Serial.print(buf);

  // Send message to the repeater. Times out after 3 retries, by default

  if (manager.sendtoWait((uint8_t *)buf, sizeof(buf), REPEATER_ADDRESS))
    Serial.println();
  else
    Serial.println(" !failed");

  // check for forwarded messages from repeater, with random wait times to avoid collisions

  uint16_t waitTime = random(1000, 3000);  //milliseconds
  // these to be updated by recvfromAckTimeout()
  uint8_t len = sizeof(buf);
  uint8_t from;

  //blocking call, times out after waitTime
  if (manager.recvfromAckTimeout((uint8_t *)buf, &len, waitTime, &from)) {
    //arguments: (uint8_t *buf, uint8_t *len, uint16_t timeout, uint8_t *source=NULL, uint8_t *dest=NULL,
    // uint8_t *id=NULL, uint8_t *flags=NULL, uint8_t* hops = NULL)

    // log ASCII message received
    if (len < sizeof (buf)) buf[len] = 0; // null terminate ASCII string
    else buf[len - 1] = 0;
    if (from == REPEATER_ADDRESS) {
      Serial.print("forwarded message: ");
      Serial.print(buf);
      Serial.println();
    }
  } //end recvfromAckTimeout
}
