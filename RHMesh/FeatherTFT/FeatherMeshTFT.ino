
// self-organizing standalone LoRa mesh network for Feather M0 LoRa modules
// S. James Remington 2/2024
// Heavily modified from example code at https://nootropicdesign.com/projectlab/2018/10/20/lora-mesh-networking/
// github source https://github.com/nootropicdesign/lora-mesh

// This version requires a Featherwing 128x64 TFT display for message traffic and network status reports
// Six lines of 21 characters are currently in use, with spacers
// Two lines at the top of the display are reserved for node reports (up to four remote nodes)
// Two lines for outgoing data from this node (about 40 characters max)
// Two lines for incoming data from other nodes (about 40 characters max), updated as each packet arrives

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

#define RH_HAVE_SERIAL   //for debug print options in RadioHead mesh and router code
#define RH_TEST_NETWORK 1  // **if defined in RHRouter.h** => Test Network 1-2-3-4
#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>


// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7
#define LED 13

// Featherwing buttons 32u4, M0, M4, nrf52840, esp32-s2 and 328p
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

// Network and radio

#define N_NODES 4  //max 10 given RHRouter table dimensions
// *** hard coded node address *** (no EEPROM)
#define THIS_NODE 1  //white antenna

// low power for bench tests
int TxPower = 0;  //dBm
float RFfreq = 915.0;

// LoRa settings:
// Bw125Cr45Sf128 (library default, medium range, CRC on, AGC enabled)

uint8_t nodeId;
uint8_t routes[N_NODES]; // local routing table for network, target nodes or first hop nodes
int16_t rssi[N_NODES]; // signal strength info
uint16_t packet_num = 0;  //optional packet ID, included in message properties

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

// message buffers
char buf[RH_MESH_MAX_MESSAGE_LEN];
char report[100];

//place holder
byte readNodeId(void) {
  return THIS_NODE;
}

void setup() {
  randomSeed(analogRead(0) + analogRead(1) + analogRead(2)); //for now
  
  pinMode(BUTTON_A, INPUT_PULLUP); //not used (yet)
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);  //red LED off

  Serial.begin(115200);
  // while (!Serial) ; // Wait for debug serial port to be available
  delay(2000);  //no hanging for battery powered node

  Serial1.begin(115200);
  delay(2000);
  Serial.println("Seria11 started");  //message input port
  while (Serial1.available()) char c = Serial1.read(); //clear Serial1 input buffer

  // display
  //    delay(250); // wait for the OLED to power up
  display.begin(0x3C, true); // Address 0x3C default
  Serial.println("TFT init");

  nodeId = readNodeId();
  if (nodeId > 10) { //RHrouter table dimensioned to 10
    Serial.print("NodeId invalid: ");
    Serial.println(nodeId);
    digitalWrite(LED_BUILTIN, 1); //turn on red LED
    while (1); //hang
  }
  Serial.print("Initializing node ");
  Serial.println(nodeId);

  manager = new RHMesh(rf95, nodeId);  //instantiat mesh network

  // manual reset
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager->init()) {
    Serial.println("Init failed");
    //   rf95.printRegisters();    //uncomment to print radio registers for debugging
    digitalWrite(LED_BUILTIN, 1); //turn on red LED
    while (1); //hang
  } else {
    Serial.println("Init OK");
  }
  rf95.setTxPower(TxPower, false);  //20 dBm max
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
        digitalWrite(LED_BUILTIN,1); //red LED on
        while (1); //hang
        }
  */
  Serial.println("RF95 ready");
  Serial.print("Max msglen: ");
  Serial.println(RH_MESH_MAX_MESSAGE_LEN);  //default 249

  // initialize tables for connectivity report
  for (uint8_t i = 0 ; i < N_NODES; i++) {
    routes[i] = 0;  //next hop or final dest
    rssi[i] = 0;
  }

  // Set up the TFT
  display.clearDisplay();
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("FeatherMesh init OK");
  display.print("Node "); display.print(nodeId);
  display.print(" "); display.print(TxPower); display.print("dBm @ ");
  display.println(RFfreq, 1);
  display.display();
  delay(3000);
  display.clearDisplay();
}

//Network status report functions

