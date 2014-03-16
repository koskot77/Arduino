#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x20,20,4);
uint8_t degChar[8]  = {0x4,0xa,0x4,0x0,0x0,0x0,0x0};

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x00, 0x27, 0x13, 0xB8, 0x04, 0xF7 };
IPAddress ip(192,168,1,20);

// initialize the library instance:
EthernetClient client;

const int requestInterval = 30000;  // delay between requests

char serverName[] = "meteo.search.ch";  // server URL

boolean requested;                   // whether you've made a request since connecting
long lastAttemptTime = 0;            // last time you connected to the server, in milliseconds

char *dayName[4] = {"Now: ", "", "", ""};
char  buff[512];
char *line = NULL;                   // this line needs to be long, it won't fit to the static memory segment of this code, so we have to allocate it dynamically
unsigned int maxSize = 512;
char *newLine = NULL;
int   day=0;
char  info[4][40];                   // selected parsed strings to display
boolean doneReading = false;         // close the conection yet?

void setup() {
// initialize serial:
///  Serial.begin(57600);

// set up the LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degChar);
  
  line = (char*)malloc(maxSize);
  if( line == NULL ){
    lcd.setCursor(0, 0);
    lcd.print("dynamic alloc failed");
///     Serial.println("dynamic allocation failed");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("dynamic alloc OK");
///     Serial.println("dynamic allocation OK");
    line[0] = '\0';
  }
  info[0][0] = '\0';
  info[1][0] = '\0';
  info[2][0] = '\0';
  info[3][0] = '\0';

  // attempt a DHCP connection:
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
    Ethernet.begin(mac, ip);
    lcd.setCursor(0, 1);
    lcd.print("Failed to DHCP");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("DHCP ok");
  }

  connectToServer();
  
}

