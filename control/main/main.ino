#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <WebSockets.h>
#include <SocketIOclient.h>
#include <WiFi.h>

#define RXD2 16
#define TXD2 17

// Enter WiFi details here
const char* ssid = "ENTER_WIFI_NAME_HERE";
const char* password = "ENTER_WIFI_PASSWORD_HERE";
int port = 80;

// Initializing webSocketsServer
WebSocketsServer webSocket = WebSocketsServer(port);


// Called when receiving any WebSocket message
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connection from ", num);
        Serial.println(ip.toString());
      }
      break;

    // Echo text message back to client
    case WStype_TEXT:
      Serial.printf("[%u] Instruction: %s\n", num, payload);
      Serial2.println("%s", payload)
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

void setup() {
    // Set up UART connection
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    // Connect to access point
    Serial.println("Connecting");
    WiFi.begin(ssid, password);
    while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

    // Print our IP address
    Serial.println("Connected!");
    Serial.print("My IP address: ");
    Serial.println(WiFi.localIP());

    // Start WebSocket server and assign callback
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
}

void loop() {
    webSocket.loop();
}
