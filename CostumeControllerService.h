#pragma once

#include <BLEDevice.h>

class BLEService;

class CostumeControlService : public BLECharacteristicCallbacks {
private:
  BLEService* _service = nullptr;

  uint8_t _tagPresent;
  uint8_t _tagWriteRequest;
  uint8_t _tagWriteError;

public:
  CostumeControlService(BLEServer* iServer);
  
  void onWrite(BLECharacteristic* characteristic);

  void setTagPresent(bool iPresent);

  /**
   * Check if the Central / client has requested a write 
   */
  bool getTagWriteRequest() { return _tagWriteRequest; };

  void clearWriteRequest();
};
