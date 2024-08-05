#include <SPI.h>
#include <MFRC522.h>

// RFID
#define RFID_PIN_SCK      13
#define RFID_PIN_MISO     12
#define RFID_PIN_MOSI     11
#define RFID_PIN_SS       10
#define RFID_PIN_RST      9

MFRC522 rfid(RFID_PIN_SS, RFID_PIN_RST);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  Serial.println(F("RFID Write Test"));

  // Wait until a card is present
  while (!rfid.PICC_IsNewCardPresent()) {
    delay(100);
  }

  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println(F("Failed to read card serial"));
    return;
  }

  byte blockAddr = 4; // Block address to write to (blocks 0-3 are for manufacturer data)
  String data = "Your data here!"; // Data to write, must be 16 bytes or less

  // Ensure the data is 16 bytes long (Pad if necessary)
  byte buffer[16];
  for (byte i = 0; i < 16; i++) {
    if (i < data.length()) {
      buffer[i] = data[i];
    } else {
      buffer[i] = 0x00; // Pad with 0x00
    }
  }

  if (writeRFIDBlock(blockAddr, buffer)) {
    Serial.println(F("Data written successfully"));
  } else {
    Serial.println(F("Failed to write data"));
  }

  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void loop() {
  // Nothing to do here
}

bool writeRFIDBlock(byte blockAddr, byte data[]) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;  // Default key

  // Authenticate using key A
  Serial.println(F("Authenticating..."));
  if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
    Serial.println(F("Authentication failed"));
    Serial.println(rfid.GetStatusCodeName(rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid))));
    return false;
  }

  // Write data to the block
  Serial.println(F("Writing data..."));
  if (rfid.MIFARE_Write(blockAddr, data, 16) != MFRC522::STATUS_OK) {
    Serial.println(F("Writing failed"));
    Serial.println(rfid.GetStatusCodeName(rfid.MIFARE_Write(blockAddr, data, 16)));
    return false;
  }

  Serial.println(F("Write successful"));
  return true;
}

