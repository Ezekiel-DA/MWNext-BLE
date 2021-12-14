#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <SPI.h>
#include <MFRC522.h>

#include <AceButton.h>
using namespace ace_button;

#include "config.h"
#include "utils.h"
#include "rfid.h"
#include "buttons.h"
#include "BLE.h"
#include "CostumeControllerService.h"
#include "lightService.h"

// CAPABILITY_COLOR      
// CAPABILITY_PATTERN    
// CAPABILITY_PWM        
// CAPABILITY_ADDRESSABLE

MWNEXTDeviceInfo MWNEXTDevices[] = {
  {.name="Windows", .deviceId=1, .capabilities= CAPABILITY_COLOR | CAPABILITY_PATTERN | CAPABILITY_PWM | CAPABILITY_ADDRESSABLE },
  {.name="Clouds",  .deviceId=2, .capabilities= CAPABILITY_COLOR | CAPABILITY_PATTERN | CAPABILITY_PWM | CAPABILITY_ADDRESSABLE },
  {.name="Walls",   .deviceId=3, .capabilities= CAPABILITY_PATTERN | CAPABILITY_PWM },
  {.name="Moat",    .deviceId=4, .capabilities= CAPABILITY_COLOR | CAPABILITY_PATTERN | CAPABILITY_PWM | CAPABILITY_ADDRESSABLE },
  {.name="Stars",   .deviceId=5, .capabilities= CAPABILITY_PATTERN } // do we want to set caps to null here? i.e. on/off only, not even patterns?
};
const uint8_t NUM_MWNEXT_BLE_SERVICES = sizeof(MWNEXTDevices) / sizeof(MWNEXTDeviceInfo);

CostumeControlService* costumeController = nullptr;

MFRC522 rfid(RFID_READER_CS_PIN, UINT8_MAX); // RST pin (NRSTPD on MFRC522) not connected; setting it to this will let the library switch to using soft reset only
static LightService* MWNEXTServices [NUM_MWNEXT_BLE_SERVICES];

void setup() {
  Serial.begin(115200);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  setupButtons();
  
  SPI.begin();
  rfid.PCD_Init();

  BLEDevice::init("MagicWheelchair-Next");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  
  // disable this for now, to stay under what appears to be a 6 user services limit (https://github.com/espressif/esp-idf/issues/5495)
  // BLEService* devInfoService = server->createService((uint16_t)ESP_GATT_UUID_DEVICE_INFO_SVC);
  // BLECharacteristic* manufacturerID = devInfoService->createCharacteristic((uint16_t)ESP_GATT_UUID_MANU_NAME, BLECharacteristic::PROPERTY_READ);
  // manufacturerID->setValue("Team Freyja");
  // devInfoService->start();

  costumeController = new CostumeControlService(server);

  Serial.print("Creating "); Serial.print(NUM_MWNEXT_BLE_SERVICES); Serial.println(" MWNEXT services...");
  for (byte i = 0; i < NUM_MWNEXT_BLE_SERVICES; ++i) {
    MWNEXTServices[i] = new LightService(server, MWNEXTDevices[i]);
  }
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAppearance(0x0CC1); // powered wheelchair appearance

  // advertising: we wanna advertise a service with a fixed "name" (UUID), so that clients can look for this, decide to connect based on that,
  // and then enumerate the other services (one per logical "device" on the costume), which do not need to be advertised.
  // pAdvertising->addServiceUUID((uint16_t)ESP_GATT_UUID_DEVICE_INFO_SVC); // maybe don't advertise a service we disabled. No idea how that worked for a while!
  pAdvertising->addServiceUUID(MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID);
  
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue // NLV: what the heck is this? Taken from the ESP32 samples, should probably test what "iPhone issue" it's talking about...
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();
  Serial.println("BLE init complete.");
}

void loop() {
  checkButtons();

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    ReaderSession reader(rfid); // used for automatic cleanup, regardless of errors

    printTagDebug(rfid);
    if (!checkCompatibleTag(rfid))
      return;

    costumeController->setTagPresent(true);

    readLightSettingsFromTag(rfid, MWNEXTServices, NUM_MWNEXT_BLE_SERVICES);
  
  } else if (tryWakeExistingCard(rfid)) {
    ReaderSession reader(rfid); // used for automatic cleanup, regardless of errors
    if (costumeController->getTagWriteRequest() && rfid.PICC_ReadCardSerial()) { // do we need to attempt a read before doing the write we've been asked for?
        // TODO: any error handling at all here >.<
        // More precisely: probably try to catch write RFID write errors in writeLightSettingsToTag,
        // then set the Tag Write Error characteristic to non 0 if we errored out (AFTER setting Tag Write Request back to 0 in all cases, to signal completion, even if failure)
        // Then it's up to the client to clear the error from their end; we should probably be checking that there isn't an active error before allowing a write, too, but since we don't even set errors right now...
        writeLightSettingsToTag(rfid, MWNEXTServices, NUM_MWNEXT_BLE_SERVICES);
        pulseStatusLED();
        costumeController->clearWriteRequest();
    }
  } else { // no new card, and no existing card => card removed
    costumeController->setTagPresent(false);
  }
}
