#include <SPI.h>
#include <Ethernet.h>
//#include <SD.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "Barometer.h"
#include <DS1307.h>
#include <avr/pgmspace.h>

Barometer myBarometer;
DS1307 clock;
short timePrevStore = -1, timePrevAve = -1;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x00, 0x27, 0x13, 0xB8, 0x04, 0xF6 };
IPAddress ip(192,168,0,24); // force IP address

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

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
"\"yAxis\": [{\"title\":{\"text\":\"Pa\"}}],"        "\n"
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

bool log_to_sd = true;

float sum_temp = 0, sum_pres = 0, norm = 0;

typedef struct {
  unsigned year : 6;
  unsigned month: 4;
  unsigned day  : 5;
  unsigned hour : 5;
  unsigned minute:6;
  unsigned second:6;
  unsigned int temp;
  unsigned int pres;
} Data; // here are 32 + 2*sizeof(int) bits in this structure

Data *historical_data = NULL;
size_t max_size = 100;

void setup()
{
  Serial.begin(9600);
//  wdt_enable(WDTO_1S);
  delay(1);
/*  Serial.print(F("Initializing SD card..."));
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
   pinMode(10, OUTPUT);

  if (!SD.begin(4)) {
    Serial.println(F(" initialization of CD card failed, application will not be logging historical data"));
    log_to_sd = false;
  } else {
    Serial.println(F(" initialization of CD card successful"));
  }
  Serial.println("");
 */ // start the Ethernet connection and the server:
  //  attempt a DHCP connection:

  Serial.print(F("DHCP:"));
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
    Serial.println(F(" failed"));
//    Ethernet.begin(mac, ip);
  } else {
    Serial.print(F(" success, "));
    Serial.print(F(" IP:"));
    Serial.print(Ethernet.localIP());
//    Serial.print(" mask: ");
//    Serial.println(Ethernet.subnetMask());
  }
  Serial.println("");
  server.begin();

  Serial.print(F("dynamic allocation:"));
  historical_data = (Data*)calloc(max_size,sizeof(Data));
  if( historical_data == NULL ){
    Serial.print(F(" failed"));
  } else {
    Serial.print(F(" success"));
  }
  Serial.println("");

  clock.begin();
  myBarometer.init();
}

const char nums[10] = {'0','1','2','3','4','5','6','7','8','9'};
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
*/
// buggy compiler
//const char* render_point(const Data &x){
const char* render_point(const void *y){
  Data &x = *(Data*)y;

  static char point[41] = {'[','D','a','t','e','.','U','T','C','(','2','0',0,0,',',0,0,',',0,0,',',0,0,',',0,0,',',0,0,')',',',0,0,0,0,0,',','1',']','\0'};
  char *pos = point + 12; // skip "[Date.UTC(20"

  *pos++ = nums[x.year/10];
  *pos++ = nums[x.year%10];
  ++pos;
  *pos++ = nums[(x.month-1)/10];
  *pos++ = nums[(x.month-1)%10];
  ++pos;
  *pos++ = nums[x.day/10];
  *pos++ = nums[x.day%10];
  ++pos;
  *pos++ = nums[x.hour/10];
  *pos++ = nums[x.hour%10];
  ++pos;
  *pos++ = nums[x.minute/10];
  *pos++ = nums[x.minute%10];
  ++pos;
  *pos++ = nums[x.second/10];
  *pos++ = nums[x.second%10];
  ++pos;

  float temperature = myBarometer.bmp085GetTemperature( x.temp );
  long  pressure    = myBarometer.bmp085GetPressure   ( x.pres );
//  float altitude = myBarometer.calcAltitude(pressure); //Uncompensated caculation - in Meters 
  point[31] = nums[(pressure / 10000)%10];
  point[32] = nums[(pressure /  1000)%10];
  point[33] = nums[(pressure /   100)%10];
  point[34] = nums[(pressure /    10)%10];
  point[35] = nums[(pressure        )%10];

  return point;
}


