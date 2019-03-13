/*
   Automated Drink Mixer
   Dave Ha, Eric Mysliwiec, Matt Gross
*/

/* MFRC522: RFID */
#include <SPI.h>
#include <MFRC522.h>

/* MCP23017: 2->16 Expansion Chipset */
#include <Wire.h>
#include "Adafruit_MCP23017.h"

/* WIFI Connection*/
#include <ESP8266WiFi.h>
#include "Gsender.h"

/* LCD */
#include <LiquidTWI2.h>

/******** Configurations and pin definitions **********/

// MFRC522 RFID
constexpr uint8_t RST_PIN = 3;     // GPIO# 5->3; configurable
constexpr uint8_t SS_PIN  = 2;      // GPIO# 4->2; configurable
MFRC522 rfid(SS_PIN, RST_PIN);     // Instance of the class
MFRC522::MIFARE_Key key;

// MCP23017 Expansion
Adafruit_MCP23017 mcp0;
Adafruit_MCP23017 mcp1;

// WiFi Config
const char* ssid = "303Guest";              // WIFI network name
const char* password = "ece391385";         // WIFI network password
uint8_t connection_state = 0;               // Connected to WIFI or not
uint16_t reconnect_interval = 5000;        // If not connected wait time to try again
uint8_t WIFI_LED_PIN = 8;                   // Physical pin 1 on MCP0; subject to change

// LCD Config
LiquidTWI2 lcd (0x20);

// Buttons Config (MCP0)
enum ButtonPress {
  LEFT,
  RIGHT,
  SELECT,
  BACK,
  UNDEFINED
};
uint8_t BUTTON_LEFT_PIN   =  7;
uint8_t BUTTON_RIGHT_PIN  =  6;
uint8_t BUTTON_BACK_PIN   =  5;
uint8_t BUTTON_SELECT_PIN =  4;
ButtonPress buttonPrev = UNDEFINED; /* default state */


void setup() {
  Serial.begin(115200);
  Serial.println(F("..System Booting........"));
  
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
  Serial.println();

  // MCP23017 Expansion Module Initialization
  mcp0.begin(0);
  mcp1.begin(1);
  mcp0.pinMode(BUTTON_LEFT_PIN, INPUT);
  mcp0.pinMode(BUTTON_RIGHT_PIN, INPUT);
  mcp0.pinMode(BUTTON_BACK_PIN, INPUT);
  mcp0.pinMode(BUTTON_SELECT_PIN, INPUT);
  mcp1.pinMode(WIFI_LED_PIN, OUTPUT);
  Serial.println(F("...MCP23017 Expansion Module Initialized."));
  Serial.println();

  // WiFi Initialization
  Serial.print(F("Connecting to "));
  WiFi.begin(ssid, password);
  Serial.println(ssid);
  uint8_t i = 0;

  /* UNCOMMENT TO ENABLE WIFI
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
    delay(200);
    Serial.print(F("."));
    }
    if(WiFi.status() == WL_CONNECTED){      // if connected to WIFI
    mcp0.digitalWrite(WIFI_LED_PIN,HIGH);
    Serial.println(F("...WiFi connection: ESTABLISHED"));
    Serial.print(F("Got IP address: "));
    Serial.println(WiFi.localIP());
    }else{
    Serial.println(F("ERROR: WiFi could not connect, check ssid and password."));
    }
    Serial.println();
  */

  // LCD Initialization
  lcd.setMCPType (LTI_TYPE_MCP23017);
  lcd.begin (16, 2); /* dimension */
  displayWelcomeScreen();
  // MCP WiFi LED Test (With regards to current draw and resistance)
  mcp1.digitalWrite(WIFI_LED_PIN, HIGH);
}

