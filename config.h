#pragma once

#define MW_RFID_DATA_BLOCK_ADDR 4

#define RFID_READER_CS_PIN 15
#define STATUS_LED_PIN 5

// the advertised service, allowing clients to find us
#define MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID           "47191881-ebb3-4a9f-9645-3a5c6dae4900"

// characteristics for the main service, giving access to Peripheral-wide functions
#define MWNEXT_BLE_TAG_PRESENT_CHARACTERISTIC_UUID        "68344255-0632-4635-93f0-178bd0d7e577"
#define MWNEXT_BLE_TAG_WRITE_REQUEST_CHARACTERISTIC_UUID  "64957168-79ba-469b-96e9-ccf53086c4c3"
#define MWNEXT_BLE_TAG_WRITE_ERROR_CHARACTERISTIC_UUID    "acd33ab1-eda9-4790-8ce2-f9fe951b3e25"

// service for controlling light devices; multiple instances allowed
#define MWNEXT_BLE_LIGHT_DEVICE_SERVICE_UUID              "0ba35e90-f55f-4f15-9347-3dc4a0287881"

// characteristics for our light controlling services 
//#define MWNEXT_BLE_DEVICE_TYPE_CHARACTERISTIC_UUID                  "8106f98f-fb24-4b97-a995-47a1695cea75"
#define MWNEXT_BLE_CAPABILITIES_CHARACTERISTIC_UUID                 "ed8128c1-5bfa-418c-9a4d-af2596f92952"
#define MWNEXT_BLE_ID_CHARACTERISTIC_UUID                           "63c62656-807f-4db4-97d3-94095962acf8"
#define MWNEXT_BLE_STATE_CHARACTERISTIC_UUID                        "c2af353b-e5fc-4bdf-b743-5d226f1198a2"
#define MWNEXT_BLE_MODE_CHARACTERISTIC_UUID                         "b54fc13b-4374-4a6f-861f-dd198f88f299"
#define MWNEXT_BLE_CYCLE_COLOR_CHARACTERISTIC_UUID                  "dfe34849-2d42-4222-b6b1-617a4f4d0869"
#define MWNEXT_BLE_HUE_CHARACTERISTIC_UUID                          "19dfe175-aa12-404b-843d-b625937cffff"
#define MWNEXT_BLE_SATURATION_CHARACTERISTIC_UUID                   "946d22e6-2b2f-49e7-b941-150b023f2261"
#define MWNEXT_BLE_VALUE_CHARACTERISTIC_UUID                        "6c5df188-2e69-4f2f-b4ab-9d2b76ef7aa9"
#define MWNEXT_BLE_ADDRESSABLE_SHAPE_HORIZONTAL_CHARACTERISTIC_UUID "19a27774-334c-4655-a9f5-a5c1c31509f7"
#define MWNEXT_BLE_ADDRESSABLE_SHAPE_VERTICAL_CHARACTERISTIC_UUID   "c5b739cd-fcb9-4eb5-8e6f-65ed08a4acb4"
#define MWNEXT_BLE_ADDRESSABLE_DATA_CHARACTERISTIC_UUID             "8d4510e9-8565-49a4-8813-f45f49061833"

enum struct MWNEXT_WRITE_REQUEST_ERROR {
  MFRC522_ERROR = 1,
  NO_TAG,
};
