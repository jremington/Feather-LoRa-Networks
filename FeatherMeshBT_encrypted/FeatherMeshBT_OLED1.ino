//Self-organizing standalone RHMesh network for Feather M0 LoRa modules
// ping message type added to update network connectivity
// text message capability added on Serial1
// AES128 encryption for text messages added 4/17/2024. Works great!
// This version supports Feather with FeatherWing OLED display
//
// S. James Remington 4/2024
// Heavily modified from example code at https://nootropicdesign.com/projectlab/2018/10/20/lora-mesh-networking/
// github source https://github.com/nootropicdesign/lora-mesh

// This version permits arbitrary text messages from Serial1 input to be sent between nodes.
// e.g. via a Bluetooth connection to smart phone or laptop
// message format on Serial1 is "n:message<cr>"  (no quotes, n=target node).
// If there are only two nodes n: is not required and the "other node" is assumed

//IMPORTANT NOTE: for the RHMesh network to self-organize, each node must attempt to communicate with all other nodes on a regular basis
// regardless of whether data are to be transmitted. In this way routes are established and maintained as current for the data communications.

// *** OPTION DEFINES ***

//#define RH_HAVE_SERIAL   //for debug print options in RadioHead mesh and router code
#define FEATHER_TFT  //for TFT display
//#define USE_AES_ENCRYPTION

#define N_NODES 2  //max 10 given RHRouter table dimensions
#define THIS_NODE 1  //green antenna COM28 (TFT) *** hard coded node address *** (no EEPROM)

// 0dBm low power for bench tests, ~20 dBm max
int TxPower = 0;  //dBm
float RFfreq = 915.0;

// Network traffic logging options
//any or all of the below may be selected

#define RCV_MSG_OUTPUT_SERIAL
#define RCV_MSG_OUTPUT_SERIAL1
#define RCV_MSG_OUTPUT_TFT

#define LOG_NETWORK_INFO   //print routing and rssi report on Serial above
//#define LOG_ACK_NACK       //response to sent messages on Serial


#include <SPI.h>

#ifdef FEATHER_TFT
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// Featherwing buttons 32u4, M0, M4, nrf52840, esp32-s2 and 328p
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5  //used for screen erase
#endif

#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>

#ifdef USE_AES_ENCRYPTION
#include <RHEncryptedDriver.h>  //***must enable encryption module in RadioHead.h***
#include <CryptoAES_CBC.h> //fork of arduinolibs (simplified, ESP8266) https://github.com/Obsttube/CryptoAES_CBC
#include <AES.h>
unsigned char encryptkey[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}; // The secret encryption key
#endif

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7
#define LED 13
#define FM_PING 8  //message flag bit for ping message (network keep alive)

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

#ifdef USE_AES_ENCRYPTION
AES128 aes128;
RHEncryptedDriver encrypted_driver(rf95, aes128); // Instantiate the driver with those two
#endif

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

// global variables
uint8_t nodeId;
uint8_t routes[N_NODES]; // full routing table for mesh
int16_t rssi[N_NODES]; // signal strength info

// message buffers
char buf[RH_MESH_MAX_MESSAGE_LEN];  //general radio use
char report[80];  //network node report
char message_buffer[80];  //text message input buffer

uint8_t packet_num = 0;  //optional packet ID, can be sent alongside message
uint8_t message_to_send = 0;  //nonzero = flag that data or text message is ready to send, and target node ID.
uint8_t message_attempts_max = 5;  // five tries in outer loop (3 tries in RHMesh inner loop)

//place holder
byte readNodeId(void) {
  return THIS_NODE;
}

void setup() {
  randomSeed(analogRead(0) + analogRead(1) + analogRead(2)); //for now
  Serial.begin(115200);
 // while (!Serial) ; // Wait for debug serial port to be available
  delay(2000);  //no hanging for battery powered node
  Serial.println("Serial started");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);  //red LED off
  pinMode(LED, OUTPUT);
#ifdef FEATHER_TFT
pinMode(BUTTON_A, INPUT_PULLUP); //erase screen
pinMode(BUTTON_B, INPUT_PULLUP);
pinMode(BUTTON_C, INPUT_PULLUP);
#endif

  Serial1.begin(38400); //HC-06 Bluetooth
  delay(2000);

  Serial.println("Seria11 started");  //message input port
  while (Serial1.available()) char c = Serial1.read(); //clear Serial1 input buffer

  nodeId = readNodeId();
  if (nodeId > 10) { //RHrouter table dimensioned to 10
    Serial.print("NodeId invalid: ");
    Serial.println(nodeId);
    while (1); //hang
  }
  Serial.print("Initializing node ");
  Serial.println(nodeId);

