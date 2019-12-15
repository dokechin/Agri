#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <SPI.h>
#include <EPD1in54.h>
#include <EPDPaint.h>

#include <stdint.h>

// Device Name: Maximum 30 bytes
#define DEVICE_NAME "My ESP32"

// User service UUID: Change this to your generated service UUID
#define USER_SERVICE_UUID "1966e739-de61-4d30-8098-e5e70ffc687f"
// User service characteristics
#define READ_CHARACTERISTIC_UUID "E9062E71-9E62-4BC6-B0D3-35CDCD9B027B"
#define NOTIFY_CHARACTERISTIC_UUID "62FBD229-6EDD-4D1A-B554-5C4E1BB29169"

// PSDI Service UUID: Fixed value for Developer Trial
#define PSDI_SERVICE_UUID "E625601E-9E55-4597-A598-76018A0D293D"
#define PSDI_CHARACTERISTIC_UUID "26E2B12B-85F0-4F3F-9FDD-91D114270E6E"

#define COLORED     0
#define UNCOLORED   1

BLEServer* thingsServer;
BLESecurity *thingsSecurity;
BLEService* userService;
BLEService* psdiService;
BLECharacteristic* psdiCharacteristic;
BLECharacteristic* writeCharacteristic;
BLECharacteristic* notifyCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;
const int PIN_NUM = 34;
const int ANALOG_MAX = 4096;
const int TRY_COUNT = 5;

uint8_t data[24*7];
char humidityStr[24];

unsigned long prev = 0;

unsigned char image[40000];
EPDPaint paint(image, 0, 0);    // width should be the multiple of 8
EPD1in54 epd(33, 25, 26, 27); // reset, dc, cs, busy

class serverCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
   deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class readCallback: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *bleReadCharacteristic) {
    bleReadCharacteristic->setValue(data, sizeof(data));
    Serial.print("onRead:");
  }
};

void setup() {
  Serial.begin(9600);

  Serial.print("setup()");

  pinMode(12, OUTPUT);

  BLEDevice::init("");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

  // Security Settings
  BLESecurity *thingsSecurity = new BLESecurity();
  thingsSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_ONLY);
  thingsSecurity->setCapability(ESP_IO_CAP_NONE);
  thingsSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  setupServices();
  startAdvertising();
  Serial.println("Ready to Connect");

  if (epd.init(lutFullUpdate) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  
  epd.clearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.displayFrame();
  epd.clearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.displayFrame();
}

void loop() {
  unsigned long now = millis();
  // every 1 hours
  if (now - prev > 1000 * 60 * 60){
    
    prev = now;
    memcpy(data, &data[1], sizeof(data) - sizeof(int8_t));

    digitalWrite(12, HIGH);

    int sum = 0;
    for (int i=0;i<TRY_COUNT;i++){
      delay(1000);
      int value = analogRead(PIN_NUM);
      Serial.println(value);
      
      sum = sum + value;
    }
    
    digitalWrite(12, LOW);

    uint8_t humidity = ((sum / TRY_COUNT) * 255)/ ANALOG_MAX;
    humidity = ~humidity;
//  uint8_t humidity = random(256);
    data[24*7 -1] = humidity;

    sprintf(humidityStr, "%03d", humidity);
    Serial.println(humidityStr);

    paint.setRotate(ROTATE_0);
    paint.setWidth(200);
    paint.setHeight(200);
  
    paint.clear(UNCOLORED);
    paint.drawStringAt(0, 20, humidityStr, &Font24, COLORED);

    for(int i=0;i<168;i++){
      paint.drawLine(i + 15 , 199, i+ 16, int(199 - (data[i] /255.0) * 168), COLORED);    
    }
      
    epd.setFrameMemory(paint.getImage(), 0, 0, paint.getWidth(), paint.getHeight());
    epd.displayFrame();

  }
  // Disconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Wait for BLE Stack to be ready
    thingsServer->startAdvertising(); // Restart advertising
    oldDeviceConnected = deviceConnected;
  }
  // Connection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

void setupServices(void) {
  // Create BLE Server
  thingsServer = BLEDevice::createServer();
  thingsServer->setCallbacks(new serverCallbacks());

  // Setup User Service
  userService = thingsServer->createService(USER_SERVICE_UUID);
  // Create Characteristics for User Service
  writeCharacteristic = userService->createCharacteristic(READ_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
  writeCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  writeCharacteristic->setCallbacks(new readCallback());

  notifyCharacteristic = userService->createCharacteristic(NOTIFY_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  notifyCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  BLE2902* ble9202 = new BLE2902();
  ble9202->setNotifications(true);
  ble9202->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  notifyCharacteristic->addDescriptor(ble9202);

  // Setup PSDI Service
  psdiService = thingsServer->createService(PSDI_SERVICE_UUID);
  psdiCharacteristic = psdiService->createCharacteristic(PSDI_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
  psdiCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Set PSDI (Product Specific Device ID) value
  uint64_t macAddress = ESP.getEfuseMac();
  psdiCharacteristic->setValue((uint8_t*) &macAddress, sizeof(macAddress));

  // Start BLE Services
  userService->start();
  psdiService->start();
}

void startAdvertising(void) {
  // Start Advertising
  BLEAdvertisementData scanResponseData = BLEAdvertisementData();
  scanResponseData.setFlags(0x06); // GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  scanResponseData.setName(DEVICE_NAME);

  thingsServer->getAdvertising()->addServiceUUID(userService->getUUID());
  thingsServer->getAdvertising()->setScanResponseData(scanResponseData);
  thingsServer->getAdvertising()->start();
}
