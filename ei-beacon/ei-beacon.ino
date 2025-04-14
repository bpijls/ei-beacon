/*
   Based on 31337Ghost's reference code from https://github.com/nkolban/esp32-snippets/issues/385#issuecomment-362535434   
*/

#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>
#include <esp_system.h>
#include <EEPROM.h>

#define SERVICE_UUID "7A0247E7-8E88-409B-A959-AB5092DDB03E"
#define CHARACTERISTIC_UUID "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"

#define CONFIG_BUTTON_PIN 9
#define EEPROM_SIZE 128

// The RGB LEDS are used for status indication
#define LED_PIN 5
#define LED_COUNT 2
#define LED_INTERVAL_MS 60000  // Light LEDs every 60000ms
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint16_t ledIntervalMs = 0; // timekeeping
uint32_t prevMillis = 0;

// Define hues to cycle through when the LED blinks
#define NHUES 6
uint8_t hueIndex = 0;
uint32_t hueColors[] = {
  pixels.Color(255, 0, 0),    // Red
  pixels.Color(0, 255, 0),    // green
  pixels.Color(0, 0, 255),    // blue
  pixels.Color(255, 255, 0),    // cyan
  pixels.Color(0, 255, 255),    // yellow
  pixels.Color(255, 0, 255)    //  Magenta
};

// Beacon name and UUID are stored in EEPROM
char beaconName[32];
char beaconUUID[64];

// BLE instances for the beacon
BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t connectionValue = 0;

// Reads beacon name and  UUID from EEPROM so it is still stored when the device is turned off
void readConfigFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++) beaconName[i] = EEPROM.read(i);
  for (int i = 0; i < 64; i++) beaconUUID[i] = EEPROM.read(i + 32);
  EEPROM.end();
}

void writeConfigToEEPROM(const char *name, const char *uuid) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++) EEPROM.write(i, name[i]);
  for (int i = 0; i < 64; i++) EEPROM.write(i + 32, uuid[i]);
  EEPROM.commit();
  EEPROM.end();
}

// Allows the beacon name and UUID to be entered using a serial connection
void enterConfigMode() {
  Serial.println("Enter Beacon Name:");
  while (!Serial.available())
    ;
  Serial.readBytesUntil('\n', beaconName, sizeof(beaconName));

  Serial.println("Enter Beacon UUID:");
  while (!Serial.available())
    ;
  Serial.readBytesUntil('\n', beaconUUID, sizeof(beaconUUID));

  writeConfigToEEPROM(beaconName, beaconUUID);
  Serial.println("Configuration saved. Restarting...");
  delay(1000);
  esp_restart();
}

// Fades the LEDs in and out successively to indicate the device is alive
void fadeLeds(uint32_t color) {
  const int steps = 50;
  const int delayMs = 15;

  for (int i = 0; i <= steps; i++) {
    int brightness = map(i, 0, steps, 0, 255);
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);
    pixels.fill(pixels.Color((r * brightness) / 255, (g * brightness) / 255, (b * brightness) / 255));
    pixels.show();
    delay(delayMs);
  }

  for (int i = steps; i >= 0; i--) {
    int brightness = map(i, 0, steps, 0, 255);
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);
    pixels.fill(pixels.Color((r * brightness) / 255, (g * brightness) / 255, (b * brightness) / 255));
    pixels.show();
    delay(delayMs);
  }

  pixels.clear();
  pixels.show();
}

// define callbacks for BLE server events: connect, disconnect
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("deviceConnected = true");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("deviceConnected = false");

    // Restart advertising to be visible and connectable again
    BLEAdvertising *pAdvertising;
    pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    Serial.println("iBeacon advertising restarted");
  }
};

// define callbacks for BLE characteristic events: connect, disconnect
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      Serial.println();
      Serial.println("*********");
    }
  }
};

// Initialize the BLE service that one can connect to
// The service UUID can also be used to filter out devices
void initService() {
  BLEAdvertising *pAdvertising;
  pAdvertising = pServer->getAdvertising();
  pAdvertising->stop();

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);    
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));

  // Start the service
  pService->start();

  pAdvertising->start();
}

// initialize the beacon
void initBeacon() {
  BLEAdvertising *pAdvertising;
  pAdvertising = pServer->getAdvertising();
  pAdvertising->stop();
  // iBeacon
  BLEBeacon myBeacon;
  myBeacon.setManufacturerId(0x4c00); // 0x4c00 is the apple manufacturer ID. This makes the BLE device register as an actual iBeacon
  myBeacon.setMajor(5); // Major and minor don't hold interesting information
  myBeacon.setMinor(88);
  myBeacon.setSignalPower(0xc5);
  myBeacon.setProximityUUID(BLEUUID(beaconUUID)); // set the name of the beacon

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x1A); // individual bits in this byte are used to set specific advertising properties (https://devzone.nordicsemi.com/f/nordic-q-a/29083/ble-advertising-data-flags-field-and-discovery)
  advertisementData.setManufacturerData(myBeacon.getData());
  pAdvertising->setAdvertisementData(advertisementData);

  pAdvertising->start(); // Start advertising
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  // When the built-in b"boot" button is pressed on startup
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP); 
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    enterConfigMode(); // Allow the user to enter name and UUID through serial
  }

  readConfigFromEEPROM(); // read name and UUID

  // set-up the beacon
  Serial.println();
  Serial.println("Initializing...");
  Serial.flush();

  BLEDevice::init(beaconName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  initService();
  initBeacon();

  Serial.println("iBeacon + service defined and advertising!");

  // set up the LEDS
  pixels.begin();
  pixels.clear();
  fadeLeds(pixels.Color(0, 0, 255));
}

void loop() {

  // count the number of connections made, just for fun
  if (deviceConnected) {
    Serial.printf("*** NOTIFY: %d ***\n", connectionValue);
    pCharacteristic->setValue(&connectionValue, 1);
    pCharacteristic->notify();
    connectionValue++;
  }

  delay(2000);

  // determine how much time elapsed since last LED fade
  if (ledIntervalMs > LED_INTERVAL_MS) {
    uint32_t color = hueColors[((hueIndex++)%NHUES)];
    fadeLeds(color);
    ledIntervalMs = 0;
  }

  // timekeeping
  uint32_t currentMillis = millis();
  ledIntervalMs += currentMillis - prevMillis;
  prevMillis = currentMillis;

}
