#include "lightService.h"

#include <Arduino.h>
#include <BLE2904.h>
#include <BLE2902.h>

/**
 * Create a new BLE Descriptor with the 0x2901 UUID (i.e. "Characteristic User Description"), set it to the provided name, and attach it to the given BLE Characteristic.
 * Note: to save on memory, names should not exceed 15 characters.
 */
void attachUserDescriptionToCharacteristic(BLECharacteristic* iCharacteristic, const std::string& iName) {
  BLEDescriptor* descriptor = new BLEDescriptor((uint16_t)ESP_GATT_UUID_CHAR_DESCRIPTION, 15);
  descriptor->setValue(iName);

  iCharacteristic->addDescriptor(descriptor);
}

/**
 * DO NOT USE. This seems to break characteristics / services it's applied to? I guess I'm using it incorrectly, but I'm not sure why at this point.
 * Set various formatting information for a characteristic, via the Characteristic Presentation Format Descriptor (UUID: 0x2904).
 * Right now, only supports setting a type; the constants for this can be found in BLE2904.h from arduino-esp32
 */
void setCharacteristicPresentationFormat(BLECharacteristic* iCharacteristic, uint8_t iType) {
  BLE2904* characteristicPresentationFormatDescriptor = new BLE2904();
  characteristicPresentationFormatDescriptor->setFormat(iType);
  characteristicPresentationFormatDescriptor->setNamespace(1); // mandatory? 1 = Bluetooth SIG Assigned Numbers
  characteristicPresentationFormatDescriptor->setUnit(0x2700); // unitless
  iCharacteristic->addDescriptor(characteristicPresentationFormatDescriptor);
}

