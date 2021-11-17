# Magic Wheelchair Next - BLE tag programmer

A minimal ESP32 app to drive an MFRC522 RFID reader and read/write MIFARE Classic 1K tags with settings for Magic Wheelchair 3 - Freyja, via Bluetooth BLE.

This is intended to serve as a building block for MWNEXT.

A companion iOS app interfaces with the BLE Peripheral created by the ESP32 and reads/writes the protocol below.


## Protocol
The ESP32 acts as a BLE Device, exposing the below Services and Characteristics. All long form UUIDs are custom defined services / characteristics; all short identifier are Bluetooth SIG defined. See `config.h` for exact UUIDs.

The ESP32 advertises one / two services:
- Device Information Service (standard service `0x180A`), to expose the standard `Manufacturer Name` Characteristic (`0x2A29`). Note: currently commented out since there is a 6/8 service limit hard coded into ESP32 BLE libraries. To remove it, we'd need access to the build settings, which can't be done with ESP32 Arduino (but could by switching to ESP-IDF and using Arduino As A Component?)
- Costume Control Service (custom), providing the following characteristics:
  - Tag present (read only)
  - Tag write request (read, write, notify)
  - Tag write error (read, write, notify)

See _Tag writing process_ below for more.


The ESP32 also offers one custom service per device on the costume, allowing actual control. These are not advertised, Centrals (clients) are expected to look for the Costume Control Service to find the ESP32, then hardcode the UUIDs of the services to be controlled. In the future, it may be wise to have each device expose the same custom service ("Costume Device Service"?), since multiple instances of a given service UUID are possible in BLE?

Currently though, clients should query services by their known UUIDs, then, for each, query all characteristics and decide how to control each device based on what characteristics are present. Possible characteristics are:
  - Object Name (`0x2ABE`): `<a friendly name for the object>`. Read only.
  - Type: a single `uint8_t` representing the device type; slightly, but not completely, redundant with whether or not color controlling characteristics are present. To be cleaned up. Read only.
  - Mode: a single `uint8_t`; 0 to mean OFF, 1+ to be some sort of animation. Read, write, notify and indicate.
  - Hue: a single `uint8_t` color value; present if object supports colors. Read, write, notify and indicate.
  - Saturation: a single `uint8_t` light power value; present only if object supports colors. Read, write, notify and indicate.
  - CycleColor: a boolean (coded as a single `uint8_t` as far as BLE is concerned) valid indicating whether or not colors are cycling; present only if object supports colors.  Read, write, notify and indicate.

See `config.h` for values and enums etc. See `Ezekiel-DA/MWNext-BLE-remote-ios` for sample implementation of client app.


### Tag writing process
- When a tag is presented, it will initially be read (updating settings on the client)*
- The Tag Present characteristic will be set to 1 on the Costume Control Service; the Central / client should listen for this
- When this flag is set (and only then), the client is allowed to write the `Tag Write Request` characteristic to 1
- The peripheral will write current settings to the tag
- Once writing is complete, `Tag Write Request` will be reset to 0
- If any errors occurred during writing, `Tag Write Error` will be set to a non null value (see enum in `config.h`)
- The client should read `Tag Write Error` once  `Tag Write Request` has returned to 0
- Once the client has processed the error, it should write `Tag Write Error` back to 0 to clear the error before the next attempt at a write; if the error is not cleared, further writes may be refused


* do we need a different UX here? This means all settings will first be changed to what the tag contains, before being able to change them and write new settings. Batch writing tags would therefore be annoying... but is that a useful thing to do?

### About notifications
Right now, I can't get Indications (notify + wait for ACK) to work. It sounds like maybe CoreBluetooth doesn't actually let you register for indications? Or maybe we'd need to disable notification support on the Peripheral ("this") end? As per [this CoreBluetooth doc](https://developer.apple.com/documentation/corebluetooth/cbperipheral/1518949-setnotifyvalue): "If the specified characteristic’s configuration allows both notifications and indications, calling this method enables notifications only."

This probably doesn't matter anyway? I'm not sure Indications would help. if the Client doesn't ACK... then what?

For now, Notifications seem to work well enough, so I'm just going to leave this whole topic alone.


## Future work
What to reuse:
- ?

What to improve:
- rethink what "device type" even means. Are device types just a fixed list? are they a set of capabilities (RGB, programmable, PWM, etc?)
- where do modes (i.e. animation patterns) belong? Should we be able to enumerate different modes on different devices?
- is OFF a mode? is it separate info, so that we don't have to overwrite the mode to set OFF, allowing us to switch back to ON and retain the previous mode selection?