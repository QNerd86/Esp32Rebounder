#define LED_PIN 14
#define SCHOCK_SENS_PIN 22
#define LED_COUNT 12

#include "BLEDevice.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID_1("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID charUUID_2("844d4687-bb1b-4c30-b230-6e0cb61e8924");
uint32_t baseColor = ring.Color(255, 255, 0);
uint32_t counter = 0; 
uint32_t deviceNumber = 2;
std::string msg ="Red";

//- - - - - - - - - Unabhängiger Code - - - - - - - - -

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static boolean shockSensHit = false;
static boolean onTheRow = false;

static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteChar_1;
BLERemoteCharacteristic* pRemoteChar_2;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};



//- - - - - - - - - BaseMethods - - - - - - - - -

void setup() {
  Serial.begin(115200);
  pinMode(SCHOCK_SENS_PIN, INPUT);
  ring.begin();
  ring.show();
  ring.setBrightness(100);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  colorWipe(ring.Color(255, 255, 255), 0, 2);
  Serial.print("End of setup.");
}  // End of setup.

void loop() {
  //Eingabe
  shockSensHit = digitalRead(SCHOCK_SENS_PIN);
  onTheRow = (counter == deviceNumber && connected);

  //Verarbeitung
  if (connected) {
    SendMessageToServer();
  }
  if (!connected) {
    TryToConnect();
  }

  //Ausabge
  SetLEDRing();
}






//- - - - - - - - - BLE Methods - - - - - - - - -
// Callback function for Notify function
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                           uint8_t* pData,
                           size_t length,
                           bool isNotify) {
  if (pBLERemoteCharacteristic->getUUID().toString() == charUUID_1.toString()) {

    // convert received bytes to integer
    counter = pData[0];
    for (int i = 1; i < length; i++) {
      counter = counter | (pData[i] << i * 8);
    }
  }
}
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    doConnect = false;
    Serial.println("onConnect");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    doScan = true;
    Serial.println("onDisconnect");
    colorWipe(ring.Color(255, 255, 255), 0, 1);
  }
};
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  connected = true;
  pRemoteChar_1 = pRemoteService->getCharacteristic(charUUID_1);
  pRemoteChar_2 = pRemoteService->getCharacteristic(charUUID_2);
  if (connectCharacteristic(pRemoteService, pRemoteChar_1) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pRemoteChar_2) == false)
    connected = false;

  if (connected == false) {
    pClient->disconnect();
    Serial.println("At least one characteristic UUID not found");
    return false;
  }
  return true;
}
bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  if (l_BLERemoteChar == nullptr) {
    Serial.print("Failed to find one of the characteristics");
    Serial.print(l_BLERemoteChar->getUUID().toString().c_str());
    return false;
  }
  Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));

  if (l_BLERemoteChar->canNotify())
    l_BLERemoteChar->registerForNotify(notifyCallback);

  return true;
}






//- - - - - - - - - Run Methods - - - - - - - - -

void SendMessageToServer() {
  if (shockSensHit) {
    msg = "Treffer";
    pRemoteChar_2->writeValue(msg, msg.length());
  } else {
    msg = "Kein Treffer";
    pRemoteChar_2->writeValue(msg, msg.length());
  }
}
void TryToConnect() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  } else {
    Serial.print("Scan");
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
}
void SetLEDRing() {
  if (!connected) {
    colorWipe(ring.Color(0, 0, 255), 0, 6);
  } 
  if (onTheRow) {
    colorWipe(baseColor, 0, 12);
  } 
  if(!onTheRow){
    colorWipe(baseColor, 0, 1);
  }


  if (shockSensHit) {
    ring.setPixelColor(6, ring.Color(255, 255, 255));
  }

  ring.show();
}
void colorWipe(uint32_t color, int wait, int count) {
  for (int i = 0; i < 12; i++) {
    if (count > i) {
      ring.setPixelColor(i, color);
    } else {
      ring.setPixelColor(i, ring.Color(0, 0, 0));
    }
    //ring.show();
    delay(wait);
  }
}