void loop()
{
  if( client.connected() ) {
    if( client.available() ) {
      // put incoming blocks into a string:

      if( newLine==NULL ){
        int nRead = client.read((uint8_t*)buff,511);
        if( nRead<512 ) buff[nRead] = '\0';
        else buff[511] = '\0';
        newLine = strchr(buff,'\n');
        if( newLine!=NULL ){
          *newLine = '\0';
          if( maxSize > strlen(line) + strlen(buff) )
            strcpy(line+strlen(line), buff);
          newLine++;
        } else {
          if( maxSize > strlen(line) + strlen(buff) )
            strcpy(line+strlen(line), buff);
        }
      } else {
        char *prevNewLine = newLine;
        newLine = strchr(newLine,'\n');
        if( newLine!=NULL ){
          *newLine = '\0';
          if( maxSize > strlen(line) + strlen(prevNewLine) )
            strcpy(line+strlen(line), prevNewLine);
          newLine++;
        } else {
          if( maxSize > strlen(line) + strlen(prevNewLine) )
            strcpy(line+strlen(line), prevNewLine);
        }
      }

      // return if we did not finish reading an '\n' terminated string
      if( newLine==NULL ) return;

        // parse the string

        // if the current line contains "meteo_cell" token,
        // it'll be followed by the day description:
        if( strstr(line,"meteo_cell")!=NULL && day<4 ){
          // today?
          if( strstr(line,"meteo_current")!=NULL ) day = 1;

          char *dayDescrBegins = strstr(line,"title=\"");
          int   dayDescrLen = 0;

          if( dayDescrBegins != NULL ){
            dayDescrBegins += 7;
            char *dayDescrEnds = strchr(dayDescrBegins,'"');
            if( dayDescrEnds != NULL )
              dayDescrLen = dayDescrEnds - dayDescrBegins;
            if( dayDescrLen>13 ) dayDescrLen = 13;
          }

          if( dayDescrLen ){
             unsigned int pos = strlen(info[day]);
             strncpy(info[day] + pos, dayDescrBegins, dayDescrLen);
             info[day][ dayDescrLen+pos   ] = ' ';
             info[day][ dayDescrLen+pos+1 ] = '\0';
          }

          char *dayTempColdBegins = strstr(line,"meteo_cold\">");
          int   dayTempColdLen = 0;

          if( dayTempColdBegins != NULL ){
            dayTempColdBegins += 12;
            char *dayTempColdEnds = strchr(dayTempColdBegins,'<');
            if( dayTempColdEnds != NULL )
              dayTempColdLen = dayTempColdEnds - dayTempColdBegins;
            if( dayTempColdLen>5 ) dayTempColdLen = 5;
          }

          if( dayTempColdLen ){
             unsigned int pos = strlen(info[day]);
             strncpy(info[day] + pos, dayTempColdBegins, dayTempColdLen-2);
             info[day][ dayTempColdLen-2+pos   ] = '/';
             info[day][ dayTempColdLen-2+pos+1 ] = '\0';
          }

          char *dayTempHotBegins = strstr(line,"meteo_hot\">");
          int   dayTempHotLen    = 0;

          if( dayTempHotBegins != NULL ){
            dayTempHotBegins += 11;
            char *dayTempHotEnds = strchr(dayTempHotBegins,'<');
            if( dayTempHotEnds != NULL )
              dayTempHotLen = dayTempHotEnds - dayTempHotBegins;
            if( dayTempHotLen>5 ) dayTempHotLen = 5;
          }

          if( dayTempHotLen ){
             unsigned int pos = strlen(info[day]);
             strncpy(info[day] + pos, dayTempHotBegins, dayTempHotLen-2);
             info[day][ dayTempHotLen-2+pos ] = '\0';
          }

          day++;
        }

        char *dayTempBegins = strstr(line,"ground\">");
        int   dayTempLen    = 0;

        if( dayTempBegins != NULL ){
          dayTempBegins += 8;
          char *dayTempEnds = strchr(dayTempBegins,'<');
          if( dayTempEnds != NULL )
            dayTempLen = dayTempEnds - dayTempBegins;
          if( dayTempLen>10 ) dayTempLen = 10;
        }

        if( dayTempLen ){
          strcpy (info[0], dayName[0]);
          strncpy(info[0] + strlen(dayName[0]), dayTempBegins, dayTempLen-4);
          info[day][ strlen(dayName[0])+dayTempLen-4 ] = '\0';
          doneReading = true;
        }


      // if you're currently reading the bytes,
      // add them to the info string:
      if( doneReading ){
          // you've reached the end of the quote:
          doneReading = false;
          day = 0;
///          Serial.println(info[0]);
///          Serial.println(info[1]); 
///          Serial.println(info[2]);
///          Serial.println(info[3]);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(info[0]);
          lcd.write(0);
          lcd.print("C");
          lcd.setCursor(0, 1);
          lcd.print(info[1]);
          lcd.write(0);
          lcd.print("C");
          lcd.setCursor(0, 2);
          lcd.print(info[2]);
          lcd.write(0);
          lcd.print("C");
          lcd.setCursor(0, 3);
          lcd.print(info[3]);
          lcd.write(0);
          lcd.print("C");

          // close the connection to the server:
          client.stop();
          client.flush();
          info[0][0] = '\0';
          info[1][0] = '\0';
          info[2][0] = '\0';
          info[3][0] = '\0';
      }


    line[0]='\0';

    }
  }
  else if (millis() - lastAttemptTime > requestInterval) {
    // if you're not connected, and two minutes have passed since
    // your last connection, then attempt to connect again:
    connectToServer();
  }

}

void connectToServer() {

  // attempt to connect, and wait a millisecond:
  lcd.setCursor(0, 2);
  lcd.print("connecting to server");
///  Serial.println("connecting to server...");
  if (client.connect(serverName, 80)) {

    lcd.setCursor(0, 3);
    lcd.print("making HTTP request");

///    Serial.println("making HTTP request...");
    client.println("GET /meyrin.en.html HTTP/1.1");
    client.println("HOST: meteo.search.ch");
    client.println();
  }
  // note the time of this connect attempt:
  lastAttemptTime = millis();
  newLine = NULL;
}
