#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <WebSockets.h>
#include <SocketIOclient.h>
#include <WiFi.h>

// UART_0 pins for ENERGY connection are predefined as Tx and Rx pins on ESP32

// UART_1 pins for DRIVE connection are defined here
#define RXD1 18 //D6
#define TXD1 19 //D5

// UART_2 pins for VISION connection are defined here 
#define RXD2 16 // D9
#define TXD2 17 // D8

// Enter WiFi details here
const char* ssid = "ENTER_WIFI_NAME_HERE";
const char* password = "ENTER_WIFI_PASSWORD_HERE";
int port = 80;

// Initializing webSocketsServer
WebSocketsServer webSocket = WebSocketsServer(port);

void setup() {
  // Set up UART connection
  Serial.begin(38400); // ENERGY connection
  Serial1.begin(38400, SERIAL_8N1, RXD1, TXD1); // DRIVE connection
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // VISIONN connection

  // Connect to access point
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {}

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

void loop() {
  webSocket.loop();

  // Sending ENERGY data back to COMMAND
  if (Serial.available()) {
    String energy_data_tmp = Serial.readString();
    int ed_length = energy_data_tmp.length();
    const char* energy_data = energy_data_tmp.c_str();
    webSocket.broadcastTXT(energy_data, ed_length);
  }
  
  // Sending DRIVE data back to COMMAND
  if (Serial1.available()) {
    String location_data_tmp = Serial1.readString();
    int ld_length = location_data_tmp.length();
    const char* location_data = location_data_tmp.c_str();
    webSocket.broadcastTXT(location_data, ld_length);
  }

  // Sending VISION data back to COMMAND
  if (Serial2.available()) {
    String vision_data_tmp = Serial2.readString();
    int vd_length = vision_data_tmp.length();
    const char* vision_data = vision_data_tmp.c_str();
    webSocket.broadcastTXT(vision_data, vd_length);
  }
  
}

// Called when receiving any WebSocket message
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      break;

    // New client has connected
    case WStype_CONNECTED:
      break;

    // Echo text message back to client
    case WStype_TEXT:
      sendInstr(payload);
      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

// Called when instruction is received from command module
void sendInstr(uint8_t* pyld) {
  String instr = (char*)pyld;
  int instr_last = instr.length()-1;
  char destination = instr[instr_last];
  instr.remove(instr_last);
  
  switch(destination) {
    // Energy Instruction
    case 'E':
      Serial.println(instr);
    // Drive Instruction
    case 'D': 
      Serial1.println(instr);
    // Vision Instruction
    case 'V':
      Serial2.println(instr);
    default:
      break;
  }
}
