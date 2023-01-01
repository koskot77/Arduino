#include <SPI.h>
#include <Ethernet.h>
//#include <SD.h>
#include <Wire.h>
//#include <avr/wdt.h>
#include "Barometer.h"
#include <DS1307.h>
#include <avr/pgmspace.h>
#include <Adafruit_I2CDevice.h>
#include "Adafruit_SHT31.h"

#include <LiquidCrystal.h>
/*
  The circuit:
 * LCD RS pin to digital pin 8
 * LCD Enable pin to digital pin 7
 * LCD D4 pin to digital pin 6
 * LCD D5 pin to digital pin 5
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
*/
LiquidCrystal lcd(8, 7, 6, 5, 3, 2);

Barometer myBarometer;
DS1307 clock;
Adafruit_SHT31 sht31;

short timePrevStore = -1, timePrevAve = -1;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x00, 0x27, 0x13, 0xB8, 0x04, 0xF6 };
//IPAddress ip(192,168,0,24); // force IP address

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

const int buttonPin = 4; // don't use pin #4 with the SD!
const int ledPin = -1;
bool      ledState = false;

// Render static part ot highcharts with the blocks below:
const PROGMEM char page_begin[] = ""
"<!doctype HTML>"           "\n"
"<meta charset = 'utf-8'>"  "\n"
"<html><head>"              "\n"
"<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>" "\n"
"<script src=\"https://code.highcharts.com/highcharts.js\"></script>"                        "\n"
"<script src=\"https://code.highcharts.com/highcharts-more.js\"></script>"                   "\n"
"<script src=\"https://code.highcharts.com/modules/xrange.js\"></script>"                    "\n"
"<script src=\"https://code.highcharts.com/modules/exporting.js\"></script>"                 "\n"
"<style>"             "\n"
".rChart {"           "\n"
"display: block;"     "\n"
"margin-left: auto;"  "\n"
"margin-right: auto;" "\n"
"width: 800px;"       "\n"
"height: 400px;"      "\n"
"}"                   "\n"
"</style>"            "\n"
"</head><body>"       "\n";

const PROGMEM char chart_begin[] = ""
"<div id='myChart' class='rChart highcharts'></div>" "\n"
"<script type='text/javascript'>"                    "\n"
"(function($){"                                      "\n"
"  $(function(){"                                    "\n"
"    var chart=new Highcharts.Chart({"               "\n"
"\"dom\": \"myChart\","                              "\n"
"\"width\": 800,"                                    "\n"
"\"height\":400,"                                    "\n"
"\"exporting\": {"                                   "\n"
" \"enabled\": false"                                "\n"
"},"                                                 "\n"
"\"title\":{\"text\":\"Pressure\"},"                 "\n"
"\"yAxis\": [{\"title\":{\"text\":\"Pa\"},\"min\":95000,\"max\":98000}],"        "\n"
"\"chart\": {\"renderTo\":\"myChart\"},"             "\n"
"\"series\": [{\"data\": ["                          "\n";


const PROGMEM char chart_end[] = ""
"], \"name\":\"Pressure\", \"type\":\"line\", \"marker\":{\"radius\":3}}" "\n"
"],"                        "\n"
"\"xAxis\": [ "             "\n"
"{"                         "\n"
" \"type\": 'datetime',"    "\n"
" \"title\": {"             "\n"
" \"text\": \"Time\""       "\n"
" }"                        "\n"
"}"                         "\n"
"],"                        "\n"
"\"subtitle\": {"           "\n"
" \"text\": null"           "\n"
"},"                        "\n"
"\"id\": \"myChart\""       "\n"
"});"                       "\n"
"});"                       "\n"
"})(jQuery);"               "\n"
"</script>"                 "\n";

const PROGMEM char page_end[] = "</body></html>";

//bool sd_ready = false;

float sum_pres = 0, count_pres = 0;

