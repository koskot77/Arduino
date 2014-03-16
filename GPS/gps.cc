// link between the computer and the SoftSerial Shield
//at 9600 bps 8-N-1
//Computer is connected to Hardware UART
//SoftSerial Shield is connected to the Software UART:D2&D3

#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x20,20,4);  // set the LCD address to 0x27 for a 20 chars and 4 line display
uint8_t degChar[8]  = {0x4,0xa,0x4,0x0,0x0,0x0,0x0};

//#include <LiquidCrystal.h>
//LiquidCrystal lcd(7, 6, 11, 10, 9, 8); // an option with non I2C display

SoftwareSerial SoftSerial(2, 3);

const int maxSize = 1024;
const char *marker = "$GPGGA";
char buffer [maxSize];
const char *curPos = NULL;    // position of the current payload (always points to ',' symbol or NULL)
unsigned short nPayload = -1; // index of the current payload
char payload[6][16];          // array of payloads (up to 6) up

void setup(){
  SoftSerial.begin(9600);     // the SoftSerial baud rate
  Serial.begin(115200);         // the Serial port of Arduino baud rate.
//  lcd.begin(16, 2);         // non I2C display
  lcd.init();                 // initialize the lcd 
  lcd.backlight();
  lcd.createChar(0, degChar);
  // empty the buffers and reset the pointers
  curPos = NULL;
  nPayload = -1;
  memset(buffer,0,maxSize);
}

void loop(){
  unsigned int count = 0;
  while( SoftSerial.available() && count<maxSize )
    buffer[count++] = SoftSerial.read();

  if( count>0 ){
    Serial.write((unsigned char*)buffer,count);

    // look for the control word
    if( curPos == NULL && (curPos = strstr(buffer,marker)) != NULL ) nPayload = 0;
    // nothing found?
    if( curPos == NULL && nPayload<0 ) return;

    // interpret the buffer while 'curPos' is not set to NULL
    while( curPos != NULL || (nPayload>=0 && nPayload<5) ){
      // look for a payload enclosed between ',' symbols
      const char *delimStart = strchr(curPos,',');
      const char *delimEnd   = ( delimStart != NULL ? strchr(delimStart+1,',') : NULL );
      // save the payload
      if( delimStart != NULL && delimEnd != NULL ){
        if( delimEnd-delimStart >= 16 ){
          Serial.print("Error, the sequence is too big to be payload: ");
          Serial.println(delimStart);
        } else {
          memcpy(payload[nPayload], delimStart+1,  delimEnd-delimStart );
          payload[nPayload][delimEnd-delimStart] = '\0';
          nPayload++;
          // if we read all we need, stop the search untill next control word is reached 
          if( nPayload == 5 ){
            curPos = NULL;
            break;
          }
        }
        // shift the current search position
        curPos = delimEnd;
      } else {
        // no or incomplete payload, move on to the following data block
      }
    }

    // if we read all we need, stop the search untill next control word is reached 
    if( nPayload == 5 ){
      curPos = NULL;
      char timeStr[9] = {payload[0][0],payload[0][1],':',payload[0][2],payload[0][3],':',payload[0][4],payload[0][5]};
      lcd.setCursor(0, 0);
      lcd.print("Time(GMT): ");
      lcd.print(timeStr);
      char latitude[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      latitude[0] = payload[1][0];
      latitude[1] = payload[1][1];
      latitude[2] = ' ';
      char *eol = strchr(payload[1],',');
      if( eol != NULL && eol!=payload[1] ){
        *eol = '\'';
        memcpy(latitude+3,payload[1]+2,13);
        lcd.setCursor(0, 1);
        lcd.print("Lat:  ");
        lcd.print(latitude);
        lcd.setCursor(8, 1);
        lcd.write(0);
        lcd.setCursor(19, 1);
        lcd.print(payload[2]);
      } else {
        lcd.setCursor(0, 1);
        lcd.print("Lat:  ");
        lcd.print(payload[1]);
      }
      char longitude[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      longitude[0] = payload[3][0];
      longitude[1] = payload[3][1];
      longitude[2] = payload[3][2];
      longitude[3] = ' ';
      eol = strchr(payload[3],',');
      if( eol != NULL && eol!=payload[3] ){
        *eol = '\'';
        memcpy(longitude+4,payload[3]+3,12);
        lcd.setCursor(0, 2);
        lcd.print("Long: ");
        lcd.print(longitude);
        lcd.setCursor(9, 2);
        lcd.write(0);
        lcd.setCursor(19, 2);
        lcd.print(payload[4]);
      } else {
        lcd.setCursor(0, 2);
        lcd.print("Long: ");
        lcd.print(payload[3]);
      }
    }
  }
  if (Serial.available()) SoftSerial.write(Serial.read());

}

