// 본 예제는 Rob Tillaart 의 HX711.h 라이브러리 및 예제를 참조하여 수정되었습니다

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "HX711.h"

HX711 scale;

uint8_t dataPin = 7;
uint8_t clockPin = 6;

uint32_t start, stop;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
      }
        scale.tare();
    }
};


void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("True Meat");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
CHARACTERISTIC_UUID_TX,
BLECharacteristic::PROPERTY_NOTIFY
);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
 CHARACTERISTIC_UUID_RX,
BLECharacteristic::PROPERTY_WRITE
);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  scale.begin(dataPin, clockPin);
  scale.set_scale(5180.2);       // TODO you need to calibrate this yourself.
  scale.tare();
}

void loop() {

  if (deviceConnected) {
    TxWeight();
    delay(100); // bluetooth stack will go into congestion, if too many packets are sent
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
  // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
}


void TxWeight() {
    int16_t dataSensor = scale.get_units(5);
    std::string dataStr = std::to_string(dataSensor);
    const char* dataChars = dataStr.c_str();
    size_t length = strlen(dataChars);

    // 문자열을 바이트 배열로 변환
    uint8_t* dataBytes = new uint8_t[length + 1]; // 추가 공간 확보
    for (size_t i = 0; i < length; ++i) {
        dataBytes[i] = static_cast<uint8_t>(dataChars[i]);
    }
    dataBytes[length] = 0x0A; // 데이터 끝에 0x0A 추가

    // BLECharacteristic에 설정
    pTxCharacteristic->setValue(dataBytes, length + 1); // 배열의 크기를 length + 1로 설정
    pTxCharacteristic->notify();

    // 동적으로 할당된 메모리 해제
    delete[] dataBytes;
}
