/**
 * ----------------------------------------------------------------------------
 * This is a MFRC522 library example; see https://github.com/miguelbalboa/rfid
 * for further details and other examples.
 * 
 * NOTE: The library file MFRC522.h has a lot of useful info. Please read it.
 * 
 * Released into the public domain.
 * ----------------------------------------------------------------------------
 * This sample shows how to read and write data blocks on a MIFARE Classic PICC
 * (= card/tag).
 * 
 * BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).
 * 
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno           Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 * 
 */

#include <SPI.h> 
#include <MFRC522.h> 
#include <Wire.h> 
#include <DS3231.h> 
#include <Eeprom24C32_64.h> 
#include <EEPROM.h>

#define START_SECTOR  2
//#define PC_DEBUG 1

#define RST_PIN       9 // Configurable, see typical pin layout above
#define SS_PIN       10 // Configurable, see typical pin layout above

#define LED_RED       6
#define LED_GREEN     7
#define LED_BLUE      8

static Eeprom24C32_64 AT24C32(0x57);

DS3231 DS3231_clock;
RTCDateTime dt;

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;


byte POST_NUMBER = 0;
byte POST_FINAL = 10;

const byte numChars = 32;
char receivedChars[numChars];

boolean newData = false;
int command = 0;
boolean showTime = true;
String inString = "";
uint16_t t_year;
uint8_t t_month, t_day, t_hour, t_minute, t_second;

uint16_t logCount = 0;

void setup() {

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(250000); // Initialize serial communications with the PC
  while (!Serial); // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  Serial.println("Initialize DS3231");;
  DS3231_clock.begin();

  Serial.println("Initialize SPI");;
  SPI.begin(); // Init SPI bus

  Serial.println("Initialize MFRC522");;
  mfrc522.PCD_Init(); // Init MFRC522 card

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  POST_NUMBER = EEPROM.read(0);
  POST_FINAL = EEPROM.read(1);

  Serial.print(F("==================== POST: "));
  Serial.print(POST_NUMBER);
  Serial.println(F(" ===================="));

//  byte erase[32] = {
//    0,0,0,0,0,0,0,0,
//    0,0,0,0,0,0,0,0,
//    0,0,0,0,0,0,0,0,
//    0,0,0,0,0,0,0,0
//  };
//  for (int i = 0; i < 128; i++) {
//    AT24C32.writeBytes(i,erase,32);
//  }
//  Serial.println(F(" ===================="));

}

void loop() {

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);

  if (Serial.available()) { // Look for char in serial que and process if found
    command = Serial.read();
    if (command == 82) { //If command = "R" Read Date ... BBR
      dt = DS3231_clock.getDateTime();
      Serial.read();
      Serial.println(DS3231_clock.dateFormat("Y-m-d H:i:s", dt));
    } else if (command == 114) { //If command = "r" Read Date ... BBR
      dt = DS3231_clock.getDateTime();
      Serial.read();
      Serial.print("$POST:");
      Serial.print(POST_NUMBER);
      Serial.print(" Time: ");
      Serial.println(DS3231_clock.dateFormat("Y-m-d H:i:s", dt));
    } else if (command == 84 || command == 116) { //If command = "Tt" Set Date
      while (Serial.available() > 0) {
        int inChar = Serial.read();
        if (isDigit(inChar)) {
          inString += (char) inChar;
        }
        if (inChar == '\n') {
          Serial.print("Setting time to: ");
          Serial.println(inString.toInt());
          DS3231_clock.setDateTime(inString.toInt());
          inString = "";
        }
      }
    } else if (command == 70) { //If command = "Fx,y" set value y to address x in EEPROM else show EEPROM values;
      byte address, value;
      while (Serial.available() > 0) {
        int inChar = Serial.read();
        inString += (char) inChar;
        if (inChar == '\n') {
          Serial.print("F: ");
          Serial.println(inString.toInt());
          if (sscanf(inString.c_str(), "%d,%d", & address, & value) == 2) {
            EEPROM.write(address, value);
          } else {
            for (int i = 0; i < 10; i++) { //EEPROM.length()
              value = EEPROM.read(i);
              Serial.print(i);
              Serial.print(":");
              Serial.print(value, DEC);
              Serial.println();
            }
          }
          inString = "";
        }
      }
    } else if (command == 67) { //If command = "C" Dump epprom
      Serial.read();
      for (int i = 0; i < 4001; i++) {
        AT24C32.writeEE(i, i);
      }
    } else if (command == 69) { //If command = "E" Dump epprom
      Serial.read();
      byte tmp;
      for (int i = 0; i < 30; i++) {
        AT24C32.readEE(i, tmp);
        Serial.print(i);
        Serial.print(":");
        Serial.println(tmp, HEX);
      }
    } else if (command == 101) { //If command = "e" Dump epprom
      Serial.read();
      uint32_t uid;
      uint32_t t2;
      for (int i = 0; i < 250; i++) {
        AT24C32.readEE(i * 8, t2);
        RTCDateTime timp2 = fromUnixTime(t2);
        AT24C32.readEE(i * 8 + 4, uid);
        Serial.print(i); Serial.print(","); Serial.print(DS3231_clock.dateFormat("d-m-Y H:i:s", timp2)); Serial.print(","); Serial.println(uid, HEX);
      }
    } else if (command == 68 || command == 100) { //If command = "Dd" Set Date
      t_year = (uint16_t)((Serial.read() - 48) * 1000 + (Serial.read() - 48) * 100 + (Serial.read() - 48) * 10 + (Serial.read() - 48));
      t_month = (uint8_t)((Serial.read() - 48) * 10 + (Serial.read() - 48));
      t_day = (uint8_t)((Serial.read() - 48) * 10 + (Serial.read() - 48));
      t_hour = (uint8_t)((Serial.read() - 48) * 10 + (Serial.read() - 48));
      t_minute = (uint8_t)((Serial.read() - 48) * 10 + (Serial.read() - 48));
      t_second = (uint8_t)((Serial.read() - 48) * 10 + (Serial.read() - 48)); // Use of (byte) type casting and ascii math to achieve result.  
      //Serial.println(inString);
      Serial.print("Setting time to: ");
      Serial.print(t_year);
      Serial.print("-");
      Serial.print(t_month);
      Serial.print("-");
      Serial.print(t_day);
      Serial.print(" ");
      Serial.print(t_hour);
      Serial.print(":");
      Serial.print(t_minute);
      Serial.print(":");
      Serial.print(t_second);
      Serial.println("");

      DS3231_clock.setDateTime(t_year, t_month, t_day, t_hour, t_minute, t_second);
      Serial.read();
    }
    //Serial.print("Command: ");
    //Serial.println(command);     // Echo command CHAR in ascii that was sent

    command = 0; // reset command 
    delay(100);
  }

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
    return;
  unsigned long StartTime = millis();
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
    return;

  digitalWrite(LED_BLUE, HIGH);

