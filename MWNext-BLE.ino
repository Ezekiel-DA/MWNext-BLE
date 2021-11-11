#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <SPI.h>
#include <MFRC522.h>

#include <AceButton.h>
using namespace ace_button;

#include "config.h"
#include "rfid.h"
#include "buttons.h"
#include "lightService.h"

bool deviceConnected = false;

MWNEXTDeviceInfo MWNEXTDevices[] = {
  {.type=MWNEXT_DEVICE_TYPE::RGB_LED,     .uuid=(BLEUUID)MWNEXT_BLE_WINDOWS_SERVICE_UUID,  .name="Windows"},
  {.type=MWNEXT_DEVICE_TYPE::RGB_LED,     .uuid=(BLEUUID)MWNEXT_BLE_CLOUDS_SERVICE_UUID,   .name="Clouds"},
  {.type=MWNEXT_DEVICE_TYPE::MONO_LED,    .uuid=(BLEUUID)MWNEXT_BLE_WALLS_SERVICE_UUID,    .name="Walls"},
  {.type=MWNEXT_DEVICE_TYPE::RGB_LED,     .uuid=(BLEUUID)MWNEXT_BLE_MOAT_SERVICE_UUID,     .name="Moat"},
  {.type=MWNEXT_DEVICE_TYPE::ONOFF_PORT,  .uuid=(BLEUUID)MWNEXT_BLE_STARS_SERVICE_UUID,    .name="Stars"}
};
const uint8_t NUM_MWNEXT_BLE_SERVICES = sizeof(MWNEXTDevices) / sizeof(MWNEXTDeviceInfo);

void writeLightSettingsToTag(MFRC522& reader, LightService* lightServices[])
{
  // reserve space for 5 lights
  // - each light is encoded over 3 bytes, so up to 5 lights per block on a MiFare Classic PICC
  byte lightsDataBuffer[16];
  memset(lightsDataBuffer, 0, sizeof(lightsDataBuffer));
  lightsDataBuffer[15] = 0xFF;

  uint8_t dummyData[] = {0xFF, 0xFE, 0xFD};

  for (byte idx = 0; idx < NUM_MWNEXT_BLE_SERVICES; ++idx)
  {
    memcpy(lightsDataBuffer + (idx*3), &(lightServices[idx]->_lightData), 3);
  }
  
  Serial.println("Writing data to tag...");
  dump_byte_array(lightsDataBuffer, 16); Serial.println();  
  
  MFRC522::StatusCode ret = writeBlock(reader, MW_RFID_DATA_BLOCK_ADDR, lightsDataBuffer);
  if (ret != MFRC522::STATUS_OK)
  {
    Serial.print(F("Internal failure while writing to tag: "));
    Serial.println(reader.GetStatusCodeName(ret));
    return;
  }

  Serial.println("Wrote lights data to tag.");
};

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    deviceConnected = true;
    Serial.println("Central connected. Start sending updates.");
  };

  void onDisconnect(BLEServer* server) {
    deviceConnected = false;
    Serial.println("Central disconnected; Advertising again...");
    BLEDevice::startAdvertising();
  };
};

MFRC522 rfid(5, UINT8_MAX); // RST pin (NRSTPD on MFRC522) not connected; setting it to this will let the library switch to using soft reset only
static LightService* MWNEXTServices [NUM_MWNEXT_BLE_SERVICES];

void setup() {
  Serial.begin(115200);
  
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

  BLEService* costumeControlService = server->createService(MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID);
  costumeControlService->start();

  Serial.print("Creating "); Serial.print(NUM_MWNEXT_BLE_SERVICES); Serial.println(" MWNEXT services...");
  for (byte i = 0; i < NUM_MWNEXT_BLE_SERVICES; ++i) {
    MWNEXTServices[i] = new LightService(server, MWNEXTDevices[i]);
  }
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAppearance(0x0CC1); // powered wheelchair appearance

  // advertising: we wanna advertise a service with a fixed "name" (UUID), so that clients can look for this, decide to connect based on that,
  // and then enumerate the other services (one per logical "device" on the costume), which do not need to be advertised.
  pAdvertising->addServiceUUID((uint16_t)ESP_GATT_UUID_DEVICE_INFO_SVC);
  pAdvertising->addServiceUUID(MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID);
  
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();
  Serial.println("BLE init complete.");
}

void readLightSettingsFromTag()
{
  byte buffer[18]; // minimum of 16 (size of a block) + 2 (CRC)
  byte size = sizeof(buffer);
  
  MFRC522::StatusCode ret = readBlock(rfid, MW_RFID_DATA_BLOCK_ADDR, buffer, &size);
  if (ret != MFRC522::STATUS_OK)
  {
    Serial.print(F("Internal failure in RFID reader: "));
    Serial.print(rfid.GetStatusCodeName(ret)); Serial.print(" while reading block #"); Serial.println(MW_RFID_DATA_BLOCK_ADDR);
    return;
  }

  Serial.print("Data in block #"); Serial.print(MW_RFID_DATA_BLOCK_ADDR); Serial.print(": ");
  dump_byte_array(buffer, 16); Serial.println();

  // TODO apply tag settings to services and characteristics so we can "read" a tag to BLE
}

void loop() {
  checkButtons();

  //MWNEXTServices[0]->setHue((MWNEXTServices[0]->_lightData.hue) + 2);
  
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    ReaderSession reader(rfid); // used for automatic cleanup, regardless of errors

    printTagDebug(rfid);
    if (!checkCompatibleTag(rfid))
      return;

    if (rfidWrite)
    {
      writeLightSettingsToTag(rfid, MWNEXTServices);
    } else // read tag and apply settings
    {
      readLightSettingsFromTag();
    }
  }

  //MWNEXTServices[1]->setSaturation((MWNEXTServices[0]->_lightData.saturation) - 2);
 
  delay(2000);
}