void loop()
{
//  wdt_reset();

  clock.getTime();
  if( timePrevAve<0 || abs(clock.second-timePrevAve)>=1 ){
    timePrevAve = clock.second;
    sum_temp += myBarometer.bmp085ReadUT();
    sum_pres += myBarometer.bmp085ReadUP();
    norm     += 1;
  }

  // store results every 1 minute or so
  if( timePrevStore<0 || abs(clock.minute-timePrevStore)>=1 ){
    timePrevStore = clock.minute;

    memmove((void*)(historical_data+1), (void*)historical_data, (max_size-1)*sizeof(Data));

    Data &now = historical_data[0];

    now.year  = clock.year;
    now.month = clock.month;
    now.day   = clock.dayOfMonth;
    now.hour  = clock.hour;
    now.minute= clock.minute;
    now.second= clock.second;
    now.temp  = int(sum_temp/norm);
    now.pres  = int(sum_pres/norm);

    sum_temp = 0;
    sum_pres = 0;
    norm     = 0;
    // if logs are possible
//    if( log_to_sd ){
//      char timeString[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//      char dateString[16] = {0,0,0,0,0,0,0,0,'.','p','r','s',0,0,0,0};
//      render_time_date(clock, timeString, dateString);
//
//      // open the file. note that only one file can be open at a time,
//      // so you have to close this one before opening another.
//      File myFile = SD.open(dateString, FILE_WRITE);
//      // if the file opened okay, write to it:
//      if( myFile != NULL ){
//        myFile.print(timeString);
////        myFile.print(", temp: ");
////        myFile.print(temperature);
////        myFile.print(" C");
//        myFile.print(", p= ");
//        long  pressure = myBarometer.bmp085GetPressure(now.pres);        
//        myFile.print(pressure);
//        myFile.println(" Pa");
////        myFile.print(", altitude: ");
////        myFile.println(altitude);
//////        myFile.println(" m");
//     // close the file:
//        myFile.close();
//      } else {
//        // if the file didn't open, print an error:
//        Serial.print("error writing ");
//        Serial.println(dateString);
//      }
//    }
  }


  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    // the request line
    char request[128], *pos = request;
    request[127] = '\0';
    char path[16] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' ,'\0', '\0', '0'};
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // store the characters in a line
        if( pos-request<127 ) *pos++ = c;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply

        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println();
          // read back constants from flash memory

          for(int k = 0; k < strlen_P(page_begin); k++) {
            char ch = pgm_read_byte_near(page_begin + k);
            client.print(ch);
          }
          if( path[0]=='\0' ){
            // render highcharts page
            for(int k = 0; k < strlen_P(chart_begin); ++k) {
              char ch = pgm_read_byte_near(chart_begin + k);
              client.print(ch);
            }

            for(int p=max_size-1; p>=0; --p){
              if( historical_data[p].pres == 0 ) continue;
              const char *point = render_point(&historical_data[p]);
              client.print(point);
              if(p!=0) client.print(",\n");
            }

            for(int k = 0; k < strlen_P(chart_end); ++k) {
              char ch = pgm_read_byte_near(chart_end + k);
              client.print(ch);
            }

//            // list files
//            if( log_to_sd ){
//              File dir = SD.open("/");
//              dir.rewindDirectory();
//              while ( true ){
//                File entry =  dir.openNextFile();
//                if( entry ){
//                  client.print("<a href=\"");
//                  client.print(entry.name());
//                  client.print("\">");
//                  client.print(entry.name());
//                  client.print("</a>");
//                  if( entry.isDirectory() ) {
//                    client.println("/");
//                  } else {
//                    // files have sizes, directories do not
//                    client.print("\t\t");
//                    client.println(entry.size(), DEC);
//                  }
//                  entry.close();
//                } else {
//                  break;
//                }
//              }
//              dir.close();
//            }

          } else {
//            // open the file for reading:
//            File myFile;
//            if( log_to_sd && (myFile = SD.open(path)) ){
//              // read from the file until there's nothing else in it:
//              while (myFile.available()) {
//                client.write(myFile.read());
//              }
//              // close the file:
//              myFile.close();
//            } else {
//              // if the file didn't open, print an error:
//              client.println("HTTP/1.1 404 Resource not found");
//              Serial.print("error opening ");
//              Serial.println(path);
//            }
          }

          for(int k = 0; k < strlen_P(page_end); ++k) {
            char ch = pgm_read_byte_near(page_end + k);
            client.print(ch);
          }
          break;
        }
        if( c == '\n' ){
          *pos = '\0';
          if( strstr(request,"GET") != NULL && (pos-request)>4 ){
            char *start = strchr(request+3,'/');
            char *end   = strchr(request+4,' ');
            if( end==NULL ) end = pos-2; // actually, in msdos/vfat system the newline is coded with 2 symbols
            if( start != NULL && end != NULL && end-start<16 ){
              start++; // skip the first '/' symbol
              memcpy(path,start,end-start);
              path[end-start] = '\0';
            } else path[0] = '\0';
          }
          // you're starting a new line
          pos = request;
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
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

