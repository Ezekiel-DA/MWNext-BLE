#pragma once

#define MW_RFID_DATA_BLOCK_ADDR 4

// the advertised service (which does nothing except allow clients to identify us)
#define MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID     "47191881-ebb3-4a9f-9645-3a5c6dae4900"

// non advertised, light controlling services
#define MWNEXT_BLE_WINDOWS_SERVICE_UUID             "c3f48cdf-18bb-4947-bef2-f804d59d74da"
#define MWNEXT_BLE_CLOUDS_SERVICE_UUID              "f248fd08-0326-482e-b023-db6e3f6bb250"
#define MWNEXT_BLE_WALLS_SERVICE_UUID               "b3701981-4955-49d5-bd2d-e2ea57e8e64c"
#define MWNEXT_BLE_MOAT_SERVICE_UUID                "5180f077-a430-4a6d-b6ea-cdf1075a0dd9"
#define MWNEXT_BLE_STARS_SERVICE_UUID               "c634d6bf-0f7b-4580-b342-7aa2d42dedab"

// characteristics for our light controlling services
#define MWNEXT_BLE_DEVICE_TYPE_UUID                 "8106f98f-fb24-4b97-a995-47a1695cea75"
#define MWNEXT_BLE_MODE_CHARACTERISTIC_UUID         "b54fc13b-4374-4a6f-861f-dd198f88f299"
#define MWNEXT_BLE_HUE_CHARACTERISTIC_UUID          "19dfe175-aa12-404b-843d-b625937cffff"
#define MWNEXT_BLE_CYCLE_COLOR_CHARACTERISTIC_UUID  "dfe34849-2d42-4222-b6b1-617a4f4d0869"
#define MWNEXT_BLE_SATURATION_CHARACTERISTIC_UUID   "946d22e6-2b2f-49e7-b941-150b023f2261"

// TODO: rethink this whole thing. This probably needs to be some sort of bitfield of capabilities?
enum struct MWNEXT_DEVICE_TYPE {
  RGB_LED = 1,
  MONO_LED,
  ONOFF_PORT
};
