#include "CostumeControllerService.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2904.h>
#include <BLE2902.h>

#include "config.h"
#include "BLE.h"

CostumeControlService::CostumeControlService(BLEServer* iServer) {
  _service = iServer->createService(MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID);

  _tagPresent = 0;
  _tagWriteRequest = 0;
  _tagWriteError = 0;

  BLECharacteristic* tagPresent = _service->createCharacteristic(MWNEXT_BLE_TAG_PRESENT_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  uint8_t tagPresentStartVal = 0;
  tagPresent->setValue(&tagPresentStartVal, 1);
  attachUserDescriptionToCharacteristic(tagPresent, "Tag present");
  tagPresent->addDescriptor(new BLE2902());

  BLECharacteristic* tagWriteRequest = _service->createCharacteristic(MWNEXT_BLE_TAG_WRITE_REQUEST_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  tagWriteRequest->setCallbacks(this);
  uint8_t tagWriteRequestStartVal = 0;
  tagWriteRequest->setValue(&tagWriteRequestStartVal, 1);
  attachUserDescriptionToCharacteristic(tagWriteRequest, "Tag write req");
  tagWriteRequest->addDescriptor(new BLE2902());

  BLECharacteristic* tagWriteError = _service->createCharacteristic(MWNEXT_BLE_TAG_WRITE_ERROR_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  tagWriteError->setCallbacks(this);
  uint8_t tagWriteErrorStartVal = 0;
  tagWriteError->setValue(&tagWriteErrorStartVal, 1);
  attachUserDescriptionToCharacteristic(tagWriteError, "Tag write err");
  tagWriteError->addDescriptor(new BLE2902());

  _service->start();
}

void CostumeControlService::onWrite(BLECharacteristic* characteristic) {
    BLEUUID id = characteristic->getUUID();
    uint8_t* val = characteristic->getData();

    Serial.print("Got new value from central for Costume Control Service for characteristic ");
    if (id.equals(std::string(MWNEXT_BLE_TAG_WRITE_REQUEST_CHARACTERISTIC_UUID))) {
        Serial.print("Tag Write");
        this->_tagWriteRequest = (uint8_t) *val;
    } else if (id.equals(std::string(MWNEXT_BLE_TAG_WRITE_ERROR_CHARACTERISTIC_UUID))) {
        Serial.print("Tag Write Error");
        this->_tagWriteError = (bool) *val;
    } else {
        Serial.print("Unrecognized characteristic!!!");
    }

    Serial.print(": "); Serial.println(*val);
}

void CostumeControlService::setTagPresent(bool iPresent) {
  if (iPresent == _tagPresent)
    return; // don't set / notify if no change
  _tagPresent = iPresent;
  Serial.print("Setting tag presence flag to "); Serial.println(_tagPresent);
  auto characteristic = getCharacteristicByUUID(_service, BLEUUID(MWNEXT_BLE_TAG_PRESENT_CHARACTERISTIC_UUID));
  characteristic->setValue(&_tagPresent, 1);
  characteristic->notify();
}

void CostumeControlService::clearWriteRequest() {
  _tagWriteRequest = 0;
  auto characteristic = getCharacteristicByUUID(_service, BLEUUID(MWNEXT_BLE_TAG_WRITE_REQUEST_CHARACTERISTIC_UUID));
  characteristic->setValue(&_tagWriteRequest, 1);
  characteristic->notify();
}