typedef struct
{
//  unsigned year : 6; // do I need this?
//  unsigned month: 4; // or this?
  unsigned day  : 5; // and this could also be reconstructed, but you'll need to remember how many days are in every month
  unsigned hour : 5;
  unsigned minute:6;
//  unsigned second:6; // or this?
//  unsigned int  raw_temp; // both, raw temperature
//  unsigned long raw_pres; //   and raw pressure have to be recorded to reconstruct normal pressure value
  unsigned int pres; // alternatively, save space storing normal pressure offset by 0xFFFF (as to fit it in 2 bytes) 
} Data; // here are 32 + sizeof(unsigned int) = 48 bits in this structure

Data *historical_data = NULL;
size_t max_size = 144; // 1440 minutes in a day, let's keep a point for every 10 minutes
bool lan_status = false;

void run_dhcp(void){
  Serial.print(F("DHCP:"));
  if (!Ethernet.begin(mac))
  {
    // if DHCP fails, start with a hard-coded address:
    Serial.println(F(" failed"));
//    Ethernet.begin(mac, ip);
    lan_status = false;
  } else {
    Serial.print(F(" success, IP:"));
    Serial.println(Ethernet.localIP());
//    Serial.print(" mask: ");
//    Serial.println(Ethernet.subnetMask());
    lan_status = true;
  }
  server.begin();
  lcd.begin(16, 2);
}

void setup()
{
  Serial.begin(9600);
//  wdt_enable(WDTO_1S);
  delay(1);
/* If you uncomment the code below the binary size exceeds max and wraps around destroying bootloader
  Serial.print(F("Initializing SD card..."));
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
   pinMode(10, OUTPUT);

  if(!SD.begin(4))
  {
    Serial.println(F(" initialization of CD card failed, application will not be logging historical data"));
    sd_ready = false;
  } else {
    Serial.println(F(" initialization of CD card successful"));
    sd_ready = true;
  }
  Serial.println("");
*/
  // start the Ethernet connection and the server:
  //  attempt a DHCP connection:
  run_dhcp();

  Serial.print(F("dynamic allocation:"));
  historical_data = (Data*)calloc(max_size,sizeof(Data));
  if( historical_data == NULL )
  {
    Serial.println(F(" failed"));
  } else {
    Serial.println(F(" success"));
  }

  clock.begin();
  myBarometer.init();

  if( !sht31.begin(0x44) )
  {   // Set to 0x45 for alternate i2c addr
    Serial.println(F("Couldn't find SHT31"));
    delay(1);
  }
  
  // initialize the LED pin as an output:
  if( ledPin>0 ) pinMode(ledPin, OUTPUT);      
  // initialize the pushbutton pin as an input:
  if( buttonPin>0 ) pinMode(buttonPin, INPUT_PULLUP);     
}

const char nums[10] = {'0','1','2','3','4','5','6','7','8','9'};

// buggy compiler
//const char* render_point(const Data &x){
char* render_point(const void *y, unsigned int year, unsigned int month, unsigned int second = 0)
{
  Data &x = *(Data*)y;

  static char point[41] = {'[','D','a','t','e','.','U','T','C','(','2','0',0,0,',',0,0,',',0,0,',',0,0,',',0,0,',',0,0,')',',',0,0,0,0,0,',','1',']','\0'};
  char *pos = point + 12; // skip "[Date.UTC(20"

  *pos++ = nums[  year/10];
  *pos++ = nums[  year%10];
  ++pos;
  *pos++ = nums[(month-1)/10];
  *pos++ = nums[(month-1)%10];
  ++pos;
  *pos++ = nums[  x.day/10];
  *pos++ = nums[  x.day%10];
  ++pos;
  *pos++ = nums[x.hour/10];
  *pos++ = nums[x.hour%10];
  ++pos;
  *pos++ = nums[x.minute/10];
  *pos++ = nums[x.minute%10];
  ++pos;
  *pos++ = nums[  second/10];
  *pos++ = nums[  second%10];
  pos += 6;
  unsigned long pressure = (unsigned long)x.pres + (unsigned long)0xFFFF;
  for( ; pressure > 0; pressure /= 10) *pos-- = nums[pressure % 10];
// the above is equivalent of:
//  point[31] = nums[(pressure / 10000)%10];
//  point[32] = nums[(pressure /  1000)%10];
//  point[33] = nums[(pressure /   100)%10];
//  point[34] = nums[(pressure /    10)%10];
//  point[35] = nums[(pressure        )%10];

  return point;
}

