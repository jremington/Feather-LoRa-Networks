//  modified from RadioHead RF95 simple encrypted server example, used fork of rweather library
// Working 4/2024 
// In order for this to compile you MUST uncomment the #define RH_ENABLE_ENCRYPTION_MODULE line
// at the bottom of RadioHead.h, AND you MUST have installed the Crypto directory from arduinolibs:
// http://rweather.github.io/arduinolibs/index.html
//  Philippe.Rochat'at'gmail.com
//  06.07.2017

#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <CryptoAES_CBC.h>  //fork of arduinolibs (simplified, ESP8266) https://github.com/Obsttube/CryptoAES_CBC
#include <AES.h>

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7
#define LED 13

RH_RF95 rf95(RFM95_CS, RFM95_INT);     // Instantiate a radio driver

AES128 aes128;
RHEncryptedDriver myDriver(rf95, aes128); // Instantiate the driver with those two

float frequency = 915.0; // Change the frequency here. 
unsigned char encryptkey[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; // The very secret key 

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for serial port to be available
  Serial.println("LoRa Simple Encrypted Server");
  if (!rf95.init())
    Serial.println("LoRa init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  aes128.setKey(encryptkey, 16);
  delay(4000);
  Serial.println("Setup completed");
}

void loop() {
  if (myDriver.available()) {
    // Should be a message for us now   
    uint8_t buf[myDriver.maxMessageLength()];
    uint8_t len = sizeof(buf);
    if (myDriver.recv(buf, &len)) {
      Serial.print("Received: ");
      Serial.println((char *)&buf);
    }
    else
    {
        Serial.println("recv failed");
    }
  }
}
