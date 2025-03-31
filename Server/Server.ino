#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Adafruit_MAX31865.h>

// BLE UUIDs
#define SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_ECG "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TEMP "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
BLECharacteristic *pEcgCharacteristic;
BLECharacteristic *pTempCharacteristic;
bool deviceConnected = false;

// EKG
const int ecgPin = 36;  // AD8232 output

// MAX31865 setup
#define MAX31865_CS 5
#define RREF 4300.0
#define RTD_NOMINAL 1000.0
Adafruit_MAX31865 max31865 = Adafruit_MAX31865(MAX31865_CS);

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(ecgPin, INPUT);

    // Start MAX31865 (3-wire eller 4-wire)
    max31865.begin(MAX31865_3WIRE);

    // Initialiser BLE
    BLEDevice::init("LNO_ESP32_EKG");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    // EKG-karakteristik
    pEcgCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_ECG,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    // Temperatur-karakteristik
    pTempCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TEMP,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    // Start servicen
    pService->start();

    // Start reklame
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    BLEDevice::startAdvertising();

    Serial.println("BLE er klar - forbind...");
}

void loop() {
    // Læs EKG
    int ecgValue = analogRead(ecgPin);
    char ecgStr[8];
    sprintf(ecgStr, "%d", ecgValue);
    pEcgCharacteristic->setValue(ecgStr);
    pEcgCharacteristic->notify();
    Serial.print("ECG: ");
    Serial.println(ecgStr);

    // Læs temperatur
    float temperature = max31865.temperature(RTD_NOMINAL, RREF);
    char tempStr[8];
    dtostrf(temperature, 4, 2, tempStr);
    pTempCharacteristic->setValue(tempStr);
    pTempCharacteristic->notify();
    Serial.print("Temp: ");
    Serial.println(tempStr);

    delay(10);  // Sampling rate på 100 Hz for EKG
}
