/*
 * Automated Drink Mixer 
 * Dave Ha, Eric Mysliwiec, Matt Gross
 */


/* MFRC522: RFID */
#include <SPI.h>
#include <MFRC522.h>

/* MCP23017: 2->16 Expansion Chipset */
#include <Wire.h>
#include "Adafruit_MCP23017.h"

/* Configurations and pin definitions */
// MFRC522 RFID
constexpr uint8_t RST_PIN = 3;     // GPIO# 5->3; configurable
constexpr uint8_t SS_PIN = 2;      // GPIO# 4->2; configurable
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 

// MCP23017 Expansion
Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;
 
void setup() { 
  Serial.begin(115200);

  // RFID Module Initialization
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("...RFID Module Initialized."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();

  // MCP23017 Expansion Module Initialization
  mcp0.begin(0);
  mcp1.begin(1);
  Serial.println(F("...MCP23017 Expansion Module Initialized."));

  mcp0.pinMode(8,OUTPUT);
  mcp1.pinMode(8,OUTPUT);
  mcp0.digitalWrite(8,HIGH);
  mcp1.digitalWrite(8,HIGH);
  Serial.println();
}
 
void loop() {
  unsigned long time_init = millis();
  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  
  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.println();
  Serial.println(F("**Card Detected:**"));
  Serial.print("Time(ms) Spent: ");
  Serial.println(millis() - time_init);
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.print(F("In dec: "));
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  // Halt PICC
  rfid.PICC_HaltA(); 
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  delay(1000);
}
 
 
/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
 
/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