LightService::LightService(BLEServer* iServer, MWNEXTDeviceInfo& iDeviceInfo) : _deviceInfo(iDeviceInfo) {
_lightData.cycleColor = 0;
_lightData.patternID = 0;
_lightData.hue = 0;
_lightData.saturation = 255;

 // NB: the default of 15 handles (for Characteristics, Descriptors, etc.) is definitely not enough; 30 should do for a bit!
 // NB2: with this change, adding Presentation Format Descriptors would probably not break anymore!
_service = iServer->createService(_deviceInfo.uuid, 30);

BLECharacteristic* objectName = _service->createCharacteristic((uint16_t)0x2ABE, BLECharacteristic::PROPERTY_READ);
objectName->setValue(_deviceInfo.name);
//setCharacteristicPresentationFormat(objectName, BLE2904::FORMAT_UTF8);

auto type = _service->createCharacteristic(MWNEXT_BLE_DEVICE_TYPE_UUID, BLECharacteristic::PROPERTY_READ);
uint8_t type_tmp = static_cast<uint8_t>(_deviceInfo.type);
type->setValue(&type_tmp, 1);
attachUserDescriptionToCharacteristic(type, "Type");

BLECharacteristic* mode = _service->createCharacteristic(MWNEXT_BLE_MODE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
mode->setCallbacks(this);
uint8_t modeStartVal = _lightData.patternID;
mode->setValue(&modeStartVal, 1);
attachUserDescriptionToCharacteristic(mode, "Mode");
//setCharacteristicPresentationFormat(mode, BLE2904::FORMAT_UINT8);
mode->addDescriptor(new BLE2902());

if (_deviceInfo.type == MWNEXT_DEVICE_TYPE::RGB_LED) {
   BLECharacteristic* cycleColor = _service->createCharacteristic(MWNEXT_BLE_CYCLE_COLOR_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
   cycleColor->setCallbacks(this);
   uint8_t cycleColorStartVal = _lightData.cycleColor;
   cycleColor->setValue(&cycleColorStartVal, 1);
   attachUserDescriptionToCharacteristic(cycleColor, "Cycle color");
   //setCharacteristicPresentationFormat(cycleColor, BLE2904::FORMAT_BOOLEAN);
   cycleColor->addDescriptor(new BLE2902());

   BLECharacteristic* hue = _service->createCharacteristic(MWNEXT_BLE_HUE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
   hue->setCallbacks(this);
   hue->setValue(&_lightData.hue, 1);
   attachUserDescriptionToCharacteristic(hue, "Hue");
   //setCharacteristicPresentationFormat(hue, BLE2904::FORMAT_UINT8);
   hue->addDescriptor(new BLE2902());

    BLECharacteristic* saturation = _service->createCharacteristic(MWNEXT_BLE_SATURATION_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
    saturation->setCallbacks(this);
    saturation->setValue(&_lightData.saturation, 1);
    attachUserDescriptionToCharacteristic(saturation, "Saturation");
    //setCharacteristicPresentationFormat(saturation, BLE2904::FORMAT_UINT8);
    saturation->addDescriptor(new BLE2902());
}

_service->start();
};
  
void LightService::onWrite(BLECharacteristic* characteristic) {
    BLEUUID id = characteristic->getUUID();
    uint8_t* val = characteristic->getData();

    Serial.print("Got new value from central for Service "); Serial.print(this->_deviceInfo.name.c_str()); Serial.print(" for characteristic ");
    if (id.equals(std::string(MWNEXT_BLE_HUE_CHARACTERISTIC_UUID))) {
        Serial.print("Hue");
        this->_lightData.hue = (uint8_t) *val;
    } else if (id.equals(std::string(MWNEXT_BLE_CYCLE_COLOR_CHARACTERISTIC_UUID))) {
        Serial.print("Cycle Color");
        this->_lightData.cycleColor = (bool) *val;
    } else if (id.equals(std::string(MWNEXT_BLE_MODE_CHARACTERISTIC_UUID))) {
        Serial.print("Mode");
        this->_lightData.patternID = *val;
    } else if (id.equals(std::string(MWNEXT_BLE_SATURATION_CHARACTERISTIC_UUID))) {
        Serial.print("Saturation");
        this->_lightData.saturation = *val;
    } else {
        Serial.print("Unrecognized characteristic!!!");
    }

    Serial.print(": "); Serial.println(*val);
}

BLECharacteristic* LightService::getCharacteristicByUUID(BLEUUID iCharacteristicID) {
  assert(_service);
  auto characteristic = _service->getCharacteristic(iCharacteristicID);
  assert(characteristic);
  return characteristic;
}

void LightService::setHue(uint8_t iHue) {
  _lightData.hue = iHue;
  auto characteristic = getCharacteristicByUUID(BLEUUID(MWNEXT_BLE_HUE_CHARACTERISTIC_UUID));
  characteristic->setValue(&_lightData.hue, 1);
}

void LightService::setSaturation(uint8_t iSaturation) {
  _lightData.saturation = iSaturation;
  auto characteristic = getCharacteristicByUUID(BLEUUID(MWNEXT_BLE_SATURATION_CHARACTERISTIC_UUID));
  characteristic->setValue(&_lightData.saturation, 1);
}

void LightService::setPatternID(uint8_t iPatternID) {
  _lightData.patternID = iPatternID;
  auto characteristic = getCharacteristicByUUID(BLEUUID(MWNEXT_BLE_MODE_CHARACTERISTIC_UUID));
  characteristic->setValue(&iPatternID, 1);
}

void LightService::setCycleColor(bool iCycleColor) {
  _lightData.cycleColor = iCycleColor;
  uint8_t tempCycleColor = iCycleColor; // because we need to be able to take the address of this value converted to a uint8_t, which neither our input bool or this bitfield will allow
  auto characteristic = getCharacteristicByUUID(BLEUUID(MWNEXT_BLE_CYCLE_COLOR_CHARACTERISTIC_UUID));
  characteristic->setValue(&tempCycleColor, 1);
}

// void LightService::stupidUpdate(bool iDeviceConnected) {
    
//     _lightData.hue += 2;
//     characteristic->setValue(&(_lightData.hue), 1);
//     if (iDeviceConnected)
//       characteristic->indicate();
// }
