#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Wire.h>
#include "Barometer.h"
#include "DS1307.h"

Barometer myBarometer;
File myFile;
DS1307 clock;
short timePrevStore = -1;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x00, 0x27, 0x13, 0xB8, 0x04, 0xF6 };
///IPAddress ip(192,168,0,5);

const char *fileName = "test.txt";

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup()
{
///  Serial.begin(9600);
///  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
   pinMode(10, OUTPUT);

  if (!SD.begin(4)) {
///    Serial.println("initialization failed!");
    return;
  }
///  Serial.println("initialization done.");
  
  // start the Ethernet connection and the server:
  //  attempt a DHCP connection:
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
///    Ethernet.begin(mac, ip);
///    Serial.print("Failed to DHCP");
  } else {
///    Serial.print("DHCP ok");
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
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open(fileName, FILE_WRITE);

    // if the file opened okay, write to it:
    if( myFile ){
      // get the parameters
      float temperature = myBarometer.bmp085GetTemperature(myBarometer.bmp085ReadUT()); //Get the temperature, bmp085ReadUT MUST be called first
      long pressure = myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP());//Get the temperature
//      float altitude = myBarometer.calcAltitude(pressure); //Uncompensated caculation - in Meters 
      // and time
      char timeString[128], *pos = timeString;
//      pos += sprintf(pos,"%02d:",clock.hour);
//      pos += sprintf(pos,"%02d:",clock.minute);
//      pos += sprintf(pos,"%d 20%d.%02d.%02d",clock.second,clock.year,clock.month,clock.dayOfMonth);
      char nums[10] = {'0','1','2','3','4','5','6','7','8','9'};
      *pos++ = nums[clock.hour/10];
      *pos++ = nums[clock.hour%10];
      *pos++ = ':';
      *pos++ = nums[clock.minute/10];
      *pos++ = nums[clock.minute%10];
      *pos++ = ':';
      *pos++ = nums[clock.second/10];
      *pos++ = nums[clock.second%10];
      *pos++ = ' ';
      *pos++ = '2';
      *pos++ = '0';
      *pos++ = nums[clock.year/10];
      *pos++ = nums[clock.year%10];
      *pos++ = '.';
      *pos++ = nums[clock.month/10];
      *pos++ = nums[clock.month%10];
      *pos++ = '.';
      *pos++ = nums[clock.dayOfMonth/10];
      *pos++ = nums[clock.dayOfMonth%10];
      *pos++ = ' ';
      switch (clock.dayOfWeek){
        case MON: *pos++ = 'M'; *pos++ = 'O'; *pos++ = 'N'; break; //pos += sprintf(pos," MON"); 
        case TUE: *pos++ = 'T'; *pos++ = 'U'; *pos++ = 'E'; break; //pos += sprintf(pos," TUE"); 
        case WED: *pos++ = 'W'; *pos++ = 'E'; *pos++ = 'D'; break; //pos += sprintf(pos," WED"); 
        case THU: *pos++ = 'T'; *pos++ = 'H'; *pos++ = 'U'; break; //pos += sprintf(pos," THU"); 
        case FRI: *pos++ = 'F'; *pos++ = 'R'; *pos++ = 'I'; break; //pos += sprintf(pos," FRI"); 
        case SAT: *pos++ = 'S'; *pos++ = 'A'; *pos++ = 'T'; break; //pos += sprintf(pos," SAT"); 
        case SUN: *pos++ = 'S'; *pos++ = 'U'; *pos++ = 'N'; break; //pos += sprintf(pos," SUN"); 
        default: break;
     }
     *pos++ = '\0';
     myFile.print(timeString);

     myFile.print(", temp: ");
     myFile.print(temperature);
     myFile.print(" C");

     myFile.print(", pressure: ");
     myFile.print(pressure);
     myFile.println(" Pa");

//     myFile.print(", altitude: ");
//     myFile.println(altitude);
////     myFile.println(" m");

     // close the file:
     myFile.close();
    } else {
      // if the file didn't open, print an error:
///      Serial.print("error writing ");
///      Serial.println(fileName);
    }
  }

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
///    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' ){ ///&& currentLineIsBlank) {
          // send a standard http response header
///          client.println("HTTP/1.1 200 OK");
///          client.println("Content-Type: text/html");
///          client.println();

          // re-open the file for reading:
          myFile = SD.open(fileName);
          if( myFile ){
            // read from the file until there's nothing else in it:
            while (myFile.available()) {
              client.write(myFile.read());
            }
            // close the file:
            myFile.close();
          } else {
            // if the file didn't open, print an error:
///            Serial.print("error opening ");
///            Serial.println(fileName);
          }
          break;
        }
///        if( c == '\n' ){
///          // you're starting a new line
///          currentLineIsBlank = true;
///        } 
///        else if (c != '\r') {
///          // you've gotten a character on the current line
///          currentLineIsBlank = false;
///        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}
