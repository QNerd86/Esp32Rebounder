#define LED_PIN 14
#define SCHOCK_SENS_PIN 22
#define LED_COUNT 12
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEServer* pServer = NULL;                    // Pointer to the server
BLECharacteristic* pCharacteristic_1 = NULL;  // Pointer to Characteristic 1
BLECharacteristic* pCharacteristic_2 = NULL;  // Pointer to Characteristic 2
BLECharacteristic* pCharacteristic_3 = NULL;  // Pointer to Characteristic 2
BLECharacteristic* pCharacteristic_4 = NULL;  // Pointer to Characteristic 3
BLEDescriptor* pDescr_1;                      // Pointer to Descriptor of Characteristic 1
BLE2902* pBLE2902_1;                          // Pointer to BLE2902 of Characteristic 1
BLE2902* pBLE2902_2;                          // Pointer to BLE2902 of Characteristic 2
BLE2902* pBLE2902_3;                          // Pointer to BLE2902 of Characteristic 3
BLE2902* pBLE2902_4;                          // Pointer to BLE2902 of Characteristic 3

bool deviceConnected = false;
bool oldDeviceConnected = false;

bool deviceConnected1 = false;
bool deviceConnected2 = false;
bool deviceConnected3 = false;

uint32_t value = 0;
uint32_t connectedClientsCounter = 0;
uint32_t deviceNumber = 0;
uint32_t baseColor = ring.Color(0, 0, 255);
uint32_t counter = 0;

String actualValue1;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1 "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define CHARACTERISTIC_UUID_3 "844d4687-bb1b-4c30-b230-6e0cb61e8924"
#define CHARACTERISTIC_UUID_4 "ce5de39d-97f3-4017-bbff-d7e39ade44ca"

BLEService* pService;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    connectedClientsCounter++;
    Serial.println("a client connected " + String(connectedClientsCounter));

    if (connectedClientsCounter < 3) {
      BLEDevice::startAdvertising();
    }
    WhoIsConnected();
  };

  void onDisconnect(BLEServer* pServer) {
    connectedClientsCounter--;
    deviceConnected = (connectedClientsCounter > 0);
    BLEDevice::startAdvertising();
    Serial.println("Client disconnected " + String(connectedClientsCounter));
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(SCHOCK_SENS_PIN, INPUT);
  ring.begin();
  ring.show();
  ring.setBrightness(100);
  BLEDevice::init("ESP32");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  createBLECharacteristics();
  createBLEDescriptors();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  pCharacteristic_2->setValue("null");
  pCharacteristic_3->setValue("null");
  pCharacteristic_4->setValue("null");
}

void loop() {

  bool shockSensHit = digitalRead(SCHOCK_SENS_PIN);
  bool count = false;
  //Serial.println(counter);

  // notify changed value
  if (deviceConnected) {
    pCharacteristic_1->setValue(counter);
    pCharacteristic_1->notify();
    delay(10);
    if (counter == 0 && shockSensHit) {
      Serial.println("(counter == 1 && shockSensHit)  - ");
      count = true;
    }
    if (counter == 1) {
      if (GetValue1() == "Treffer") {
        Serial.println("rxValue_1.c_str()==Treffer");
        count = true;
      }
    }
    if (counter == 2) {
      if (GetValue2() == "Treffer") {
        Serial.println("rxValue_2.c_str()==Treffer");
        count = true;
      }
    }
    if (counter == 3) {
      if (GetValue3() == "Treffer") {
        Serial.println("rxValue_3.c_str()==Treffer");
        count = true;
      }
    }

    if(count)
    {
      counter++;
      counter = counter % 4;
    }

    if (counter == 0) {
      colorWipe(baseColor, 0, 12);
    } else {
      colorWipe(ring.Color(0, 0, 0), 0, counter + 1);
    }
  } else {
    colorWipe(ring.Color(255, 0, 0), 0, 12);  // Blue
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // Connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

void createBLECharacteristics() {
  pCharacteristic_1 = pService->createCharacteristic(
    CHARACTERISTIC_UUID_1,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_2 = pService->createCharacteristic(
    CHARACTERISTIC_UUID_2,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);


  pCharacteristic_3 = pService->createCharacteristic(
    CHARACTERISTIC_UUID_3,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_4 = pService->createCharacteristic(
    CHARACTERISTIC_UUID_4,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
}

void createBLEDescriptors() {
  pDescr_1 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_1->setValue("A very interesting variable");
  pCharacteristic_1->addDescriptor(pDescr_1);

  // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
  pBLE2902_1 = new BLE2902();
  pBLE2902_1->setNotifications(true);
  pCharacteristic_1->addDescriptor(pBLE2902_1);

  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);
  pCharacteristic_2->addDescriptor(pBLE2902_2);

  pBLE2902_3 = new BLE2902();
  pBLE2902_3->setNotifications(true);
  pCharacteristic_3->addDescriptor(pBLE2902_3);

  pBLE2902_4 = new BLE2902();
  pBLE2902_4->setNotifications(true);
  pCharacteristic_4->addDescriptor(pBLE2902_4);
  // Start the service
  pService->start();
}

void colorWipe(uint32_t color, int wait, int count) {
  for (int i = 0; i < 12; i++) {
    if (count > i) {
      ring.setPixelColor(i, baseColor);
    } else {
      ring.setPixelColor(i, ring.Color(0, 0, 0));
    }
    ring.show();
    delay(wait);
  }
}

String GetValue1() {
  int retrieCounter = 0;
  std::string rxValue_1 = pCharacteristic_2->getValue();
  while (rxValue_1 == "null" && retrieCounter < 60) {
    delay(50);
    rxValue_1 = pCharacteristic_2->getValue();
    retrieCounter++;
  }
  Serial.print("GetValue1 retries " + String(retrieCounter));
  Serial.println(rxValue_1.c_str());
  pCharacteristic_2->setValue("null");
  if (retrieCounter >= 60)
    return "Treffer";
  return rxValue_1.c_str();
}

String GetValue2() {
  int retrieCounter = 0;
  std::string rxValue_2 = pCharacteristic_3->getValue();
  while (rxValue_2 == "null" && retrieCounter < 60) {
    delay(50);
    rxValue_2 = pCharacteristic_3->getValue();
    retrieCounter++;
  }
  Serial.print("GetValue2 retries " + String(retrieCounter));
  Serial.println(rxValue_2.c_str());
  pCharacteristic_3->setValue("null");

  if (retrieCounter >= 60)
    return "Treffer";
  return rxValue_2.c_str();
}

String GetValue3() {
  int retrieCounter = 0;
  std::string rxValue_3 = pCharacteristic_4->getValue();
  while (rxValue_3 == "null" && retrieCounter < 100) {
    delay(50);
    rxValue_3 = pCharacteristic_4->getValue();
    retrieCounter++;
  }
  Serial.print("GetValue3 retries " + String(retrieCounter));
  Serial.println(rxValue_3.c_str());
  pCharacteristic_3->setValue("null");
  if (retrieCounter >= 60)
    return "Treffer";
  return rxValue_3.c_str();
}
