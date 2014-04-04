#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Wire.h>
#include "Barometer.h"
#include "DS1307.h"

Barometer myBarometer;
DS1307 clock;
short timePrevStore = -1;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x00, 0x27, 0x13, 0xB8, 0x04, 0xF6 };
///IPAddress ip(192,168,0,24); // force IP address

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup()
{
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
   pinMode(10, OUTPUT);

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  // start the Ethernet connection and the server:
  //  attempt a DHCP connection:
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
///    Ethernet.begin(mac, ip);
    Serial.println("Failed to DHCP");
  } else {
    Serial.println("DHCP ok");
  }

  server.begin();
  clock.begin();
  myBarometer.init();
}

void loop()
{
  clock.getTime();
  if( timePrevStore<0 || abs(clock.minute-timePrevStore)>=1 ){ // store results every 1 minute or so
    timePrevStore = clock.minute;

    // compose time and date
    char timeString[16], dateString[16], *tPos = timeString, *dPos = dateString;
//    pos += sprintf(pos,"%02d:",clock.hour);
//    pos += sprintf(pos,"%02d:",clock.minute);
//    pos += sprintf(pos,"%d 20%d.%02d.%02d",clock.second,clock.year,clock.month,clock.dayOfMonth);
    char nums[10] = {'0','1','2','3','4','5','6','7','8','9'};
    *tPos++ = nums[clock.hour/10];
    *tPos++ = nums[clock.hour%10];
    *tPos++ = ':';
    *tPos++ = nums[clock.minute/10];
    *tPos++ = nums[clock.minute%10];
    *tPos++ = ':';
    *tPos++ = nums[clock.second/10];
    *tPos++ = nums[clock.second%10];
//    *pos++ = ' ';
    *dPos++ = '2';
    *dPos++ = '0';
    *dPos++ = nums[clock.year/10];
    *dPos++ = nums[clock.year%10];
//    *pos++ = '.';
    *dPos++ = nums[clock.month/10];
    *dPos++ = nums[clock.month%10];
//    *pos++ = '.';
    *dPos++ = nums[clock.dayOfMonth/10];
    *dPos++ = nums[clock.dayOfMonth%10];
//    *pos++ = ' ';
    *dPos++ = '.';
    *dPos++ = 'p';
    *dPos++ = 'r';
    *dPos++ = 's';
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
//   *pos++ = '\0';
   *tPos++ = '\0';
   *dPos++ = '\0';

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File myFile = SD.open(dateString, FILE_WRITE);

    // if the file opened okay, write to it:
    if( myFile ){
      // get the parameters
      float temperature = myBarometer.bmp085GetTemperature( myBarometer.bmp085ReadUT() ); //Get the temperature, bmp085ReadUT MUST be called first
      long  pressure    = myBarometer.bmp085GetPressure( myBarometer.bmp085ReadUP() );//Get the temperature
//      float altitude = myBarometer.calcAltitude(pressure); //Uncompensated caculation - in Meters 

     myFile.print(timeString);

//     myFile.print(", temp: ");
//     myFile.print(temperature);
//     myFile.print(" C");

     myFile.print(", p= ");
     myFile.print(pressure);
     myFile.println(" Pa");

//     myFile.print(", altitude: ");
//     myFile.println(altitude);
////     myFile.println(" m");

     // close the file:
     myFile.close();
    } else {
      // if the file didn't open, print an error:
      Serial.print("error writing ");
      Serial.println(dateString);
    }

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
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<html><head></head><body bgcolor=\"#8CCEF7\"><pre>");

          if( path[0]=='\0' ){
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
          } else {
            // re-open the file for reading:
            File myFile = SD.open(path);
            if( myFile ){
              // read from the file until there's nothing else in it:
              while (myFile.available()) {
                client.write(myFile.read());
              }
              // close the file:
              myFile.close();
            } else {
            // if the file didn't open, print an error:
              Serial.print("error opening ");
              Serial.println(path);
            }
          }

          client.println("</pre></body></html>");
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
