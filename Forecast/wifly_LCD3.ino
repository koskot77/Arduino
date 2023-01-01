#include <SoftwareSerial.h>
#include "WiFly.h"
#include <TimeLib.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x20,20,4);  // set the LCD address to 0x27 for a 20 chars and 4 line display
uint8_t degChar[8]  = {0x4,0xa,0x4,0x0,0x0,0x0,0x0};

#define SSID      "UPC1344544"
#define KEY       "C3w7ysjkmRxt"
#define AUTH      WIFLY_AUTH_WPA2_PSK

#define BKL_PIN   12
bool backlight = false;
bool released  = true;
int time_zone = 2; // hardcode CEST timezone: GMT+2

// Pins' connection
// Arduino       WiFly
//  10   <---->    TX
//  9    <---->    RX
SoftwareSerial uart(10, 9);
WiFly wifly(&uart);

#include "HTTPClient.h"
HTTPClient http;

unsigned long time_of_last_update = 0;
unsigned short refreshes = 0;

void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  pinMode(BKL_PIN, INPUT_PULLUP);

  uart.begin(9600);

  Serial.begin(9600);
  Serial.println(F("- Activating WIFLY -"));
  Serial.println(F("WiFly Version: "));
  Serial.println(wifly.version());

  lcd.setCursor(0, 0);
  lcd.print(F("- Activating WIFLY -"));
  lcd.setCursor(0, 2);
  lcd.println("WiFly Version: ");
  lcd.setCursor(15, 2);
  lcd.print(wifly.version());

  // wait for initilization of wifly
  delay(3000);
  
  uart.begin(9600);     // WiFly UART Baud Rate: 9600

  pinMode(2, INPUT); // for interrupts, never pull it up, it'll damage WiFy permanently

  // check if WiFly is associated with AP(SSID)
  if (!wifly.isAssociated(SSID)) {
    while (!wifly.join(SSID, KEY, AUTH)) {
      Serial.print(F("Fail join "));
      Serial.println(SSID);
      Serial.println(F("Wait 1 second, retry"));
      lcd.setCursor(0, 1);
      lcd.print(F("Fail join "));
      lcd.println(SSID);
      lcd.setCursor(0, 2);
      lcd.print(F("Wait 1 second, retry"));
      delay(1000);
    }

    Serial.println(F("Join success"));
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(F("Join success "));
    delay(1000);
    wifly.sendCommand("set time address 157.161.57.2\r");
    wifly.sendCommand("set time enable 1\r");
    wifly.save();    // save configuration, 
    wifly.sendCommand("time\r");
  }

  lcd.clear();
}



bool set_time()
{
  for(int attempt=0; attempt<10; )
  {
    wifly.clear();
    if( !wifly.commandMode() ) return false;
    wifly.sendCommand("show t t\r","\nTime=");
    delay(30);

    static char response[64];
    char *pos = response, *end = response+64;
    memset(response, '\0', sizeof(response));

//while (wifly.available() && pos < end) *pos++ = (char)wifly.read();
    char c;
    while (wifly.receive((uint8_t *)&c, 1, 300) > 0 && pos < end) {
      *pos++ = c;
      Serial.print((char)c);
    }

    wifly.clear();
    char *sec_1970  = strstr(response, "RTC=");
    if( sec_1970 == NULL ){ ++attempt; continue; }
    char *term = strchr(sec_1970, '\n');
    if( term == NULL ){ ++attempt; continue; }
    *term = '\0';
    setTime(strtol(sec_1970 + 4, NULL, 10));
    return true;
  }
  Serial.println(F("Failure to read time"));
  return false;
}

