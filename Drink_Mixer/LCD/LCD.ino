#include <Wire.h>
#include <LiquidTWI2.h>
// Connection via i2c, default address (0)
LiquidTWI2 lcd (0x20);
void setup () {
  lcd.setMCPType (LTI_TYPE_MCP23017);
  lcd.begin (16, 2);
  lcd.print ( "Hi");
  lcd.setCursor (0, 1);
  lcd.print ("Welcome2 Chili's");
}
void loop () {
  // prepare to write in column 0, line 1
  // (note: row 1 is the second one, the count starts from 0)
  //lcd.setCursor (0, 1);
  // write the number of seconds (in the position set before)
  //lcd.print (millis () / 1000);
}
