#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <time.h>
#include <AiEsp32RotaryEncoder.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Definiere UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-0b62d4a4b1c8"

// WiFi und Anmeldedaten
#define EAP_IDENTITY "YOUR_C-ID"
#define EAP_USERNAME "YOUR_C-ID"
#define EAP_PASSWORD "YOUR_PASSWORD"
const char *ssid = "eduroam";

// Anzeige Einstellungen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define ROTARY_ENCODER_A_PIN_1 41
#define ROTARY_ENCODER_B_PIN_1 42

#define ROTARY_ENCODER_A_PIN_2 38
#define ROTARY_ENCODER_B_PIN_2 39

#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 2

#define EEPROM_SIZE 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
AiEsp32RotaryEncoder rotaryEncoder1 = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN_1, ROTARY_ENCODER_B_PIN_1, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
AiEsp32RotaryEncoder rotaryEncoder2 = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN_2, ROTARY_ENCODER_B_PIN_2, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

const char* ntpServer = "ntp1.uibk.ac.at";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

int weckzeitMinuten;
int weckzeitStunden;

bool alarmModus = true;
struct tm now;

int sda_pin = 8;
int scl_pin = 9;

// Variable zum Speichern der letzten Position
int lastPosition1 = 0;
int lastPosition2 = 0;

// Variable zum Speichern des aktuellen Positionswerts
int currentPosition1 = 0;
int currentPosition2 = 0;

// BLE Variablen
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

// Drehgeber
void IRAM_ATTR readEncoderISR1() {
  rotaryEncoder1.readEncoder_ISR();
}

void IRAM_ATTR readEncoderISR2() {
  rotaryEncoder2.readEncoder_ISR();
}

void setup() {
  Wire.setPins(sda_pin, scl_pin);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(2000);
  display.clearDisplay();
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to network: ");
  Serial.println(ssid);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address set: ");
  Serial.println(WiFi.localIP());

  // Initialisierung von BLE
  BLEDevice::init("ESP32_A");  // initialisierung BLE mit Gerätenamen
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12); 
  BLEDevice::startAdvertising();

  Serial.println("Bluetooth started. Waiting for connection...");
  
  if (weckzeitMinuten > 60) {
    weckzeitMinuten = 0;
    Serial.println("Minuten reset");
  }

  if (weckzeitStunden > 24) {
    weckzeitStunden = 0;
    Serial.println("Stunden reset");
  }

  // Drehgeber Einstellungen
  rotaryEncoder1.begin();
  rotaryEncoder1.setup(readEncoderISR1);
  rotaryEncoder1.setBoundaries(0, 23, false);  // minWert, maxWert
  rotaryEncoder1.setAcceleration(0);  // Keine Beschleunigung

  rotaryEncoder2.begin();
  rotaryEncoder2.setup(readEncoderISR2);
  rotaryEncoder2.setBoundaries(-1, 58, false);  // minWert, maxWert
  rotaryEncoder2.setAcceleration(0);  // Keine Beschleunigung

  EEPROM.begin(EEPROM_SIZE);
  weckzeitStunden = EEPROM.read(1);
  weckzeitMinuten = EEPROM.read(0);

  pinMode(47, INPUT); // Pin zum deaktivieren
  pinMode(20, INPUT); // Taster zum auslösen

  configTime(0, 0, ntpServer);
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  Serial.println("Zeit vom NTP-Server:");

  struct tm timeinfo;
if (getLocalTime(&timeinfo)) {
    Serial.printf("Stunde: %d, Minute: %d\n", timeinfo.tm_hour, timeinfo.tm_min);
} else {
    Serial.println("Fehler beim Abrufen der Zeit");
}
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  currentPosition1 = rotaryEncoder1.readEncoder();
  if (currentPosition1 != lastPosition1) {
    if (abs(currentPosition1 - lastPosition1) >= 1) {
      weckzeitStunden = currentPosition1;
      Serial.print("Weckzeit Stunden eingestellt auf: ");
      Serial.println(weckzeitStunden);
      lastPosition1 = currentPosition1;
      EEPROM.write(1, weckzeitStunden);
      EEPROM.commit();
    }
  }

  currentPosition2 = rotaryEncoder2.readEncoder();
  if (currentPosition2 != lastPosition2) {
    if (abs(currentPosition2 - lastPosition2) >= 1) {
      weckzeitMinuten = currentPosition2;
      Serial.print("Weckzeit Minuten eingestellt auf: ");
      Serial.println(weckzeitMinuten);
      lastPosition2 = currentPosition2;
      EEPROM.write(0, weckzeitMinuten);
      EEPROM.commit();
    }
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Z:" + String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min));

    if (digitalRead(47) == HIGH) {
    alarmModus = !alarmModus;
    delay(500);
  }

  if (deviceConnected) {
    Serial.println("Bluetooth verbunden. Bereit, ein Trigger-Signal zu senden.");
  }

    if (digitalRead(20) == HIGH) {
      Serial.println("GO");
      if (pCharacteristic != nullptr) {
        pCharacteristic->setValue("Trigger");
        pCharacteristic->notify();  
      }
      delay(100); 
  }

  if (alarmModus) {
    display.println("A:" + String(weckzeitStunden) + ":" + String(weckzeitMinuten + 1));
    if (timeinfo.tm_hour == weckzeitStunden && timeinfo.tm_min == weckzeitMinuten) {
      Serial.println("GO");
      delay(100);
      if (pCharacteristic != nullptr) {
        pCharacteristic->setValue("Trigger");
        pCharacteristic->notify();  
      }
      delay(100);  // Kurze Pause zur Stabilisierung der Schleife
    }
  } else {
    display.println("deact.");
    delay(100);  // Kurze Pause zur Stabilisierung der Schleife
  }

  display.display();
  delay(50);  // Verzögerung zur Reduzierung der Empfindlichkeit

  // Kontrolle der Verbindung
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);  // Zeit zum zurücksetzten
    pServer->startAdvertising();  
    Serial.println("Restarted advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
