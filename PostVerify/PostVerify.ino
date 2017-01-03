

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

/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  


//#define POST_CONTROL    7           // 0=START , 1=P1, 2=P2, ....

#define START_SECTOR    2
//#define PC_DEBUG 1

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

#define LED_RED       6
#define LED_GREEN       7
#define LED_BLUE      8

static Eeprom24C32_64 AT24C32(0x57);

DS3231 DS3231_clock;
RTCDateTime dt;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
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
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

    Serial.println("Initialize DS3231");;
    DS3231_clock.begin();

    Serial.println("Initialize SPI");;
    SPI.begin();        // Init SPI bus
    
    Serial.println("Initialize MFRC522");;
    mfrc522.PCD_Init(); // Init MFRC522 card

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

  POST_NUMBER = EEPROM.read(0);
    POST_FINAL = EEPROM.read(1);

    Serial.print(F("==================== POST: ")); Serial.print(F("VERIFICARE"));
    Serial.print("Post Final:"); Serial.print(POST_FINAL); 
    Serial.println(F(" ===================="));
 
}

void loop() {

   digitalWrite(LED_GREEN, LOW);  
   digitalWrite(LED_RED, LOW);  
   digitalWrite(LED_BLUE, LOW);  

     if (Serial.available()) {      // Look for char in serial que and process if found
        command = Serial.read();
      if (command == 70) { //If command = "Fx,y" set value y to address x in EEPROM else show EEPROM values;
        byte address, value;
        while (Serial.available() > 0) {
          int inChar = Serial.read();
          //if (isDigit(inChar)) {
            inString += (char)inChar;
          //}
          // if you get a newline, print the string,
          // then the string's value:
          if (inChar == '\n') {
            Serial.print("F: ");
            
              if (sscanf(inString.c_str(), "%d,%d", &address, &value) == 2) {
                                EEPROM.write(address,value);
                                Serial.print(address);
                                Serial.print(":");
                                Serial.println(value);
              } else {
                for (int i = 0; i < 10; i++) {//EEPROM.length()
                  
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

     }
     }
    
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;
    unsigned long StartTime = millis();
    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;
    digitalWrite(LED_BLUE, HIGH);  

#ifdef PC_DEBUG  
    unsigned long CurrentTime = millis();
    unsigned long ElapsedTime = CurrentTime - StartTime;
  Serial.println(ElapsedTime);
#endif
    
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID: "));
    //dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    uint32_t  uid;
    memcpy(&uid,mfrc522.uid.uidByte,sizeof(uid));
    Serial.println(uid, HEX);
    //Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
    }


 
#ifdef PC_DEBUG  
    CurrentTime = millis();
    ElapsedTime = CurrentTime - StartTime;
  Serial.println(ElapsedTime);
#endif

    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);
    uint32_t tStart, tStop;
    bool eroare = false;
    
    Serial.println(F("Reading data from card !!!!"));
    int32_t timpiIntermediari[POST_FINAL+1];
    for(int i = 0; i<=POST_FINAL; i++){
      //Serial.println();
      Serial.print(F("POST#")); Serial.print(i); Serial.print(F(": "));
      status = read_sector(i+START_SECTOR,buffer, size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_read() --- failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        blinkRed(5);
        mfrc522.PICC_HaltA();
        // Stop encryption on PCD
        mfrc522.PCD_StopCrypto1();
        return;
      }
      //dump_byte_array(buffer, 16); Serial.println();
      uint32_t timp[4];
      memcpy(timp, buffer, 16);
      byte c = 0;
      for(int j=3;j>=0;j--){
        if(timp[j] != 0){
          c++;
          //if(c>1) Serial.print(F("         "));
          //Serial.println(timp[j]);

          RTCDateTime timp2 = fromUnixTime(timp[j]);
          Serial.print(DS3231_clock.dateFormat("H:i:s ", timp2));
         
          if(i==0){
            tStart=timp[j];
          }
          if(i==POST_FINAL){
            tStop=timp[j];
          }
        } else {
           Serial.print(F("         "));
        }
        timpiIntermediari[i] = timp[j]-tStart; 
        
      }
      if(i>0){
        Serial.print(": ");
//        Serial.print(timpiIntermediari[i]);
//        Serial.print("H");
//        
//        Serial.print(timpiIntermediari[i-1]);
//        Serial.print(": ");
        if((timpiIntermediari[i]-timpiIntermediari[i-1]) > 0){
        Serial.println(timpiIntermediari[i]-timpiIntermediari[i-1]);
        } else { 
          Serial.println("ERROR !!!!!");
          eroare = true;
        }
      }else{
        Serial.println();
      }
    }
    Serial.println(F("======================="));
    Serial.print(F("| TIMP TOTAL ")); 
    if(eroare== true) {
      Serial.print("ERROR !!");
    } else {
      Serial.print(TimeToString(tStop - tStart));
    }
    Serial.println(F(" |"));
    Serial.println(F("======================="));


Serial.print("$");
Serial.print(uid, HEX);
Serial.print(",");
    if(eroare== true) {
      Serial.print("ERROR !!");
    } else {
      Serial.print(TimeToString(tStop - tStart));
    }
//      for(int ii = 0; ii<=POST_FINAL; ii++){
//        Serial.print(",");
//        Serial.print( timpiIntermediari[ii] );
//        
//      }
Serial.println();

#ifdef PC_DEBUG 
    CurrentTime = millis();
    ElapsedTime = CurrentTime - StartTime;
    //Serial.println(ElapsedTime);
#endif    
     
   digitalWrite(LED_GREEN, LOW);  
   digitalWrite(LED_RED, LOW);  
   digitalWrite(LED_BLUE, LOW); 
    
    digitalWrite(LED_GREEN, HIGH);  
    //blinkGreen(10);
//    delay(2000);
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}



MFRC522::StatusCode read_sector(byte sector, byte buffer[], byte size){

    byte blockAddr      = sector * 4;
    byte trailerBlock   = sector * 4 + 3;

    MFRC522::StatusCode status;
    //Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return status;
    }
  
    //byte size = sizeof(buffer);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return status;
    }
    //Serial.println();
    //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);Serial.println();
    
    return MFRC522::STATUS_OK;

}