// local routing table informs only of next hop and RSSI
void updateRoutingTable() {
  
  for (uint8_t n = 1; n <= N_NODES; n++) {
     RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
    if ( (n == nodeId) || route == NULL) { //fixed NULL pointer access sjr
      routes[n - 1] = 255;  //node not available
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
// entries with rssi = 0 denote first hop node to target

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
      itoa(routes[n - 1], stemp, 10); //target or next hop node
      if (strlen(p) + strlen(stemp) < len - 1) strcat(p, stemp);
      else return 1;
      if (strlen(p) + 1 < len - 1) strcat(p, ",");
      else return 1;
      itoa(rssi[n - 1], stemp, 10); //latest rssi from target or hop
      if (strlen(p) + strlen(stemp) < len - 1) strcat(p, stemp);
      else return 1;
      if (strlen(p) + 2 < len - 2) strcat(p, ") "); //terminate this node entry
      else return 1;
    }
  }  //end for n
  return 0;  //normal return
}

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
    result = strchr(s,' ');
    result++; //skip it
    result=strchr(result,' '); //second one.
    result++; //skip it
    display.write(s, result-s); //print to that point
    display.setCursor(0, 8);
    display.println(result); //print the rest on next line
  }
  else display.println(s);
    display.display();
  }

// translate sendToWait() error return to message
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

void loop() {

  uint8_t flags = 0; //sendToWait() optional message flags
  static uint8_t n = 1;

  if (n != nodeId) { // not self

    updateRoutingTable();
    if (getRouteInfoString(report, sizeof(report))) {  //non zero = error
      //      Serial.println("getRouteInfoString: result truncated");
    }
    // network info report at top
    printNodeInfo(nodeId, report);
    display.display();

    // Make up and send message with packet number and battery voltage
    unsigned long vbat = 200000UL * analogRead(VBATPIN) / (33UL * 1024); //mV
    // packet number is stored and incremented on a per node basis,
    snprintf(buf, sizeof (buf), "#%u: %lu mV", packet_num++, vbat);

    // sendToWait() -- send an acknowledged message to the target node
    // Note: sendtoWait times out after preset number of retries fails, see RHReliableDatagram.h
    uint8_t error = manager->sendtoWait((uint8_t *)buf, strlen(buf), n, flags);

    // log message and result
    display.drawRect(0, 19, 127, 16, SH110X_BLACK); //erase and skip 3 horizontal rows
    display.fillRect(0, 19, 127, 16, SH110X_BLACK);
    display.setCursor(0, 19);
    display.print(">"); display.print(n);
    display.print("="); display.print(buf);  //message delimiter "="
    if (error != RH_ROUTER_ERROR_NONE) {  //print relevant error message, if any, or OK
      display.print("= !");
      display.println(getErrorString(error));
    }
    else {
      // we received an acknowledgement from the next hop to the target node.
      display.print("= ");  //message delimiter and Ack
      display.println(rf95.lastRssi());
      display.display();
    }
  }

  // Listen for incoming messages. To reduce collisions, wait a random amount of time
  // then transmit again to the next node

  uint16_t waitTime = random(1000, 5000);  //milliseconds
  uint8_t len = sizeof(buf);
  uint8_t from;
  uint8_t hops;

  //blocking call:
  if (manager->recvfromAckTimeout((uint8_t *)buf, &len, waitTime, &from, NULL, NULL, &flags, &hops))
  {
    if (len < sizeof (buf)) buf[len] = 0; // null terminate ASCII string
    else buf[len - 1] = 0;

    // we received data from node 'from', but it may have arrived via an intermediate node
    uint8_t via = from;
    RHRouter::RoutingTableEntry *route = manager->getRouteTo(from);
    if (route != NULL ) {
      via = route->next_hop;
      if (via > 0 and via <= N_NODES) rssi[via - 1] = rf95.lastRssi();  //fix out of bounds writes
    }

    // log ASCII message received to display, reporting last hop node
    
    display.drawRect(0, 38, 127, 16, SH110X_BLACK); //skip 3 horizontal rows and
    display.fillRect(0, 38, 127, 16, SH110X_BLACK); //erase two lines
    display.setCursor(0, 38);
    if (via != from) { //add hop node number
      display.print("<");
      display.print(via); //last hop
    }
    display.print("<");
    display.print(from);
    display.print(":");
    display.print(hops);
    display.print("=");
    display.print(buf);
    display.println("=");
    display.display();
  }  //end recvfromAckTimeout
  n++; //next node
  if (n > N_NODES) n = 1;
}
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