// set up manager with selected driver, depending on USE_AES_ENCRYPTION

#ifdef USE_AES_ENCRYPTION
manager = new RHMesh(encrypted_driver, nodeId);
aes128.setKey(encryptkey, sizeof(encryptkey));
Serial.println("Encryption enabled");
#else
manager = new RHMesh(rf95, nodeId);  //instantiate mesh network
#endif

  // manual reset
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager->init()) {
    Serial.println("Radio manager init failed");
    //   rf95.printRegisters();    //uncomment to print radio registers for debugging
    while (1); //hang
  } else {
    Serial.println("Radio manager init OK");
  }

  rf95.setTxPower(TxPower, false);  //TODO check max
  rf95.setFrequency(RFfreq);
  rf95.setCADTimeout(500);

  // predefined configurations: see RH_RF95.h source code for the details
  // Bw125Cr45Sf128 (the chip default, medium range, CRC on, AGC enabled)
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
  Serial.print("RF95 ready. ");
  Serial.print("Max msglen: ");
  Serial.println(RH_MESH_MAX_MESSAGE_LEN);  //default 249

  // initialize tables for connectivity report
  for (uint8_t i = 0 ; i < N_NODES; i++) {
    routes[i] = 0;  //next hop or final dest
    rssi[i] = 0;
  }

#ifdef FEATHER_TFT
// Set up the TFT
Serial.println("TFT init");
display.begin(0x3C, true); // Address 0x3C default
display.clearDisplay();
display.setRotation(1);
display.setTextSize(1);
display.setTextColor(SH110X_WHITE);
display.setCursor(0, 0);
display.print("Node "); display.print(nodeId);
display.print(" "); display.print(TxPower); display.print("dBm @ ");
display.println(RFfreq, 1);

#ifdef USE_AES_ENCRYPTION
display.println("Encryption enabled");
#endif

  display.display();
  delay(3000);
  display.clearDisplay();
  Serial.println("Setup done");
  Serial.flush();
#endif

} //end setup

