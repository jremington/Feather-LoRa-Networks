// Direct text messaging between two LoRa nodes with fixed network address
// RHRouter manager application
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, 2 node routed reliable messaging client
// with the RHRouter class. It is designed to work with the other examples rf95_router_*

#include <RHRouter.h>
#include <RH_RF95.h>
#include <SPI.h>

#define CLIENT1_ADDRESS 1
#define CLIENT2_ADDRESS 2

//client1 yellow antenna COM14
//client2 white antenna COM12

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, CLIENT2_ADDRESS);

uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];
char message_buffer[80]; //message input buffer on Serial1

void setup()
{
  Serial.begin(115200);
  delay(2000); //wait for connection
  Serial1.begin(115200);
  delay(2000);
  Serial.println("Seria11 started");  //message input port

  // reset radio
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager.init()) {
    Serial.println("Client2 init failed");
    //   rf95.printRegisters();    //uncomment to print radio registers for debugging
    while (1); //hang
  } else {
    Serial.println("Client2 init OK");
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
  // assume network connectivity 1<->2
  // Manually define the routes for this network node, which can talk only to SERVER1

  manager.addRouteTo(CLIENT1_ADDRESS, CLIENT1_ADDRESS);
  while (Serial1.available()) char c = Serial1.read(); //empty Serial1 input buffer.
}

// loop() collects data from Serial1 and transmits text messages to receiver node

void loop()
{
  static uint8_t index = 0; //message buffer index
  static uint8_t message_to_send = 0;  //flag for something to send

  //input a message from Serial1 and send to client1
  // ***NOTE:*** lines in Serial1 input buffer longer than message_buffer will result in two or more transmissions

  while (Serial1.available()) {
    char c = Serial1.read();
    if ( (c == 13) or (index + 1 >= sizeof(message_buffer))) { //<CR> terminates line
      message_buffer[index] = 0; //terminate line
      message_to_send = 1; //done
      index = 0; //reset for next input line
      break;
    }
    else if (c >= ' ') {//printable char?
      message_buffer[index] = c;
      index++;
    }
  }

  if (message_to_send) {
    Serial.print("<Serial1: ");
    Serial.println(message_buffer);

    // Send a message to the endpoint
    // It will be routed by the intermediate
    // nodes to the destination node, according to the
    // routing tables in each node

    // C-string message length includes terminating zero byte
    if (manager.sendtoWait((uint8_t *)message_buffer, strlen(message_buffer) + 1, CLIENT1_ADDRESS) != RH_ROUTER_ERROR_NONE)
      Serial.println(" !send failed");
      else Serial.println(" OK");
  }
  message_to_send = 0;  //next round

  // check for message from other nodes, with random timeout
  uint16_t waitTime = random(1000, 3000);  //milliseconds
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAckTimeout(buf, &len, waitTime, &from))
  {
    Serial.print("<");
    Serial.print(from);
    Serial.print(": ");
    Serial.println((char*)buf);
  }
}
