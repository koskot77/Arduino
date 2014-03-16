#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Barometer.h"
#include "DS1307.h"

float temperature;
float pressure;
float atm;
float altitude;

LiquidCrystal_I2C lcd(0x20,20,4);
Barometer myBarometer;
DS1307 clock;

void setup()
{
//  Serial.begin(57600);
  clock.begin();
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  myBarometer.init();

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.setCursor(0, 1);
  lcd.print("Temperature: ");
  lcd.setCursor(0, 2);
  lcd.print("Preassure: ");
//  lcd.setCursor(10, 2);
//  lcd.print(" (atmosphere: ");
//  lcd.setCursor(15 2);
//  lcd.print(")");
  lcd.setCursor(0, 3);
  lcd.print("Altitude: ");
}

void loop()
{

   clock.getTime();
   lcd.setCursor(6, 0);

   if( clock.hour<10 ) lcd.print("0");
   lcd.print(clock.hour, DEC);
   lcd.print(":");
   if( clock.minute<10 ) lcd.print("0");
   lcd.print(clock.minute, DEC);
   lcd.print(":");
   if( clock.second<10 ) lcd.print("0");
   lcd.print(clock.second, DEC);
   lcd.print(" ");
   lcd.print(clock.month, DEC);
   lcd.print("/");
   lcd.print(clock.dayOfMonth, DEC);
//   lcd.print("/");
//   lcd.print(clock.year, DEC);
//   lcd.print(" ");
//   lcd.print(clock.dayOfMonth);
//   lcd.print("*");
//   switch (clock.dayOfWeek){
//     case MON: lcd.print("MON"); break;
//     case TUE: lcd.print("TUE"); break;
//     case WED: lcd.print("WED"); break;
//     case THU: lcd.print("THU"); break;
//     case FRI: lcd.print("FRI"); break;
//     case SAT: lcd.print("SAT"); break;
//     case SUN: lcd.print("SUN"); break;
//   }


   temperature = myBarometer.bmp085GetTemperature(myBarometer.bmp085ReadUT()); //Get the temperature, bmp085ReadUT MUST be called first
   pressure = myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP());//Get the temperature
   altitude = myBarometer.calcAltitude(pressure); //Uncompensated caculation - in Meters 
   atm = pressure / 101325; 

   lcd.setCursor(13, 1);
   lcd.print(temperature, 2); //display 2 decimal places
   lcd.print(" C");

   lcd.setCursor(12, 2);
   lcd.print(pressure, 0); //whole number only.
   lcd.print(" Pa");

   //lcd.print("Ralated Atmosphere: ");
   //lcd.println(atm, 4); //display 4 decimal places

   lcd.setCursor(12, 3);
   lcd.print(altitude, 2); //display 2 decimal places
   lcd.print(" m");

   delay(1000); //wait a second and get values again.
}

