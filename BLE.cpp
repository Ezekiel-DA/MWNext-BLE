#include "BLE.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2904.h>
#include <BLE2902.h>

#include "config.h"

void attachUserDescriptionToCharacteristic(BLECharacteristic* iCharacteristic, const std::string& iName) {
  assert(iName.length() < 15); // enforce the limit below, because otherwise things break in annoying, hard to debug ways, if you forget about this limit...
  BLEDescriptor* descriptor = new BLEDescriptor((uint16_t)ESP_GATT_UUID_CHAR_DESCRIPTION, 15);
  descriptor->setValue(iName);

  iCharacteristic->addDescriptor(descriptor);
}

void setCharacteristicPresentationFormat(BLECharacteristic* iCharacteristic, uint8_t iType) {
  BLE2904* characteristicPresentationFormatDescriptor = new BLE2904();
  characteristicPresentationFormatDescriptor->setFormat(iType);
  characteristicPresentationFormatDescriptor->setNamespace(1); // mandatory? 1 = Bluetooth SIG Assigned Numbers
  characteristicPresentationFormatDescriptor->setUnit(0x2700); // unitless
  iCharacteristic->addDescriptor(characteristicPresentationFormatDescriptor);
}

BLECharacteristic* getCharacteristicByUUID(BLEService* iService, BLEUUID iCharacteristicID) {
  assert(iService);
  auto characteristic = iService->getCharacteristic(iCharacteristicID);
  assert(characteristic);
  return characteristic;
}

void ServerCallbacks::onConnect(BLEServer* server) {
  deviceConnected = true;
  Serial.println("Central connected. Start sending updates.");
}

void ServerCallbacks::onDisconnect(BLEServer* server) {
  deviceConnected = false;
  Serial.println("Central disconnected; Advertising again...");
  BLEDevice::startAdvertising();
}