void loop() {

  static uint8_t n = 1;  //loop through the nodes. n = currently addressed target node
  static uint8_t index = 0; //message buffer index
  static uint8_t message_to_send = 0;  //flag for something to send
  static uint8_t message_target_node = 0;  //if text message, this is the node ID
  static uint8_t message_attempts = 0;

  uint8_t via = 0, flags = 0; //intermedidate node, sendToWait() optional message flags

  // check TFT buttons  BUTTON_C erases screen
#ifdef FEATHER_TFT
if (digitalRead(BUTTON_C) == LOW) display.clearDisplay();
#endif


  // *****
  //if activity on Serial1, input a text message to send
  // NOTE: lines in Serial1 input buffer longer than message_buffer will result in two or more transmissions
  // *****

  while (Serial1.available()) {
    char c = Serial1.read();
    
    if ( (c == 13) or (index + 1 >= sizeof(message_buffer))) { //<CR> terminates line
      message_buffer[index] = 0; //terminate character string
      
      if (N_NODES == 2) { //just pick the other node
        if (nodeId == 1) message_target_node = 2;
        else message_target_node = 1;
        message_to_send = 1;
      }
      else {
        message_target_node = atoi(message_buffer);
        if (message_target_node > 0 and message_target_node <= N_NODES) message_to_send = 1;
        if (message_target_node == nodeId) message_to_send = 0; //ignore message to self
      }

      message_attempts = 0;  //reset counter
      index = 0; //reset for next input line
      break;  //exit while
    }

    else if (c >= ' ') {//printable char?
      message_buffer[index] = c;
      index++;
    }
  }

  // *****
  // step through the nodes, send message or ping
  // *****

  if (n != nodeId) { // not self

    if (message_to_send and message_target_node == n) { //this node is the target

      manager->setRetries(3);  //default is 3
      // send an acknowledged message to the target node
      // Note: sendtoWait times out after preset number of retries fails, see RHReliableDatagram.h
      uint8_t error = manager->sendtoWait((uint8_t *)message_buffer, strlen(message_buffer), message_target_node, flags);

#ifdef LOG_ACK_NACK        // log ACK/NACK result
Serial.print(">");
Serial.print(n);
#endif

      if (error != RH_ROUTER_ERROR_NONE) {  //print relevant error message, if any, or continue

#ifdef LOG_ACK_NACK        // log ACK/NACK result
Serial.print(" !");
Serial.println(getErrorString(error));
#endif

        // *** reset message_to_send, or keep trying?
        message_attempts++;
        if (message_attempts == message_attempts_max) message_to_send = 0; //give up
      }

      else {
        // we received an acknowledgement from the next hop to the target node.
        RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
        if (route != NULL ) {
          via = route->next_hop;
          if (via > 0 and via <= N_NODES) rssi[via - 1] = rf95.lastRssi();  //update rssi
        }

#ifdef LOG_ACK_NACK
Serial.print(" Ack ");
Serial.println(rf95.lastRssi());
#endif

        message_to_send = 0;  //reset send flag
      }  //end else (no ACK error)
    }  //end if message_to_send


    else {  // no message, just ping node

      flags = FM_PING;  //inform the other node that this is just a ping
      strcpy(buf, "!");  //the message content
      manager->setRetries(1); //one retry for ping messages
      uint8_t error = manager->sendtoWait((uint8_t *)buf, 1 , n, flags); //one byte message

      //update rssi?
      if (error == RH_ROUTER_ERROR_NONE) {  //get last intermediate address
        RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
        if (route != NULL ) {
          via = route->next_hop;
          if (via > 0 and via <= N_NODES) rssi[via - 1] = rf95.lastRssi();
        }
      }
    } //end ping

#ifdef LOG_NETWORK_INFO
//log local node info report
if (getRouteInfoString(report, sizeof(report))) {  //non zero = error
//      Serial.println("getRouteInfoString: result truncated");
}
printNodeInfo(nodeId, report);
//   manager->printRoutingTable(); // debug: print complete RadioHead routing table, less informative
#endif
}  //end (n != nodeId)

    updateRoutingTable();  //update current routes from network manager

    // *****
    // Listen for incoming messages. To reduce collisions, wait a random amount of time
    // then transmit again to the next node
    // *****

    uint16_t waitTime = random(2000, 5000);  //milliseconds
    // these to be updated by recvfromAckTimeout()
    uint8_t len = sizeof(buf);
    uint8_t from;
    uint8_t hops;

    //blocking call, times out after waitTime
    if (manager->recvfromAckTimeout((uint8_t *)buf, &len, waitTime, &from, NULL, NULL, &flags, &hops)) {
      //arguments: (uint8_t *buf, uint8_t *len, uint16_t timeout, uint8_t *source=NULL, uint8_t *dest=NULL,
      // uint8_t *id=NULL, uint8_t *flags=NULL, uint8_t* hops = NULL)

      // we received data from node 'from', but it may have arrived via an intermediate node
      // note that there may be more than one route to this node, and the message could have arrived via a different node
      via = from;
      RHRouter::RoutingTableEntry *route = manager->getRouteTo(from);
      if (route != NULL ) {
        via = route->next_hop;
        if (via > 0 and via <= N_NODES) rssi[via - 1] = rf95.lastRssi();
      }

      if ((flags & FM_PING) == 0) { //if not a ping, do something with the message

        // log ASCII message received, reporting last hop node

        if (len < sizeof (buf)) buf[len] = 0; // null terminate ASCII string
        else buf[len - 1] = 0;

#ifdef RCV_MSG_OUTPUT_SERIAL
if (via != from) { //add hop node and number
Serial.print("<");
Serial.print(via); //last hop
}
Serial.print("<");
Serial.print(from);
Serial.print(":");
Serial.print(hops);
Serial.print(" \""); //added quotes
Serial.print(buf);
Serial.println("\"");
#endif

#ifdef RCV_MSG_OUTPUT_SERIAL1
Serial1.print(buf);  //just the message
Serial1.println();
#endif

#ifdef FEATHER_TFT
#ifdef RCV_MSG_OUTPUT_TFT

// log ASCII message received to display, reporting last hop node
        display.drawRect(0, 19, 127, 32, SH110X_BLACK); //skip 2 horizontal rows and
        display.fillRect(0, 19, 127, 32, SH110X_BLACK); //erase four lines
        display.setCursor(0, 19);
        display.println(buf);
        display.display();
#endif
#endif
}  //process message
} //end recvfromAckTimeout

        n++; //next node
        if (n > N_NODES) n = 1;

      }  //end loop()

      // ***** utility functions *****

      // translate error number to message
      const char* getErrorString(uint8_t error) {
        switch (error) {
          case 1: return "invalid length";
            break;
          case 2: return "no route";
            break;
          case 3: return "timeout";
            break;
          case 4: return "no reply";
            break;
          case 5: return "unable to deliver";
            break;
        }
        return "unknown error";
      }


      // local network status table informs only of next hop
      void updateRoutingTable() {
        for (uint8_t n = 1; n <= N_NODES; n++) {
          RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
          if ( (n == nodeId) || route == NULL) { //fixed NULL pointer access sjr
            routes[n - 1] = 255;
            rssi[n - 1] = 0;
          }
          else {
            routes[n - 1] = route->next_hop;
          }
        }
      }

      // ASCII report of routing info: (node,rssi) pairs, ACCESSIBLE nodes only
      // return 0: normal completion
      // return 1: string terminated early, anticipated buffer overflow.
      // example output 1 2(2,-35) 3(3,-66) 4(4,-57)  if all three remote nodes directly reachable
      // example output 1 2(2,-35) 3(2,0) 4(2,0)  node 2 is first hop to nodes 3 and 4

      uint8_t getRouteInfoString(char *p, size_t len) {
        char stemp[6]; //temp buffer for int conversion
        p[0] = 0; //initialize C-string

        for (uint8_t n = 1; n <= N_NODES; n++) {
          if (routes[n - 1] != 255) { //valid path to node?

            itoa(n, stemp, 10); //target node
            if (strlen(p) + strlen(stemp) < len - 1) strcat(p, stemp);
            else return 1; //buffer overflow avoided

            if (strlen(p) + 1 < len - 1) strcat(p, "("); // (n,rssi) pairs
            else return 1;

            itoa(routes[n - 1], stemp, 10); //next hop node
            if (strlen(p) + strlen(stemp) < len - 1) strcat(p, stemp);
            else return 1;

            if (strlen(p) + 1 < len - 1) strcat(p, ",");
            else return 1;

            itoa(rssi[n - 1], stemp, 10); //rssi info
            if (strlen(p) + strlen(stemp) < len - 1) strcat(p, stemp);
            else return 1;

            if (strlen(p) + 2 < len - 2) strcat(p, ") ");
            else return 1;
          }
        }  //end for n
        return 0;  //normal return
      }
