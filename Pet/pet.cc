#include "Arduino.h"
class Ultrasonic
{
	public:
		Ultrasonic(int pin);
        void DistanceMeasure(void);
		long microsecondsToCentimeters(void);
		long microsecondsToInches(void);
	private:
		int _pin;//pin number of Arduino that is connected with SIG pin of Ultrasonic Ranger.
        long duration;// the Pulse time received;
};
Ultrasonic::Ultrasonic(int pin)
{
	_pin = pin;
}
/*Begin the detection and get the pulse back signal*/
void Ultrasonic::DistanceMeasure(void)
{
    pinMode(_pin, OUTPUT);
	digitalWrite(_pin, LOW);
	delayMicroseconds(2);
	digitalWrite(_pin, HIGH);
	delayMicroseconds(5);
	digitalWrite(_pin,LOW);
	pinMode(_pin,INPUT);
	duration = pulseIn(_pin,HIGH);
}
/*The measured distance from the range 0 to 400 Centimeters*/
long Ultrasonic::microsecondsToCentimeters(void)
{
	return duration/29/2;	
}
/*The measured distance from the range 0 to 157 Inches*/
long Ultrasonic::microsecondsToInches(void)
{
	return duration/74/2;	
}

#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
int step = 10, maxDistDiff = 10; 
Ultrasonic ultrasonic(7);
int RangeInCentimeters[180];
int RangeInCentimeters1[180];
int RangeInCentimeters2[180];

void setup(){
  Serial.begin(9600);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  for(int pos=0; pos<180; pos++){
    RangeInCentimeters1[pos] = -1;
    RangeInCentimeters2[pos] = -1;
  }
}

void loop(){
  int motion = digitalRead(6);
  if( motion ){
    digitalWrite(4, HIGH);
    for(int pos = 0; pos < 180; pos += step){
      myservo.write(pos);
      delay(100);
      ultrasonic.DistanceMeasure();// get the current signal time;
      int range = ultrasonic.microsecondsToCentimeters();

      if( RangeInCentimeters1[pos]<0 ){
        RangeInCentimeters1[pos] = range;
        Serial.print(pos);
        Serial.print(" : ");
        Serial.println(range);
      } else {
        long delta = abs(RangeInCentimeters1[pos]-range);
        if( delta>maxDistDiff && RangeInCentimeters2[pos]<0 ){
          RangeInCentimeters2[pos] = range;
        }
      }
      if( RangeInCentimeters1[pos]>=0 && RangeInCentimeters2[pos]>=0 ){
        long delta1 = abs(RangeInCentimeters1[pos]-range);
        long delta2 = abs(RangeInCentimeters2[pos]-range);
        if( delta1>maxDistDiff && delta2>maxDistDiff ){
          Serial.print("The distance to obstacles at");
          Serial.print(pos);
          Serial.println(" in front is: ");
          Serial.print(range);//0~400cm
          Serial.print(" cm (was ");
          Serial.print(RangeInCentimeters1[pos]);
          Serial.println(" / ");
          Serial.print(RangeInCentimeters2[pos]);
          Serial.println(" )");
          RangeInCentimeters1[pos] = range;
          RangeInCentimeters2[pos] = -1;
          digitalWrite(5, HIGH);
          delay(30);
          digitalWrite(5, LOW);
        }
      }
    } 
  } else {
    digitalWrite(4, LOW);
  }
  delay(100);
}
