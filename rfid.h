#pragma once

#include <MFRC522.h>

#include "lightService.h"

MFRC522::MIFARE_Key mifareDefaultKey = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void printHex(byte *buffer, byte bufferSize) {
  Serial.print("{ ");
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print("0x");
    Serial.print(buffer[i] < 0x10 ? "0" : "");
    Serial.print(buffer[i], HEX);
    Serial.print((i == bufferSize-1) ? " }" : ", ");
  }
}

/**
 * Print some debugging info for a tag.
 * Note: the input reader object must have already called PICC_ReadCardSerial()
 */
void printTagDebug(MFRC522& reader)
{
  MFRC522::PICC_Type piccType = reader.PICC_GetType(reader.uid.sak);
  
  Serial.print("Tag ID: "); printHex(reader.uid.uidByte, reader.uid.size); Serial.print(" - "); Serial.print(F("PICC type: ")); Serial.println(reader.PICC_GetTypeName(piccType));
}

bool checkCompatibleTag(MFRC522& reader)
{
  MFRC522::PICC_Type type = reader.PICC_GetType(reader.uid.sak);

  if (type != MFRC522::PICC_TYPE_MIFARE_MINI && type != MFRC522::PICC_TYPE_MIFARE_1K && type != MFRC522::PICC_TYPE_MIFARE_4K)
  {
    Serial.println(F("Unsupported PICC type; please use MIFARE Classic tags."));
    return false;
  }

  return true;
}

MFRC522::StatusCode readBlock(MFRC522& reader, byte iBlockAddr, byte* oData, byte* ioSize) {
  MFRC522::StatusCode status;
  
  byte trailerAddr = ((iBlockAddr / 4)*4)+3; // block address of the sector trailer for the block we're trying to read
  
  status = (MFRC522::StatusCode) reader.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerAddr, &mifareDefaultKey, &(reader.uid));
  if (status != MFRC522::STATUS_OK) {
      return status;
  }
  
  // Serial.print("Reading block #"); Serial.print(iBlockAddr); Serial.print(" - trailer address for read is: "); Serial.println(trailerAddr);
  return (MFRC522::StatusCode) reader.MIFARE_Read(iBlockAddr, oData, ioSize);
}

MFRC522::StatusCode writeBlock(MFRC522& reader, byte iBlockAddr, byte* iData)
{
  MFRC522::StatusCode status;
  
  byte trailerAddr = ((iBlockAddr / 4)*4)+3; // block address of the sector trailer for the block we're trying to read
  status = (MFRC522::StatusCode) reader.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerAddr, &mifareDefaultKey, &(reader.uid));
  if (status != MFRC522::STATUS_OK) {
      return status;
  }

  // Serial.print("Writing block #"); Serial.println(iBlockAddr); Serial.print(" - railer address for write is: "); Serial.println(trailerAddr);
  return reader.MIFARE_Write(iBlockAddr, iData, 16);
};

void dump_byte_array(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

class ReaderSession
{
  MFRC522& _reader;

public:
  ReaderSession(MFRC522& reader) : _reader(reader) {};
  ~ReaderSession() {
    _reader.PICC_HaltA();
    _reader.PCD_StopCrypto1();
  };
};

bool tryWakeExistingCard(MFRC522& reader) {
  byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);

  // Should we do this for a wakeup?
	// Reset baud rates
	reader.PCD_WriteRegister(reader.TxModeReg, 0x00);
	reader.PCD_WriteRegister(reader.RxModeReg, 0x00);
	// Reset ModWidthReg
	reader.PCD_WriteRegister(reader.ModWidthReg, 0x26);

  MFRC522::StatusCode res = reader.PICC_WakeupA(bufferATQA, &bufferSize);
  return (res == MFRC522::STATUS_OK || res == MFRC522::STATUS_COLLISION);
};

void writeLightSettingsToTag(MFRC522& reader, LightService* lightServices[], const uint8_t& iNumLightServices)
{
  // reserve space for 5 lights
  // - each light is encoded over 3 bytes, so up to 5 lights per block on a MiFare Classic PICC
  byte lightsDataBuffer[16];
  memset(lightsDataBuffer, 0, sizeof(lightsDataBuffer));
  lightsDataBuffer[15] = 0xFF;

  for (byte idx = 0; idx < iNumLightServices; ++idx)
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

void readLightSettingsFromTag(MFRC522& reader, LightService* lightServices[], const uint8_t& iNumLightServices)
{
  byte buffer[18]; // minimum of 16 (size of a block) + 2 (CRC)
  byte size = sizeof(buffer);
  
  MFRC522::StatusCode ret = readBlock(reader, MW_RFID_DATA_BLOCK_ADDR, buffer, &size);
  if (ret != MFRC522::STATUS_OK)
  {
    Serial.print(F("Internal failure in RFID reader: "));
    Serial.print(reader.GetStatusCodeName(ret)); Serial.print(" while reading block #"); Serial.println(MW_RFID_DATA_BLOCK_ADDR);
    return;
  }

  Serial.print("Data in block #"); Serial.print(MW_RFID_DATA_BLOCK_ADDR); Serial.print(": ");
  dump_byte_array(buffer, 16); Serial.println();

  for (byte i = 0; i < iNumLightServices; ++i)
  {
    memcpy(&(lightServices[i]->_lightData), &(((LightDataBlock*)buffer)[i]), sizeof(LightDataBlock));
    lightServices[i]->debugDump();
    lightServices[i]->forceBLEUpdate();
  }
}