void loop() {

  // STEP 1: SCAN RFID

  if ( ! rfid.PICC_IsNewCardPresent()) return; // Look for new cards
  if ( ! rfid.PICC_ReadCardSerial()) return;   // Verify if the NUID has been read
 
  Serial.println();
  Serial.println(F("**Card Detected:**"));
  Serial.print(F("NUID tag #:"));
  String UID = byteToString(rfid.uid.uidByte, rfid.uid.size);
  Serial.println(UID);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  Serial.println("Select your drink: (*display first recipe name)");

  // STEP 2: SHOW MENU, HAVE THE USER SELECT W/ BUTTONPRESS

  delay(1000);  
  while (buttonPrev != SELECT) {
    // LEFT BUTTON PRESSED 
    if (mcp0.digitalRead(BUTTON_LEFT_PIN) == HIGH) {
      Serial.println("Left Pressed : Show prev recipe");
    }
    // RIGHT BUTTON PRESSED 
    if (mcp0.digitalRead(BUTTON_RIGHT_PIN) == HIGH) {
      Serial.println("Right Pressed : Show next recipe");
    }
    // SELECT BUTTON PRESSED 
    if (mcp0.digitalRead(BUTTON_SELECT_PIN) == HIGH) {
      Serial.println("Select Pressed : Make selected recipe");
      break;
    }
    // BACK BUTTON PRESSED 
    if (mcp0.digitalRead(BUTTON_BACK_PIN) == HIGH) {
      Serial.println("Back Pressed : Order canceled, back to main menu");
      displayWelcomeScreen();
      return;
    }
    delay(100);
  }
  
  // STEP 3: MAKE DRINKS
  Serial.println("Making Selected Drink...");

  // STEP 4: RESET POSITION
  Serial.println("Recalibrating Disc Position...");

  // STEP 5: SEND EMAIL
  /* UNCOMMENT TO ENABLE
    Serial.println(F("Sending Order to the Server..."));
    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    if(gsender->Subject(UID)->Send("handsfreemixer445@gmail.com", "drinkName")) {
      Serial.println("Message sent.");
    } else {
      Serial.print("Error sending message: ");
      //Serial.println(gsender->getError());
    }
    Serial.println();
  */
  
  // STEP 6: ON LCD: DISPLAY WELCOME MESSAGE, ASK TO SCAN RFID
  displayWelcomeScreen();


  // RETURN TO STEP 1
  delay(1000);
}

// HELPER FUNCTIONS BELOW**********************************************

void displayWelcomeScreen(){
  lcd.setCursor(0, 0);
  lcd.print("    Welcome!    ");
  lcd.setCursor(0, 1);
  lcd.print(" Scan your RFID ");
  Serial.println(" Welcome! Please Scan your RFID ");
}

// WIFI HELPER FUNCTIONS (currently unused)*********************************
uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
  static uint16_t attempt = 0;
  Serial.print("Connecting to ");
  if (nSSID) {
    WiFi.begin(nSSID, nPassword);
    Serial.println(nSSID);
  } else {
    WiFi.begin(ssid, password);
    Serial.println(ssid);
  }

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50)
  {
    delay(200);
    Serial.print(".");
  }
  ++attempt;
  Serial.println("");
  if (i == 51) {
    Serial.print("Connection: TIMEOUT on attempt: ");
    Serial.println(attempt);
    if (attempt % 2 == 0)
      Serial.println("Check if access point available or SSID and Password\r\n");
    return false;
  }
  Serial.println("Connection: ESTABLISHED");
  Serial.print("Got IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void Awaits()
{
  uint32_t ts = millis();
  while (!connection_state)
  {
    delay(50);
    if (millis() > (ts + reconnect_interval) && !connection_state) {
      connection_state = WiFiConnect();
      ts = millis();
    }
  }
}

// RFID HELPER FUNCTIONS ***********************************************

String byteToString(byte *buffer, byte bufferSize) {
  String UID;
  for (byte i = 0; i < bufferSize; i++) {
    UID += String(buffer[i] < 0x10 ? "0" : "");
    UID += String(buffer[i], HEX);
  }
  UID.toUpperCase();
  return UID;
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