float temp_box = 0, temp_sht31_norm = 0, humid_sht31_norm = 0, temp_sht31_heat = 0, humid_sht31_heat = 0;
bool sht31_heater_enabled = false;
unsigned int  raw_temp;
unsigned long raw_pres;
Data tmp;
unsigned short year;
unsigned char  month;
const unsigned short refresh_minutes = 10;

void loop()
{
//  wdt_reset(); // not using watchdog

  // read the state of the pushbutton value and reobtain IP address if pressed
  if( buttonPin>0 && digitalRead(buttonPin) == LOW )
  {
    if( ledPin>0 ) digitalWrite(ledPin, HIGH);    
    run_dhcp();
  }

  // every second read sensors and toggle heartbeat LED
  clock.getTime();
  if( timePrevAve<0 || abs(clock.second-timePrevAve)>=1 )
  {
    timePrevAve = clock.second;
                  raw_temp = myBarometer.bmp085ReadUT(); // let's hope temperature doesn't change too much over minute
                  raw_pres = myBarometer.bmp085ReadUP();
    temp_box  = myBarometer.bmp085GetTemperature(raw_temp); // Barometer's driver is not stateless (!), you have to make this call even you don't need to use its result
    sum_pres += myBarometer.bmp085GetPressure(raw_pres); // calculate a sum so as to average it for reporting later
//  float altitude = myBarometer.calcAltitude(pressure); //Uncompensated caculation - in Meters, who needs this anyways?
    count_pres += 1; // normalization factor for averaging

    if( sht31_heater_enabled )
    {
      temp_sht31_heat  = sht31.readTemperature();
      humid_sht31_heat = sht31.readHumidity();
    } else {
      // give it some time to cool down from last warm-up cycle (upon first cycle it is automatically true)
      if( abs(clock.minute-timePrevStore) > 1 )
      {
        temp_sht31_norm  = sht31.readTemperature();
        humid_sht31_norm = sht31.readHumidity();
      }
    }

    // hartbeat LED:
    if( ledState )
    {
      // turn LED off:
      if( ledPin>0 ) digitalWrite(ledPin, LOW); 
      ledState = false;
    } else {
      // turn LED on:
      if( ledPin>0 ) digitalWrite(ledPin, HIGH);  
      ledState = true;    
    }

    // display readings
    lcd.setCursor(0, 0);
//    tmp.year   = clock.year;
//    tmp.month  = clock.month;
    tmp.day    = clock.dayOfMonth;
    tmp.hour   = clock.hour;
    tmp.minute = clock.minute;
//    tmp.second = clock.second;
    tmp.pres   = myBarometer.bmp085GetPressure(raw_pres) - 0xFFFF;
    char *buf = render_point(&tmp, clock.year, clock.month, clock.second);
    buf[23] = ':';
    buf[26] = ':';
    buf[29] = '\0';
    lcd.print(buf+21);
    // restore static data
    buf[23] = ',';
    buf[26] = ',';
    buf[29] = ')';

    lcd.setCursor(15, 0);
    switch (clock.dayOfWeek)// Friendly printout of the weekday
    {
        case MON: lcd.print(F("M")); break;
        case TUE: lcd.print(F("T")); break;
        case WED: lcd.print(F("W")); break;
        case THU: lcd.print(F("T")); break;
        case FRI: lcd.print(F("F")); break;
        case SAT: lcd.print(F("S")); break;
        case SUN: lcd.print(F("S")); break;
        default:  lcd.print(F("?")); break;
    }
    lcd.setCursor(9, 0);
    lcd.print("  ");
    lcd.setCursor(9, 0);
    lcd.print(clock.dayOfMonth);
    lcd.setCursor(11, 0);
    const char* mo[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    lcd.print(mo[clock.month]);

    lcd.setCursor(0, 1);
    lcd.print(int(temp_sht31_norm + 0.5)); // rounding
    lcd.setCursor(2, 1);
    lcd.print(char(223));
    lcd.setCursor(3, 1);
    lcd.print(F("C"));
    lcd.setCursor(6, 1);
//    buf[26] = '\0';
//    lcd.print(buf+31);
    lcd.print(int(((unsigned long)tmp.pres + 0xFFFF)/133.322 + 0.5)); // rounding
    lcd.setCursor(9, 1);
    lcd.print(F("mm"));
    lcd.setCursor(13, 1);
    lcd.print(int(humid_sht31_norm + 0.5)); // rounding
    lcd.setCursor(15, 1);
    lcd.print(F("%"));
  }

  // store results every 10 minute or so
  if( timePrevStore<0 || abs(clock.minute-timePrevStore) >= refresh_minutes )
  {
    timePrevStore = clock.minute;

    memmove((void*)(historical_data+1), (void*)historical_data, (max_size-1)*sizeof(Data));

    Data &now = historical_data[0];

//    now.year   = clock.year;
//    now.month  = clock.month;
    now.day    = clock.dayOfMonth;
    now.hour   = clock.hour;
    now.minute = clock.minute;
//    now.second = clock.second;
    now.pres   = (unsigned int)(sum_pres/count_pres - 0xFFFF);

    sum_pres   = 0;
    count_pres = 0;
    // if logs are possible
//    if( sd_ready ) sd_write(now.pres);
  }

  // toggle the heater on SHT31 for a minute every 10 minutes
  if( abs(clock.minute-timePrevStore) >= 9 && !sht31.isHeaterEnabled() )
  {
    sht31_heater_enabled = true;
    sht31.heater(sht31_heater_enabled);
  }
  if( abs(clock.minute-timePrevStore) <  9 && sht31.isHeaterEnabled() )
  {
    sht31_heater_enabled = false;
    sht31.heater(sht31_heater_enabled);
  }

  // maybe duplicating the 'client' check below
  if( !lan_status ) return;

  // listen for incoming clients
  EthernetClient client = server.available();
  if(client)
  {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    // the request line
    char request[96], *pos = request;
    request[95] = '\0';
    char path[20] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        // store the characters in a line
        if( pos-request<95 ) *pos++ = c;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply

        if (c == '\n' && currentLineIsBlank)
        {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println();
          // read back constants from flash memory

          for(int k = 0; k < strlen_P(page_begin); k++)
          {
            char ch = pgm_read_byte_near(page_begin + k);
            client.print(ch);
          }
          if( path[0]=='\0' )
          {
            // render highcharts page
            for(int k = 0; k < strlen_P(chart_begin); ++k)
            {
              char ch = pgm_read_byte_near(chart_begin + k);
              client.print(ch);
            }

            for(int p=max_size-1; p>=0; --p)
            {
              // no earlier data?
              if( historical_data[p].pres == 0 ) continue;
              // todays year and month
              year  = clock.year;
              month = clock.month;
              // previous month for the current point?
              if( historical_data[p].day > clock.dayOfMonth )
              {
                  // previous year for the current point?
                  if( clock.month == 1 )
                  {
                      year  = clock.year - 1;
                      month = 12;
                  } else {
                      month = clock.month - 1;
                  }
              }
              const char *point = render_point(&historical_data[p], year, month);
              client.print(point);
              if(p!=0) client.print(",\n");
            }

            for(int k = 0; k < strlen_P(chart_end); ++k)
            {
              char ch = pgm_read_byte_near(chart_end + k);
              client.print(ch);
            }
            // list files
//            if( log_to_sd ) sd_list(client);

          } else {
//            if( log_to_sd ) sd_read(client);
              if( strstr(path, "time") != NULL ){
                  int year  = clock.year;
                  int month = clock.month;
                  int day   = clock.dayOfMonth;
                  int weekday = clock.dayOfWeek;
                  clock.fillByYMD(year,month,day);
                  clock.fillDayOfWeek(weekday);
                  int hour   = (*(path+5)-48)*10 + *(path+6)-48;
                  int minute = (*(path+8)-48)*10 + *(path+9)-48;
                  int second = (*(path+11)-48)*10 + *(path+12)-48;
                  clock.fillByHMS(hour,minute,second);
                  if( hour>=0 && hour<24 && minute>=0 && minute<60 && second>=0 && second<60)
                      clock.setTime(); //write time to the RTC chip
              } else
              if( strstr(path, "date") != NULL ){
                  int year  = (*(path+5)-48)*1000 + (*(path+6)-48)*100 + (*(path+7)-48)*10 + *(path+8)-48;
                  int month = (*(path+10)-48)*10 + (*(path+11)-48);
                  int day   = (*(path+13)-48)*10 + (*(path+14)-48);
                  int weekday = *(path+16)-48;
                  clock.fillByYMD(year,month,day);
                  clock.fillDayOfWeek(weekday);
                  int hour   = clock.hour;
                  int minute = clock.minute;
                  int second = clock.second;
                  clock.fillByHMS(hour,minute,second);
                  if( year>1977 && year<2100 && month>0 && month<=12 && day>0 && day<32 && weekday<8 )
                      clock.setTime(); //write time to the RTC chip
              }
          }

          client.print(F("<br>Temperature SHT31: "));
          client.print(temp_sht31_norm);
          client.print(F(" C (heated = "));
          client.print(temp_sht31_heat);
          client.print(F(" C)\n"));

          client.print(F("<br>Humidity: "));
          client.print(humid_sht31_norm);
          client.print(F(" % (heated = "));
          client.print(humid_sht31_heat);
          client.print(F(" %)\n"));

          client.print(F("<br>SHT31 heater is currently: "));
          if( sht31.isHeaterEnabled() )
            client.println(F("enabled\n<br>\n"));
          else {
            if( abs(clock.minute-timePrevStore) < 1 )
              client.println(F("cooling down\n<br>\n"));
            else
              client.println(F("disabled\n<br>\n"));
          }

          client.print(F("<br>(Temperature inside the box: "));
          client.print(temp_box);
          client.print(F(" C)\n"));

          client.print(F("<br>Pressure: "));
          client.print(int(((unsigned long)tmp.pres + 0xFFFF)/133.322 + 0.5)); // rounding
          client.print(F(" mmHg\n"));

          for(int k = 0; k < strlen_P(page_end); ++k)
          {
            char ch = pgm_read_byte_near(page_end + k);
            client.print(ch);
          }
          break;
        }
        if( c == '\n' )
        {
          *pos = '\0';
          if( strstr(request,"GET") != NULL && (pos-request)>4 )
          {
            char *start = strchr(request+3,'/');
            char *end   = strchr(request+4,' ');
            if( end==NULL ) end = pos-2; // actually, in msdos/vfat system the newline is coded with 2 symbols
            if( start != NULL && end != NULL && end-start<20 )
            {
              start++; // skip the first '/' symbol
              memcpy(path,start,end-start);
              path[end-start] = '\0';
            } else path[0] = '\0';
          }
          // you're starting a new line
          pos = request;
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }

}



/*
// The function below is introduced to spare some space otherwise taken by sprintf/stdlib
// pos += sprintf(pos,"%02d:",clock.hour);
// pos += sprintf(pos,"%02d:",clock.minute);
// pos += sprintf(pos,"%d 20%d.%02d.%02d",clock.second,clock.year,clock.month,clock.dayOfMonth);
void render_time_date(const DS1307& clock, char (&timeString)[16], char (&dateString)[16]){
    // compose time and date
    char *tPos = timeString, *dPos = dateString;

    *tPos++ = nums[clock.hour/10];
    *tPos++ = nums[clock.hour%10];
    *tPos++ = ':';
    *tPos++ = nums[clock.minute/10];
    *tPos++ = nums[clock.minute%10];
    *tPos++ = ':';
    *tPos++ = nums[clock.second/10];
    *tPos++ = nums[clock.second%10];

    *dPos++ = '2';
    *dPos++ = '0';
    *dPos++ = nums[clock.year/10];
    *dPos++ = nums[clock.year%10];
//    *dPos++ = '.';
    *dPos++ = nums[clock.month/10];
    *dPos++ = nums[clock.month%10];
//    *dPos++ = '.';
    *dPos++ = nums[clock.dayOfMonth/10];
    *dPos++ = nums[clock.dayOfMonth%10];
//    *dPos++ = '.';
//    *dPos++ = 'p';
//    *dPos++ = 'r';
//    *dPos++ = 's';
//    switch (clock.dayOfWeek){
//      case MON: *pos++ = 'M'; *pos++ = 'O'; *pos++ = 'N'; break; //pos += sprintf(pos," MON"); 
//      case TUE: *pos++ = 'T'; *pos++ = 'U'; *pos++ = 'E'; break; //pos += sprintf(pos," TUE"); 
//      case WED: *pos++ = 'W'; *pos++ = 'E'; *pos++ = 'D'; break; //pos += sprintf(pos," WED"); 
//      case THU: *pos++ = 'T'; *pos++ = 'H'; *pos++ = 'U'; break; //pos += sprintf(pos," THU"); 
//      case FRI: *pos++ = 'F'; *pos++ = 'R'; *pos++ = 'I'; break; //pos += sprintf(pos," FRI"); 
//      case SAT: *pos++ = 'S'; *pos++ = 'A'; *pos++ = 'T'; break; //pos += sprintf(pos," SAT"); 
//      case SUN: *pos++ = 'S'; *pos++ = 'U'; *pos++ = 'N'; break; //pos += sprintf(pos," SUN"); 
//      default: break;
//   }
   *tPos++ = '\0';
   *dPos++ = '\0';
}

void sd_write(long  pressure){
      char timeString[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      char dateString[16] = {0,0,0,0,0,0,0,0,'.','p','r','s',0,0,0,0};
      render_time_date(clock, timeString, dateString);

      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      File myFile = SD.open(dateString, FILE_WRITE);
      // if the file opened okay, write to it:
      if( myFile != NULL ){
        myFile.print(timeString);
//        myFile.print(", temp: ");
//        myFile.print(temperature);
//        myFile.print(" C");
        myFile.print(", p= ");
        myFile.print(pressure);
        myFile.println(" Pa");
//        myFile.print(", altitude: ");
//        myFile.println(altitude);
////        myFile.println(" m");
     // close the file:
        myFile.close();
      } else {
        // if the file didn't open, print an error:
        Serial.print("error writing ");
        Serial.println(dateString);
      }
}

void sd_list(Stream &client){
  File dir = SD.open("/");
  dir.rewindDirectory();
  while ( true ){
    File entry =  dir.openNextFile();
    if( entry ){
      client.print("<a href=\"");
      client.print(entry.name());
      client.print("\">");
      client.print(entry.name());
      client.print("</a>");
      if( entry.isDirectory() ) {
        client.println("/");
      } else {
       // files have sizes, directories do not
       client.print("\t\t");
       client.println(entry.size(), DEC);
      }
      entry.close();
    } else {
      break;
    }
  }
  dir.close();
}

void sd_read(Stream &client, const char *path){
  // open the file for reading:
  File myFile;
  if( (myFile = SD.open(path)) ){
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      client.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    client.println("HTTP/1.1 404 Resource not found");
    Serial.print("error opening ");
    Serial.println(path);
  }
}

*/
