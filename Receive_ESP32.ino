#include <Wire.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLE2902.h>

// Pins für die Verbindung zum DFPlayer Mini
#define DFPLAYER_RX_PIN 40
#define DFPLAYER_TX_PIN 1

DFRobotDFPlayerMini player;
SoftwareSerial mp3Serial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);

BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;
bool triggerReceived = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-0b62d4a4b1c8"

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    Serial.println("Connected to server.");
  }

  void onDisconnect(BLEClient* pClient) {
    Serial.println("Disconnected from server.");
    triggerReceived = false;
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client");
  
  // Initialisierung DFPlayer Mini
  mp3Serial.begin(9600);
  if (player.begin(mp3Serial)) {
    player.volume(30);
    Serial.println("DFPlayer Mini initialized.");
  } else {
    Serial.println(F("Connecting to DFPlayer Mini failed!"));
    return;
  }

  // Initialisierung BLE
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  BLEAddress serverAddress("DC:DA:0C:22:8B:C9");  // Adresse vom ESP_A

  Serial.println("Connecting to server...");
  pClient->connect(serverAddress);
  Serial.println("Connected to server.");

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service.");
    pClient->disconnect();
    return;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find characteristic.");
    pClient->disconnect();
    return;
  }

  pRemoteCharacteristic->registerForNotify(notifyCallback);
  Serial.println("Characteristic registered for notifications.");
}

void loop() {
  if (triggerReceived) {
    player.play(1);  // Spiel den Track Nummer 1
    Serial.println("PLay");
    delay(65000);
    triggerReceived = false;  // zurücksetzten des Singals
  }
  
  // Verbindung aufrechterhalten
  if (pClient->isConnected()) {
    delay(1000);
  } else {
    Serial.println("Reconnecting...");
    setup();
  }
}

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // Übersetzung von pData zu ASCII
  String value = String((char*)pData, length);
  Serial.println("Notification received: " + value);
  
  if (value == "Trigger") {
    triggerReceived = true;  // Das Signal zum spielen setzten
  }
}


