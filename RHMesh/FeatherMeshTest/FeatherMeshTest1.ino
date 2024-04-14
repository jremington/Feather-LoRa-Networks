// self-organizing standalone LoRa mesh network for Feather M0 LoRa modules  S. James Remington 2/2024
// Heavily modified from example code at https://nootropicdesign.com/projectlab/2018/10/20/lora-mesh-networking/
// github source https://github.com/nootropicdesign/lora-mesh
// start 2/20/2024, remove ATmega-specific stuff
// added Feather M0 specific pin defs and setup
// rewrote routing table formatter for simpler display
// added example of arbitrary node to node messages

//IMPORTANT NOTE: for an RHMesh network to self-organize, each node must attempt to communicate with all other nodes on a regular basis
// regardless of whether data are to be transmitted. In this way routes are established and maintained for the data communications.

#include <SPI.h>

#define RH_HAVE_SERIAL   //for debug print options in RadioHead mesh and router code
//#define RH_TEST_NETWORK 1  // if defined in RHRouter.h => forces limited connectivity test network 1-2-3-4

#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7
#define LED 13

#define N_NODES 4  //max 10 given RHRouter table dimensions

// *** hard coded node address *** (no EEPROM)
#define THIS_NODE 1  //white antenna

uint8_t nodeId;
uint8_t routes[N_NODES]; // route status for report, updated frequently
int16_t rssi[N_NODES]; // signal strength info

uint16_t packet_num = 0;  //optional packet ID included in message content

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
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  // while (!Serial) ; // Wait for debug serial port to be available
  delay(2000);  //no hanging for battery powered node
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  Serial1.begin(115200);
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

  manager = new RHMesh(rf95, nodeId);

  // manual reset
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!manager->init()) {
    Serial.println("Init failed");
    //   rf95.printRegisters();    //uncomment to print radio registers for debugging
    while (1); //hang
  } else {
    Serial.println("Init OK");
  }
  rf95.setTxPower(0, false);  //TODO check max
  rf95.setFrequency(915.0);
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
  Serial.println("RF95 ready");
  Serial.print("Max msglen: ");
  Serial.println(RH_MESH_MAX_MESSAGE_LEN);  //default 249

  // initialize tables for connectivity report
  for (uint8_t i = 0 ; i < N_NODES; i++) {
    routes[i] = 0;  //next hop or final dest
    rssi[i] = 0;
  }
}

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
// table informs only of next hop
void updateRoutingTable() {
  for (uint8_t n = 1; n <= N_NODES; n++) {
    RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
    if ( (n == nodeId) || route == NULL) { //fixed NULL pointer access sjr
      routes[n - 1] = 255;  //not visible from here
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

// log network hop/rssi table status info: this_node,(node,rssi) pairs
void printNodeInfo(uint8_t node, char *s) {
  Serial.print(node);
  Serial.print(' ');
  Serial.print(s);
  // Serial.print(freeMemory()); //check for leaks (none detected in trials)
  // Serial.print(" free");
  Serial.println();
}


void loop() {

  uint8_t flags = 0; //sendToWait() optional message flags
  static uint8_t n = 1;

  if (n != nodeId) { // not self

    updateRoutingTable();
    if (getRouteInfoString(report, sizeof(report))) {  //non zero = error
      //      Serial.println("getRouteInfoString: result truncated");
    }

    // Example of message with source, target, packet number and battery voltage
    unsigned long vbat = 200000UL * analogRead(VBATPIN) / (33UL * 1024); //mV
    // packet number is stored and incremented on a per node basis,
    snprintf(buf, sizeof (buf), "#%u: %lu mV", packet_num++, vbat);

    // send an acknowledged message to the target node
    // Note: sendtoWait times out after preset number of retries fails, see RHReliableDatagram.h
    uint8_t error = manager->sendtoWait((uint8_t *)buf, strlen(buf), n, flags);

    // log result
    Serial.print(">");
    Serial.print(n);
    /*  log sent message to console
    Serial.print(" {");  //added braces as message delimiters
    Serial.print(buf);
    Serial.print("}");
    */
    if (error != RH_ROUTER_ERROR_NONE) {  //print relevant error message, if any, or OK
      Serial.print(" !");
      Serial.println(getErrorString(error));
    }
    else {
      // we received an acknowledgement from the next hop to the target node.
      Serial.print(" Ack ");
      Serial.println(rf95.lastRssi());
    }
  }

  // network info report
  printNodeInfo(nodeId, report);
  manager->printRoutingTable(); // complete RadioHead routing table, less informative

  //Listen for incoming messages. To reduce collisions, wait a random amount of time
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

    // log ASCII message received, reporting last hop node
    if (via != from) { //add hop node number
      Serial.print("<");
      Serial.print(via); //last hop
    }
    Serial.print("<");
    Serial.print(from);
    //   Serial.print(" f:");
    //   Serial.print(flags, HEX);
    Serial.print(":");
    Serial.print(hops);
    Serial.print(" \""); //added quotes
    Serial.print(buf);
    Serial.println("\"");
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
