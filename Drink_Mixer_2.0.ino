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
//#include "Gsender.h"

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
uint16_t reconnect_interval = 5000;         // If not connected wait time to try again
uint8_t WIFI_LED_PIN = 8;                   // Physical pin 1 on MCP1

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

// Inventory and Recipes
char * INVENTORY[] = {"Water", "Vodka", "OJ", "Coke", "Rum"}; // drink ingredients/supplies
int POSITIONS[] = {0, 80, 160, 240, 320}; // each position is number of clockwise ticks, each of 0.9 degrees
int NUM_RECIPES = 4;
int NUM_INGREDIENTS = 5;
char * RECIPE_NAME[] = {" Iced Water", " Cold OJ", " Rum & Coke", " Screwdriver"}; // change NUM_RECIPES if adding more
int RECIPE[4][5] = {
  {178, 0, 0, 0, 0},  // Iced Water
  {0, 0, 178, 0, 0},  // Cold OJ
  {0, 0, 0, 133, 45}, // Rum & Coke
  {0, 60, 118, 0, 0}  // Screwdriver
};
int CURRENT_DRINK_IDX = 0;

// Stepmotor, cup positioning
float CUP_POSITION = 0;
enum Direction{
  CW, // 0
  CCW // 1
};
Direction DIRECTION = CW;


// MOTOR/FLOWMETER PINS
uint8_t STEPPER_SIGNAL_PIN =     9;
uint8_t STEPPER_DIRECTION_PIN = 10;
uint8_t FLOWMETER_5_PIN =       11;
uint8_t FLOWMETER_4_PIN =       12;
uint8_t FLOWMETER_3_PIN =       13;
uint8_t FLOWMETER_2_PIN =       14;
uint8_t FLOWMETER_1_PIN =       15;
uint8_t PUMP_1_SIGNAL_PIN =      0; // connected to PUMP_1_GND on PCB
uint8_t PUMP_2_SIGNAL_PIN =      1; //              PUMP_2_GND
uint8_t PUMP_3_SIGNAL_PIN =      2; //              PUMP_3_GND ...
uint8_t PUMP_4_SIGNAL_PIN =      3;
uint8_t PUMP_5_SIGNAL_PIN =      4;