#ifdef PC_DEBUG  
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = CurrentTime - StartTime;
  Serial.println(ElapsedTime);
#endif

  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("This sample only works with MIFARE Classic cards."));
    return;
  }

  byte sector = POST_NUMBER + START_SECTOR;
  byte blockAddr = sector * 4;
  byte dataBlock[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
  };

  MFRC522::StatusCode status;

#ifdef PC_DEBUG  
  CurrentTime = millis();
  ElapsedTime = CurrentTime - StartTime;
  Serial.println(ElapsedTime);
#endif

  if (POST_NUMBER == 0) // IF START, CLEAR CARD
  {
    Serial.println(F("Clearing data on card !!!!"));
    for (int i = START_SECTOR; i < START_SECTOR + POST_FINAL; i++) {
      status = clear_sector(i, dataBlock);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_read() --- failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        blinkRed(5);
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
      }
    }
  }

#ifdef PC_DEBUG  
  CurrentTime = millis();
  ElapsedTime = CurrentTime - StartTime;
  Serial.println(ElapsedTime);
#endif

  dt = DS3231_clock.getDateTime();
  uint32_t timp = dt.unixtime;

  Serial.println(timp);

  byte trailerBlock = sector * 4 + 3;
  byte bufer[18];
  byte size = sizeof(bufer);

  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, & key, & (mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkRed(5);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, bufer, & size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkRed(5);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

#ifdef PC_DEBUG  
  Serial.print(F("Data in block "));
  Serial.print(blockAddr);
  Serial.println(F(":"));
  dump_byte_array(bufer, 16);
  Serial.println();
  Serial.println(timp);
#endif

  memcpy(dataBlock + 4, bufer, 12);
#ifdef PC_DEBUG  
  dump_byte_array(bufer, 16);
  Serial.println();
  Serial.println(timp);
#endif

  memcpy(dataBlock, & timp, sizeof(timp));
#ifdef PC_DEBUG  
  dump_byte_array(dataBlock, 16);
  Serial.println();
#endif

#ifdef PC_DEBUG  
  Serial.print(F("Writing data into block "));
  Serial.print(blockAddr);
  Serial.println(F(" ..."));
  dump_byte_array(dataBlock, 16);
  Serial.println();
#endif

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkRed(5);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  // Read data from the block (again, should now be what we have written)
#ifdef PC_DEBUG  
  Serial.print(F("Reading data from block "));
  Serial.print(blockAddr);
  Serial.println(F(" ..."));
#endif
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, bufer, & size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkRed(5);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  
#ifdef PC_DEBUG  
  Serial.print(F("Data in block "));
  Serial.print(blockAddr);
  Serial.println(F(":"));
  dump_byte_array(bufer, 16);
  Serial.println();
#endif

  // Check that data in block is what we have written
  // by counting the number of bytes that are equal
#ifdef PC_DEBUG  
  Serial.println(F("Checking result..."));
#endif
  byte count = 0;
  for (byte i = 0; i < 16; i++) {
    // Compare bufer (= what we've read) with dataBlock (= what we've written)
    if (bufer[i] == dataBlock[i])
      count++;
  }
  
#ifdef PC_DEBUG  
  Serial.print(F("Number of bytes that match = "));
  Serial.println(count);
#endif

  if (count == 16) {
#ifdef PC_DEBUG  
    Serial.println(F("Success :-)"));
    CurrentTime = millis();
    ElapsedTime = CurrentTime - StartTime;
    Serial.println(ElapsedTime);
#endif
  } else {
#ifdef PC_DEBUG    
    Serial.println(F("Failure, no match :-("));
    Serial.println(F("  perhaps the write didn't work properly..."));
#endif    
    blinkRed(5);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  Serial.println();

#ifdef PC_DEBUG  
  // Dump the sector data
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial( & (mfrc522.uid), & key, sector);
  Serial.println();
#endif

//  if (POST_NUMBER == POST_FINAL) // READ & Calculate time
//  {
//    blockAddr = START_SECTOR * 4;
//    trailerBlock = START_SECTOR * 4 + 3;
//    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, & key, & (mfrc522.uid));
//    if (status != MFRC522::STATUS_OK) {
//      Serial.print(F("PCD_Authenticate() failed: "));
//      Serial.println(mfrc522.GetStatusCodeName(status));
//      blinkRed(5);
//      mfrc522.PICC_HaltA();
//      mfrc522.PCD_StopCrypto1();
//
//      return;
//    }
//    Serial.print(F("Reading data from block "));
//    Serial.print(blockAddr);
//    Serial.println(F(" ..."));
//    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, bufer, & size);
//    if (status != MFRC522::STATUS_OK) {
//      Serial.print(F("MIFARE_Read() failed: "));
//      Serial.println(mfrc522.GetStatusCodeName(status));
//      blinkRed(5);
//      mfrc522.PICC_HaltA();
//      mfrc522.PCD_StopCrypto1();
//      return;
//    }
//    Serial.print(F("Data in block "));
//    Serial.print(blockAddr);
//    Serial.println(F(":"));
//    dump_byte_array(bufer, 16);
//    Serial.println();
//    uint32_t timp_start;
//    memcpy( & timp_start, bufer, 4);
//    //Serial.println(now());
//    Serial.println(F("========================================================="));
//    Serial.print("TIMP TOTAL: ");
//    Serial.println(TimeToString(timp - timp_start));
//    Serial.println(F("========================================================="));
//
//  }

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);

  AT24C32.writeEE(logCount * 8, timp);
  AT24C32.writeEE_mem(logCount * 8 + 4, mfrc522.uid.uidByte, 4);
  logCount++;

  blinkGreen(2);

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte * bufer, byte buferSize) {
  for (byte i = 0; i < buferSize; i++) {
    Serial.print(bufer[i] < 0x10 ? " 0" : " ");
    Serial.print(bufer[i], HEX);
  }
}