#ifdef FEATHER_TFT

      // Log network hop/rssi table status info:
      // target(hop,rssi) entries separated by blanks
      // Two lines accommodate only four remote nodes (five total).
      // Note for debugging:
      // manager->printRoutingTable(); //prints complete RadioHead routing table, less informative

      void printNodeInfo(uint8_t node, char *s) {
        display.drawRect(0, 0, 127, 16, SH110X_BLACK);  //erase two lines
        display.fillRect(0, 0, 127, 16, SH110X_BLACK);
        display.setCursor(0, 0);
        if (strlen(s) > 19) { //try to neatly arrange long table in two lines
          char * result; //pointer to locate second blank delimiter, start of second line
          result = strchr(s, ' ');
          result++; //skip it
          result = strchr(result, ' '); //second one.
          result++; //skip it
          display.write(s, result - s); //print to that point
          display.setCursor(0, 8);
          display.println(result); //print the rest on next line
        }
        else display.println(s);
        display.display();
      }
#else

      // log network hop/rssi table status info: this_node,(node,rssi) pairs
      // VISIBLE nodes only
      void printNodeInfo(uint8_t node, char *s) {
        Serial.print(node);
        Serial.print(' ');
        Serial.println(s);
      }
#endif

      /*
        // monitor free memory

        #ifdef __arm__
        // should use uinstd.h to define sbrk but Due causes a conflict
        extern "C" char* sbrk(int incr);
        #else  // __ARM__
        extern char *__brkval;
        #endif  // __arm__

        int freeMemory() {
        char top;
        #ifdef __arm__
        return &top - reinterpret_cast<char*>(sbrk(0));
        #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
        return &top - __brkval;
        #else  // __arm__
        return __brkval ? &top - __brkval : &top - __malloc_heap_start;
        #endif  // __arm__
        }
      */