void print_time(){

  lcd.setCursor(0, 0);
  lcd.print("'");
  lcd.print(year()-2000);
  lcd.print(" ");
  switch( month() ){
    case 1: lcd.print(F("Jan")); break;
    case 2: lcd.print(F("Feb")); break;
    case 3: lcd.print(F("Mar")); break;
    case 4: lcd.print(F("Apr")); break;
    case 5: lcd.print(F("May")); break;
    case 6: lcd.print(F("Jun")); break;
    case 7: lcd.print(F("Jul")); break;
    case 8: lcd.print(F("Aug")); break;
    case 9: lcd.print(F("Sep")); break;
    case 10: lcd.print(F("Oct")); break;
    case 11: lcd.print(F("Nov")); break;
    case 12: lcd.print(F("Dec")); break;
    default: lcd.print(month()); break;
  }
  lcd.print(" ");
  if(day()<10) lcd.print(0);
  lcd.print(day());
  lcd.print(" ");
  if(hour()<10) lcd.print(0);
  lcd.print(hour());
  lcd.print(":");
  if(minute()<10) lcd.print(0);
  lcd.print(minute());

  lcd.print(" ");
  switch( weekday() ){
    case 1: lcd.print(F("Sun")); break;
    case 2: lcd.print(F("Mon")); break;
    case 3: lcd.print(F("Tue")); break;
    case 4: lcd.print(F("Wed")); break;
    case 5: lcd.print(F("Thr")); break;
    case 6: lcd.print(F("Fri")); break;
    case 7: lcd.print(F("Sat")); break;
    default: lcd.print(weekday()); break;
  }
  
}

