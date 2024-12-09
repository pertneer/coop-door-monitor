#include <stdint.h>
#include <secrets.h>
#include <WiFi.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

// constants
#define DOOR_SENSOR_PIN 21  // ESP32 pin GPIO21 connected to door sensor's pin
#define doorLedGPin 12
#define doorLedRPin 13
#define TRIGGER_PIN 0
String message = "null";

// Variables
bool isConnected = false;
bool isOpened = false;

int door_state;       // current state of door sensor
int last_door_state;  // previous state of door sensor
const char *doorClosedMessage = "The door is closed";
const char *doorOpenMessage = "The door is open";
const char *doorClosed = "Closed";
const char *doorOpen = "Open";

WiFiClient wcClient;
PubSubClient client(wcClient);

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if(client.connect(ID, HA_USER, HA_PASS, TOPIC, willQoS, true, willMessage)) {
      client.subscribe(TOPIC);
      Serial.println("connected");
      Serial.print("Subcribed to: ");
      Serial.println(TOPIC);
      Serial.println("");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
    WiFi.hostname(hostname);
    WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    delay(10); // give a little time for serial to start
    Serial.println("\n Starting");
    
    WiFiManager wm;
    // Remove any previous network settings
    //wm.resetSettings();

    if(wm.autoConnect("AutoConnectAP")){
        delay(500); // needed to allow WiFi time to start
        Serial.println("WiFi is starting up");

        pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);  // set ESP32 pin to input pull-up mode
        pinMode(doorLedGPin, OUTPUT);
        pinMode(doorLedRPin, OUTPUT);

        door_state = digitalRead(DOOR_SENSOR_PIN);  // read state
        if(wm.autoConnect(hostname)){
            // place setup code here that relies on WiFi connection
            client.setServer(broker, 1883);
        }
        if (!client.connected()) {
            reconnect();
        }
        if (door_state == HIGH) { // state change: LOW -> HIGH
            Serial.println(doorOpenMessage);
            digitalWrite(doorLedGPin, LOW);
            digitalWrite(doorLedRPin, HIGH);
            // let me know the initial state of door
            client.publish(TOPIC, doorOpen);
            message = "Status message ";
            message += doorOpen;
            message += " sent";
            Serial.println(message); 
        }
        if (door_state == LOW) { // state change: HIGH -> LOW
            Serial.println(doorClosedMessage);
            digitalWrite(doorLedGPin, HIGH);
            digitalWrite(doorLedRPin, LOW);
            // let me know the initial state of door
            client.publish(TOPIC, doorClosed);
            message = "Status message ";
            message += doorClosed;
            message += " sent";
            Serial.println(message);  
        }
        
    }
    
}


void loop() {
    // if WiFi is connected lets log some information to serial and send a message to Telegraph
    if (WiFi.status() == WL_CONNECTED && !isConnected) {
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        digitalWrite(LED_BUILTIN, HIGH); // turn on onboard LED to indicate it is connected. 
        Serial.println(WiFi.localIP()),
        Serial.print("RRSI: ");
        Serial.println(WiFi.RSSI());
        // Grab IP address for use in message
        IPAddress ip = WiFi.localIP();
        // Convert IP to string
        String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
        message = "WiFi connected: ";
        message += ipStr;
        Serial.println(message);

        isConnected = true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(".");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(1000);
        isConnected = false;
    }
    if (!client.connected()) {
        reconnect();
    }
    // door switch monitoring
    last_door_state = door_state; // save the last state
    door_state  = digitalRead(DOOR_SENSOR_PIN); // read new state

    if (last_door_state == LOW && door_state == HIGH) { // state change: LOW -> HIGH
        Serial.println(doorOpenMessage);
        digitalWrite(doorLedGPin, LOW);
        digitalWrite(doorLedRPin, HIGH);
        // TODO: turn on alarm, light or send notification ...
        client.publish(TOPIC, doorOpen);
        Serial.println("Status message Online sent");
    }
    else
    if (last_door_state == HIGH && door_state == LOW) { // state change: HIGH -> LOW
        Serial.println(doorClosedMessage);
        digitalWrite(doorLedGPin, HIGH);
        digitalWrite(doorLedRPin, LOW);
        // send a message so I know it is closed
        client.publish(TOPIC, doorClosed);
        Serial.println("Status message Online sent"); 
    }
   
    client.loop();
}