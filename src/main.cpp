#include <secrets.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// constants
#define DOOR_SENSOR_PIN 21  // ESP32 pin GPIO21 connected to door sensor's pin
#define doorLedGPin 12
#define doorLedRPin 13
#define TRIGGER_PIN 0


// Variables
const char* hostname = "Coop Door Monitor";
bool isConnected = false;
bool isOpened = false;

int door_state;       // current state of door sensor
int last_door_state;  // previous state of door sensor
const char* doorClosedMessage = "The door is closed";
const char* doorOpenMessage = "The door is open";

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);


void setup() {
    WiFi.mode(WIFI_STA);

    Serial.begin(115200);
    delay(10); // give a little time for serial to start
    Serial.println("\n Starting");
    
    WiFiManager wm;
    // Remove any previous network settings
    wm.resetSettings();

    if(wm.autoConnect("AutoConnectAP")){
        WiFi.hostname(hostname);
        client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
        delay(500); // needed to allow WiFi time to start
        bot.sendMessage(CHAT_ID, "Coop Door Monitor started up", "");

        pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);  // set ESP32 pin to input pull-up mode
        pinMode(doorLedGPin, OUTPUT);
        pinMode(doorLedRPin, OUTPUT);

        door_state = digitalRead(DOOR_SENSOR_PIN);  // read state

        if (door_state == HIGH) { // state change: LOW -> HIGH
            Serial.println(doorOpenMessage);
            digitalWrite(doorLedGPin, LOW);
            digitalWrite(doorLedRPin, HIGH);
            // let me know the initial state of door
            bot.sendMessage(CHAT_ID, doorOpenMessage);
        }
        if (door_state == LOW) { // state change: HIGH -> LOW
            Serial.println(doorClosedMessage);
            digitalWrite(doorLedGPin, HIGH);
            digitalWrite(doorLedRPin, LOW);
            // let me know the initial state of door
            bot.sendMessage(CHAT_ID, doorClosedMessage);    
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
        String message = "WiFi connected: ";
        message += ipStr;
        bot.sendMessage(CHAT_ID, message, "");

        isConnected = true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(".");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(1000);
        isConnected = false;
    }

    // door switch monitoring
    last_door_state = door_state; // save the last state
    door_state  = digitalRead(DOOR_SENSOR_PIN); // read new state

    if (last_door_state == LOW && door_state == HIGH) { // state change: LOW -> HIGH
        Serial.println(doorOpenMessage);
        digitalWrite(doorLedGPin, LOW);
        digitalWrite(doorLedRPin, HIGH);
        // TODO: turn on alarm, light or send notification ...
        bot.sendMessage(CHAT_ID,doorOpenMessage);
    }
    else
    if (last_door_state == HIGH && door_state == LOW) { // state change: HIGH -> LOW
        Serial.println(doorClosedMessage);
            digitalWrite(doorLedGPin, HIGH);
        digitalWrite(doorLedRPin, LOW);
        // send a message so I know it is closed
        bot.sendMessage(CHAT_ID,doorClosedMessage);
    }
}