MFRC522::StatusCode clear_sector(byte sector, byte dataBlock[]) {

  //Serial.print("Clearing sector: ");Serial.println(sector);
  //byte sector         = 2;
  byte blockAddr = sector * 4;
  byte trailerBlock = sector * 4 + 3;
  
  MFRC522::StatusCode status;

  //Serial.println(F("Authenticating using key A..."));
  //dump_byte_array(key.keyByte,6);
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, & key, & (mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return status;
  }

  // Write data to the block
  //Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
  //Serial.println(F(" ..."));
  //dump_byte_array(dataBlock, 16); Serial.println();
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr + 1, dataBlock, 16);
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr + 2, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return status;
  }
  //Serial.println();
  //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);Serial.println();

  return MFRC522::STATUS_OK;

}

char * TimeToString(unsigned long t) {
  static char str[12];
  long h = t / 3600;
  t = t % 3600;
  int m = t / 60;
  int s = t % 60;
  sprintf(str, "%02ld:%02d:%02d", h, m, s);
  return str;
}

void blinkRed(byte n) {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  for (byte i = 0; i < n; i++) {
    digitalWrite(LED_RED, HIGH);
    delay(200);
    digitalWrite(LED_RED, LOW);
    delay(100);
  }
}

void blinkGreen(byte n) {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  for (byte i = 0; i < n; i++) {
    //Serial.println(i);
    digitalWrite(LED_GREEN, HIGH);
    delay(200);
    digitalWrite(LED_GREEN, LOW);
    delay(100);
  }
}