void loop() {

  // toggle backlight; with INPUT_UP it reads 0 when pressed
  if( digitalRead(BKL_PIN) == 0 ){
    if( released ){
      if( backlight ){
        lcd.noBacklight();
        backlight = false;
      } else {
        lcd.backlight();
        backlight = true;
      }
      released = false;
    }
  } else {
    released = true;
  }

  // do not update more than once a half a minute
  if( time_of_last_update != 0 && millis() - time_of_last_update < 30000 ){
    //delay(1000);
    return;
  }
  time_of_last_update = millis();

  // Adjust RTC using NTP of wifly
  if( !set_time() ){
    lcd.setCursor(0, 0);
    lcd.print(F("Failed setting time "));
    delay(100);
    return;
  } else {
    adjustTime(60*60*time_zone);
  }
  // render time on LCD
  print_time();

  if( refreshes != 0 && ++refreshes < 10 ) return;

  refreshes = 1;

  if( http.get("http://192.168.0.234:8080/v1/weather", 10000) < 0 ){
      lcd.setCursor(0, 1);
      lcd.print(F("Failed connect :8080"));
      if (!wifly.isAssociated(SSID)) {
        while (!wifly.join(SSID, KEY, AUTH)) {
          Serial.print(F("Fail join "));
          Serial.println(SSID);
          Serial.println(F("Wait 1 second, retry"));
          lcd.setCursor(0, 1);
          lcd.print(F("Fail join "));
          lcd.println(SSID);
          lcd.setCursor(0, 2);
          lcd.print(F("Wait 1 second, retry"));
          delay(1000);
        }
      }
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print(F("   Rejoin success   "));
      delay(1000);
      time_of_last_update = 0;
      refreshes = 0;
      return;
  }

  static char response[400];
  memset(response,0,sizeof(response));

  struct Data {
    char *temp;
    char *pres;
    char *wind;
    char *gust;
    char *humi;
    char *prec;
    char *sun;
    char *moon;
    char *lune;
    char *zone;
  } data;
  memset((void*)&data, 0, sizeof(data));

  wifly.clear();
  while(wifly.receive((uint8_t *)&response, 399, 1000) > 0) {

    data.temp = strstr(response, "Temperature: ");
    data.pres = strstr(response, "Presure: ");
    data.wind = strstr(response, "Wind: ");
    data.gust = strstr(response, "Gusts: ");
    data.humi = strstr(response, "Humidity: ");
    data.prec = strstr(response, "Precepitation: ");
    data.sun  = strstr(response, "Sun: ");
    data.moon = strstr(response, "Moon: ");
    data.lune = strstr(response, "Neumond: ");
    data.zone = strstr(response, "Time: ");

    if( data.temp != NULL ){
      data.temp += 13;
      *(data.temp+4) = '\0';
    }
    if( data.pres != NULL ){
      //*(data.pres-1) = '\0';
      data.pres += 9;
    }
    if( data.wind != NULL ){
      *(data.wind-1) = '\0';
      data.wind += 6;
    }
    if( data.wind != NULL ){
      *(data.gust-1) = '\0';
      data.gust += 7;
    }
    if( data.humi != NULL ){
      *(data.humi-1) = '\0';
      data.humi += 10;
    }
    if( data.prec != NULL ){
      *(data.prec-1) = '\0';
      data.prec += 15;
    }
    if( data.prec != NULL ){
      *(data.sun-1) = '\0';
      data.sun += 5;
    }
    if( data.moon != NULL ){
      *(data.moon-1) = '\0';
      data.moon += 6;
    }
    if( data.moon != NULL ){
      data.moon   += 0;
    }
    if( data.zone != NULL ){
      // assuming time is in RFC3339
      char *zone = data.zone + 25;
      zone[3] = '\0';
      int sign  = (zone[0] == '+' ? 1 : -1);
      zone = (zone[1] == '0' ? zone+2 : zone+1);
      int tz = strtol(zone, NULL, 10) * sign;
      if( time_zone != tz ){
        adjustTime(60*60*(tz-time_zone));
        time_zone = tz;
        print_time();
      }
    }
  }

  Serial.print(F("T: "));
  Serial.print(data.temp);
  Serial.println(F("C"));
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  char *sep;
  if( (sep = strchr(data.temp, '.')) != NULL ){
    sep += 2;
    *sep++ = '\0';
  } else {
    if( (sep = strchr(data.temp, ' ')) != NULL )
      *sep++ = '\0';
  }
  if( sep != NULL )
    lcd.print(data.temp);
  else {
    lcd.print("---");
    time_of_last_update = 0;
    refreshes = 10;
  }

  lcd.print(char(223));
  lcd.print(F("C"));

  Serial.print(F("Presure: "));
  Serial.print(data.pres);
  sep = strchr(data.pres,' ');
  *sep++ = '\0';
  int mmHg = int(strtol(data.pres,NULL,10)/1.33322 + 0.5);
  Serial.print(F(" = "));
  Serial.println(mmHg);
  Serial.print(F("mm"));
  lcd.setCursor(8, 1);
//  lcd.print("P: ");
  lcd.print(mmHg);
  lcd.print("mm");

//  Serial.print(F("Precipitation: "));
//  Serial.println(data.prec);
//  lcd.setCursor(12, 1);
//  lcd.print(data.prec);
  Serial.print(F("Humidity: "));
  Serial.println(data.humi);
  lcd.setCursor(15, 1);
  lcd.print(data.humi);

  Serial.print(F("Wind: "));
  Serial.println(data.wind);
  Serial.print(F("Gusts: "));
  Serial.println(data.gust);
  lcd.setCursor(0, 2);
  lcd.print("Wind:               ");
  lcd.setCursor(0, 2);
  lcd.print(F("Wind: "));
  lcd.print(data.wind);
  lcd.print(" (<");
  sep = strchr(data.gust,' ');
  *sep++ = '\0';
  lcd.print(data.gust);
  lcd.print(")");

  Serial.print(F("Sun: "));
  Serial.println(data.sun);
  lcd.setCursor(0, 3);
  lcd.print("Sun:                ");
  lcd.setCursor(0, 3);
  lcd.print(F("Sun: "));
  sep = strchr(data.sun,' ');
  *sep++ = '\0';
  lcd.print(data.sun);
  lcd.print(" - ");
  lcd.print(sep);
  
  if( sep == NULL ){
    time_of_last_update = 0;
    refreshes = 10;
  }

}
