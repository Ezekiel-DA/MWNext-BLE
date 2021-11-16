#include "BLE.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "config.h"

void createCostumeControlService(BLEServer* server) {
  BLEService* costumeControlService = server->createService(MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID);



  costumeControlService->start();
}

