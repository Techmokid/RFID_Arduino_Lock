#include <SPI.h>
#include <Wire.h> 
#include <MFRC522.h>
#include <LiquidCrystal.h>

// LCD
#define LCD_PIN_D4        2
#define LCD_PIN_D5        3
#define LCD_PIN_D6        4
#define LCD_PIN_D7        5
#define LCD_PIN_RS        6
#define LCD_PIN_E         7

// RFID
#define RFID_PIN_SCK      13
#define RFID_PIN_MISO     12
#define RFID_PIN_MOSI     11
#define RFID_PIN_SS       10
#define RFID_PIN_RST      9

// Other
#define LED_G_PIN         A0
#define LED_R_PIN         A1
#define RELAY_PIN         A2

// Values
#define UNLOCK_TIME       6    // Seconds
#define SECRET_KEY        "MYSECRETKEY41139"

LiquidCrystal lcd(LCD_PIN_RS, LCD_PIN_E, LCD_PIN_D4, LCD_PIN_D5, LCD_PIN_D6, LCD_PIN_D7);
MFRC522 rfid(RFID_PIN_SS, RFID_PIN_RST);

void setup() {
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
    
  lcd.begin(16,2);
  //lcd.backlight();

  digitalWrite(LED_G_PIN,HIGH);
  digitalWrite(LED_R_PIN,HIGH);
  printLCD(0,0,"   RFID  LOCK   ");
  printLCD(0,1,"   RFID  LOCK   ");
  delay(2000);
  digitalWrite(LED_G_PIN,LOW);
  digitalWrite(LED_R_PIN,LOW);

  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  lcd.clear();

  Serial.println(F("Checking MFRC522..."));
  rfid.PCD_DumpVersionToSerial();
  Serial.println(rfid.PCD_PerformSelfTest());
  while (!rfid.PCD_PerformSelfTest()) {
    Serial.println(F("*****************************"));
    Serial.println(F("MFRC522 Digital self test:"));
    Serial.println(GetRFIDVersion());  // Show version of PCD - MFRC522 Card Reader
    Serial.println();
    
    printLCD(0,0,"   RFID   ERR   ");
    printLCD(0,1," SER DIAG START ");
    
    for (int i = 0; i < 4; i++) {
      digitalWrite(LED_R_PIN, HIGH);
      delay(150);
      digitalWrite(LED_R_PIN, LOW);
      delay(150);
    }
  }
}

unsigned long lastSerialUpdate = 0;
bool hasErrorPrinted = false;
bool hasPrinted = false;
void loop()  {
  //if (millis() - lastSerialUpdate > 1000) { Serial.println("KEEP ALIVE"); lastSerialUpdate = millis(); }

  if (!rfid.PCD_PerformSelfTest()) {
    if (!hasErrorPrinted) {
      lcd.clear();
      printLCD(0,0,"   RFID   ERR   ");
      printLCD(0,1,"   COMMS LOST   ");
    }
    
    while (!rfid.PCD_PerformSelfTest()) { delay(50); }
    hasPrinted = false;
    hasErrorPrinted = true;
    return;
  }

  if (!hasPrinted) {
    hasPrinted = true;
    hasErrorPrinted = false;
    lcd.clear();
    printLCD(0,0,"   PLACE YOUR   ");
    printLCD(0,1,"    RFID TAG    ");
  }
  
  
  if (!rfid.PICC_IsNewCardPresent()) { return; }
  if (!rfid.PICC_ReadCardSerial())   { return; }
  hasPrinted = false;
  hasErrorPrinted = false;

  String content = readBlockData(1);  // Read block 1 (you may need to adjust based on your tag's layout)
  content.toUpperCase();
  Serial.println("Contents: " + content);
  
  if (content == SECRET_KEY) {
    // Correct card used
    lcd.clear();
    printLCD(0,0,"    UNLOCKED    ");
    delay(2000);
    
    lcd.clear();
    digitalWrite(LED_G_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
    delay(UNLOCK_TIME * 1000);

    printLCD(0,0,"     LOCKED     ");
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_G_PIN, LOW);
    delay(1000);
  } else {
    // Wrong card used
    printLCD(0,0," INCORRECT AUTH ");
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_R_PIN, HIGH);
      delay(200);
      digitalWrite(LED_R_PIN, LOW);
      delay(200);
    }
    
    lcd.clear();
    delay(2000);
  }

  rfid.PICC_HaltA();
}

void printLCD(int x, int y, String msg) {
  lcd.setCursor(x, y);
  lcd.print(msg);
}

String GetRFIDVersion() {
  return String(rfid.PCD_ReadRegister(rfid.CommandReg), HEX);
}

String readBlockData(byte block) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  MFRC522::StatusCode status;

  // Authenticate with the card
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return "";
  }

  byte buffer[18];
  byte len = 18;

  // Read data from the block
  status = rfid.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return "";
  }

  // Convert the data to a string, ignoring spaces (0x20)
  String data = "";
  for (uint8_t i = 0; i < 16; i++) {
    if (buffer[i] != 32) {
      data += (char)buffer[i];
    }
  }

  return data;
}