uint8_t LASER_SIGNAL_PIN =       6;
uint8_t PHOTOCELL_PIN =          7;

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
  mcp1.pinMode(STEPPER_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(STEPPER_DIRECTION_PIN, OUTPUT);
  mcp1.pinMode(FLOWMETER_1_PIN, INPUT);
  mcp1.pinMode(FLOWMETER_2_PIN, INPUT);
  mcp1.pinMode(FLOWMETER_3_PIN, INPUT);
  mcp1.pinMode(FLOWMETER_4_PIN, INPUT);
  mcp1.pinMode(FLOWMETER_5_PIN, INPUT);
  mcp1.pinMode(PUMP_1_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(PUMP_2_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(PUMP_3_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(PUMP_4_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(PUMP_5_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(LASER_SIGNAL_PIN, OUTPUT);
  mcp1.pinMode(PHOTOCELL_PIN, INPUT);
  Serial.println(F("...MCP23017 Expansion Module Initialized."));
  Serial.println();

  /* UNCOMMENT TO ENABLE WIFI
  Serial.print(F("Connecting to "));
  WiFi.begin(ssid, password);
  Serial.println(ssid);
  uint8_t i = 0;

  while(WiFi.status()!= WL_CONNECTED && i++ < 50)
  {
    delay(200);
    Serial.print(F("."));
  }
  if(WiFi.status() == WL_CONNECTED){      // if connected to WIFI
    mcp1.digitalWrite(WIFI_LED_PIN,HIGH);
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
  lcd.begin (16, 2); // dimension 
  displayWelcomeScreen();

  // MCP WiFi LED Test
  mcp1.digitalWrite(WIFI_LED_PIN, HIGH);

  // Keep all Pumps Low (although they should be by default)
  mcp1.digitalWrite(PUMP_1_SIGNAL_PIN, LOW);
  mcp1.digitalWrite(PUMP_2_SIGNAL_PIN, LOW);
  mcp1.digitalWrite(PUMP_3_SIGNAL_PIN, LOW);
  mcp1.digitalWrite(PUMP_4_SIGNAL_PIN, LOW);
  mcp1.digitalWrite(PUMP_5_SIGNAL_PIN, LOW);
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, HIGH); 

  Serial.println("Setup Done!");
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
  lcd.setCursor(0, 0);
  lcd.print(" Select Drink:  ");
  CURRENT_DRINK_IDX = 0;
  displayDrink(CURRENT_DRINK_IDX);

  // STEP 2: SHOW MENU, HAVE THE USER SELECT W/ BUTTONPRESS

  delay(1000);
  while (buttonPrev != SELECT) {
    // LEFT BUTTON PRESSED
    if (mcp0.digitalRead(BUTTON_LEFT_PIN) == HIGH) {
      Serial.println("Left Pressed : Show prev recipe");
      Serial.println(CURRENT_DRINK_IDX);
      CURRENT_DRINK_IDX = (CURRENT_DRINK_IDX == 0)? NUM_RECIPES-1 : --CURRENT_DRINK_IDX;
      displayDrink(CURRENT_DRINK_IDX);
    }
    // RIGHT BUTTON PRESSED
    if (mcp0.digitalRead(BUTTON_RIGHT_PIN) == HIGH) {
      Serial.println("Right Pressed : Show next recipe");
      Serial.println(CURRENT_DRINK_IDX);
      CURRENT_DRINK_IDX = (++CURRENT_DRINK_IDX) % NUM_RECIPES;
      displayDrink(CURRENT_DRINK_IDX);
    }
    // SELECT BUTTON PRESSED
    if (mcp0.digitalRead(BUTTON_SELECT_PIN) == HIGH) {
      Serial.println("Select Pressed : Make selected recipe");
      break;
    }
    // BACK BUTTON PRESSED
    if (mcp0.digitalRead(BUTTON_BACK_PIN) == HIGH) {
      Serial.println("Back Pressed : Order canceled, back to main menu");
      Serial.println(CURRENT_DRINK_IDX);
      displayWelcomeScreen();
      lcd.setCursor(0, 0);
      lcd.print("Order Cancelled!");
      return;
    }
    delay(100);
  }

  recalibrate();
  
  // STEP 3: MAKE DRINKS
  Serial.println("Making Selected Drink...");
  lcd.setCursor(0,0);
  lcd.print("  Making Drink  ");
  delay(100);

  makeDrink(CURRENT_DRINK_IDX);

  lcd.print("Drink Finished! ");
  delay(1000);
  
  // STEP 4: RESET POSITION
  lcd.setCursor(0,1);
  lcd.print("Recalibrating...");
  recalibrate();

  // STEP 5: SEND EMAIL
  /* UNCOMMENT TO ENABLE
    Serial.println(F("Sending Order to the Server..."));
    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    if(gsender->Subject(UID)->Send("handsfreemixer445@gmail.com", RECIPE_NAME[CURRENT_DRINK_IDX])) {
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

}

// HELPER FUNCTIONS BELOW**********************************************
void pour(){
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, HIGH); 
  delay(50);
  mcp1.digitalWrite(PUMP_1_SIGNAL_PIN, HIGH);
  delay(5000);
  mcp1.digitalWrite(PUMP_1_SIGNAL_PIN, LOW);
  delay(50);
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, LOW);
}

// STEPPER MOTOR HELPER FUNCTIONS(MAKING DRINKS)
void moveCupTo(int target_pos){
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, LOW); 
  delay(50);
  if(CUP_POSITION == target_pos) return;
  
  // choose direction of shortest distance; 1 = CW, 0 = CCW
  int distance = target_pos - CUP_POSITION; // clockwise distance
  if(distance < 0) distance += 400;

//  if(distance < 200){
//    DIRECTION = CW;
//  }else{
//    DIRECTION = CCW;
//    distance = 400 - distance;
//  }

  for(int i=0;i<distance;++i){
    mcp1.digitalWrite(STEPPER_SIGNAL_PIN, HIGH);
    delay(20);
    mcp1.digitalWrite(STEPPER_SIGNAL_PIN, LOW);
    delay(20);
  }
  CUP_POSITION = target_pos;  
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, HIGH); 
}


void makeDrink(int CURRENT_DRINK_IDX){
  moveCupTo(POSITIONS[1]);
  delay(1000);
  pour();
  moveCupTo(POSITIONS[0]);
  delay(1000);
  pour();
  moveCupTo(POSITIONS[2]);
  delay(1000);
  pour();
  moveCupTo(POSITIONS[3]);
  delay(1000);
  pour();
  moveCupTo(POSITIONS[4]);
  delay(1000);
  pour();
  
//  for(int i=0;i<NUM_INGREDIENTS;++i){
//    if(RECIPE[CURRENT_DRINK_IDX][i] == 0) continue; // skip if 0 amount is to be poured
//    moveCupTo(POSITIONS[i]);
//    // note: pump pin #'s are increments of each other; pump_1 = 0, pump_2 = 1, pump_3 = 2, etc
//    pour(i, RECIPE[CURRENT_DRINK_IDX][i]); // pump idx(0~4),amount) 
//    delay(500);
//  }
}


void recalibrate(){
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, LOW); 
  lcd.setCursor(0,0);
  lcd.print("  Calibrate     ");
  delay(1000);
  // set shorter direction to 0
  //DIRECTION = (CUP_POSITION > 200)? CW : CCW;
  int threshold = 0;

  // turn laser on
  mcp1.digitalWrite(LASER_SIGNAL_PIN, HIGH);

  // move until laser value 0
  while(mcp1.digitalRead(PHOTOCELL_PIN) > threshold){
    mcp1.digitalWrite(STEPPER_SIGNAL_PIN, HIGH);
    delay(20);
    mcp1.digitalWrite(STEPPER_SIGNAL_PIN, LOW);
    delay(20);
  }

  CUP_POSITION = 0;
  lcd.setCursor(0,0);
  lcd.print("  Done          ");
  mcp1.digitalWrite(STEPPER_DIRECTION_PIN, HIGH); 
  delay(1000);
}

// LCD HELPER FUNCTIONS
void displayWelcomeScreen() {
  lcd.setCursor(0, 0);
  lcd.print("    Welcome!    ");
  lcd.setCursor(0, 1);
  lcd.print(" Scan your RFID ");
  Serial.println(" Welcome! Please Scan your RFID ");
}

void displayDrink(int idx){ // display drink on the bottom row of the lcd
  if(idx >= NUM_RECIPES) Serial.println(F("drink idx invalid"));
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(1, 1);
  lcd.print(RECIPE_NAME[idx]);
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
