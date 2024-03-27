// SJR: started from 
// https://github.com/LooUQ/Arduino-LoRa-Sniffer/blob/master/Arduino-LoRa-Sniffer.ino
// MIT License
// Copyright 2017 LooUQ Incorportated.
// Many thanks to Adafruit and AirSpayce.

// Feb/Mar 2024: Many mods to formatted dump.

/* Example sketch to capture all packets on a LoRa (RF95) radio network and send them to the
    serial port for monitoring or debugging a LoRa network.

    Output MODE can be verbose or delimited
        - verbose sends output as readable text output with each field labeled
        - delimited sends output as a compressed text string, delimited by a char of your choice
   Delimited is intended to be captured to a file on the serial host and displayed in an application
   such as Excel or OpenOffice
*/

enum OutputMode { verbose, delimited };

/* Define your desired output format here */
OutputMode mode = verbose; //delimited;
#define DELIMITER_CHAR '~'
/* end output format defintion */

#include <SPI.h>
#include <RH_RF95.h>
#include <stdio.h>

#define RH_FLAGS_ACK 0x80

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3


// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blink LED on packet receipt (Adafruit M0 here)
#define LED 13


int16_t rxCount = 0;                                        // packet counter
uint8_t rxBuffer[RH_RF95_MAX_MESSAGE_LEN];                  // receive buffer
uint8_t rxRecvLen;                                          // number of bytes actually received
char printBuffer[512] = "\0";                               // to send output to the PC
char formatString[] = {"%d~%d~%d~%d~%d~%d~%2x~%s~%d~%s"};
char legendString[] = "Rx Count~Rx@millis~LastRSSI~FromAddr~ToAddr~MsgId~HdrFlags~isAck~PacketLen~PacketContents";

int N_NODES = 6; //maximum expected number of LoRa network nodes (concerns only message display formatting)

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  while (!Serial);

  Serial.print("Feather LoRa Network Probe [Mode=");
  Serial.print(mode == verbose ? "verbose" : "delimited");
  Serial.println("]");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {                  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
    Serial.println("setFrequency failed");
    while (1);                                        // if can't set frequency, we are cooked!
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // need to setPromiscuous(true) to receive all frames
  rf95.setPromiscuous(true);

  // set delimeter
  if (mode == delimited) {
    if (DELIMITER_CHAR != '~') {
      for (int i = 0; i < sizeof(formatString); i++)
      {
        if (formatString[i] == '~')
          formatString[i] = DELIMITER_CHAR;
      }
      for (int i = 0; i < sizeof(legendString); i++)
      {
        if (legendString[i] == '~')
          legendString[i] = DELIMITER_CHAR;
      }
    }
    Serial.println(legendString);
  }
}


void loop()
{
  // wait for a lora packet
  rxRecvLen = sizeof(rxBuffer);               // RadioHead expects max buffer, will update to received bytes
  digitalWrite(LED, LOW);

  if (rf95.available())
  {
    digitalWrite(LED, HIGH);
    if (rf95.recv(rxBuffer, &rxRecvLen))
    {
      char isAck[4] = {0};
      if (rf95.headerFlags() & RH_FLAGS_ACK)
        memcpy(isAck, "Ack", 3);
      rxBuffer[rxRecvLen] = '\0';

      if (mode == delimited)
      {
        snprintf(printBuffer, sizeof(printBuffer), formatString, rxCount++, millis(), rf95.lastRssi(),
                 rf95.headerFrom(), rf95.headerTo(), rf95.headerId(), rf95.headerFlags(), isAck, rxRecvLen, rxBuffer);
        Serial.println(printBuffer);
      }
      else
      {
        snprintf(printBuffer, sizeof(printBuffer), "\n#%d @%d, RSSI= %d ",
                 rxCount++, millis(), rf95.lastRssi());
        Serial.print(printBuffer);
        snprintf(printBuffer, sizeof(printBuffer), "From: %d To: %d MsgId: %d  Flags: %2x %s %d bytes.",
                 rf95.headerFrom(), rf95.headerTo(), rf95.headerId(), rf95.headerFlags(), isAck, rxRecvLen);
        Serial.println(printBuffer);
        
        if ((rf95.headerFlags() & RH_FLAGS_ACK) == 0) { //skip if ACK
        // hex and/or ASCII dump of packet
        char hexbuf[6];
        int charcnt = 0;
		
        // hex dump
        for (int ic = 0; ic < rxRecvLen; ic++) {
          sprintf(hexbuf, "%02x ", rxBuffer[ic]);
          Serial.print(hexbuf);
          charcnt += 3;
          if (charcnt > 120) {
            Serial.println();
            charcnt = 0;
          }
        }
        if (charcnt > 0) Serial.println(); //terminate last line
        
        // ASCII dump
        for (int ic = 0; ic < rxRecvLen; ic++) {
          char c = rxBuffer[ic];
          if (c >= ' ' and c < 127) Serial.print(c);
          else Serial.print('.');
          charcnt++;
          if (charcnt > 120) {  //line wrap as in ASCII dump
            Serial.println();
            charcnt = 0;
          }
        }
        if (charcnt > 0) Serial.println();
        } //not ACK
      }
    }
  }
}