void clear_sector(byte sector, byte dataBlock[]){




    //byte sector         = 2;
    byte blockAddr      = sector * 4;
    byte trailerBlock   = sector * 4 + 3;

    MFRC522::StatusCode status;
     
    //Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
  

    // Write data to the block
    //Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    //Serial.println(F(" ..."));
    //dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr+1, dataBlock, 16);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr+2, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    //Serial.println();
    //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);Serial.println();
    


}



char * TimeToString(unsigned long t)
{
 static char str[12];
 long h = t / 3600;
 t = t % 3600;
 int m = t / 60;
 int s = t % 60;
 sprintf(str, "%02ld:%02d:%02d", h, m, s);
 return str;
}


void blinkRed(byte n){
     digitalWrite(LED_GREEN, LOW);  
   digitalWrite(LED_RED, LOW);  
   digitalWrite(LED_BLUE, LOW); 
  for(byte i=0; i<n; i++){
    digitalWrite(LED_RED, HIGH);  
    delay(200);
    digitalWrite(LED_RED, LOW);  
    delay(100);
  }
}


void blinkGreen(byte n){
     digitalWrite(LED_GREEN, LOW);  
   digitalWrite(LED_RED, LOW);  
   digitalWrite(LED_BLUE, LOW); 
  for(byte i=0; i<n; i++){
    //Serial.println(i);
    digitalWrite(LED_GREEN, HIGH);  
    delay(200);
    digitalWrite(LED_GREEN, LOW);  
    delay(100);
  }
}


void printDigits(byte digits){
 // utility function for digital clock display: prints colon and leading 0
 if(digits < 10)
   Serial.print('0');
 Serial.print(digits,DEC);  
}
