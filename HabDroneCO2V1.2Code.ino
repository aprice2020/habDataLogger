// High Altitude Balloon and Drone Data Logger
// This logger collects and stores GPS and CO2 sensor data
// Storage on microSD card
// Written By Aaron Price
// Version 1.2
// October 22 2017
// Based off Full Example by Mikal Hart
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
//SoftwareSerial K_30_Serial(0,1);

static const int RXPin = 7, TXPin = 6;
static const uint32_t GPSBaud = 9600;
const int buttonPin = 8;
const int led = 9;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

File myFile;

byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};  //Command packet to read Co2 (see app note)
byte response[] = {0,0,0,0,0,0,0};  //create an array to store the response

//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;
int buttonState = 0;
bool drone = true;
int i = 1;


void setup()
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
  pinMode(led, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  if (!SD.begin(10)) {
    digitalWrite(led, HIGH); // if SD is not connected, turn on led
    return;
  }
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) { //if button pressed set HAB mode
    digitalWrite(led, HIGH);
    delay(200);
    digitalWrite(led, LOW);
    delay(200);
    digitalWrite(led, HIGH);
    delay(200);
    digitalWrite(led, LOW);
    drone = false;
  }
  myFile = SD.open("test.txt", FILE_WRITE);
  if (myFile) {
  myFile.println();
  myFile.println("initialization done.");
  myFile.println(F("High Altitude Balloon and Drone CO2 Logger"));
  myFile.println(F("Version 1.2"));
  myFile.println(F("Based off FullExample Tiny++ GPS by Mikal Hart"));
  myFile.println(F("October 22, 2017")); 
  myFile.println(F("By Aaron Price"));
  myFile.println();
  myFile.println(F("Sats HDOP Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card    Chars Sentences Checksum CO2 PPM"));
  myFile.println(F("          (deg)      (deg)       Age                      Age  (m)    --- from GPS ----    RX    RX        Fail"));
  if (drone == false) {
    
    myFile.println("HAB Mode");
    } else {
      
      myFile.println("Drone Mode");
      }
  myFile.println(F("---------------------------------------------------------------------------------------------------------------------------------------"));
  sendRequest(readCO2); //58 milliseconds
  unsigned long valCO2 = getValue(response);
  myFile.print("Co2 ppm = ");
  myFile.println(valCO2);
  
      
  myFile.close();
  }
}

void loop()
{
  myFile = SD.open("test.txt", FILE_WRITE);
  
  if (myFile) {
  if (drone == true || i == 30) {
  digitalWrite(led, HIGH);
  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
  myFile.print(",");
  printInt(gps.hdop.value(), gps.hdop.isValid(), 5);
  myFile.print(",");
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  myFile.print(",");
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  myFile.print(",");
  printInt(gps.location.age(), gps.location.isValid(), 5);
  myFile.print(",");
  printDateTime(gps.date, gps.time);
  myFile.print(",");
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
  myFile.print(",");
  printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
  myFile.print(",");
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
  myFile.print(",");
  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "*** ", 6);
  myFile.print(",");
  printInt(gps.charsProcessed(), true, 6);
  myFile.print(",");
  printInt(gps.sentencesWithFix(), true, 10);
  myFile.print(",");
  printInt(gps.failedChecksum(), true, 9);
  myFile.print(",");
  sendRequest(readCO2); //58 milliseconds
  unsigned long valCO2 = getValue(response);
  myFile.println(valCO2);
  myFile.print(",");
  myFile.println();

 // sendRequest(readCO2); //58 milliseconds
//  unsigned long valCO2 = getValue(response);
//  myFile.println(valCO2);
//  myFile.print(",");
  i = 1;
  digitalWrite(led, LOW);
  } else {

    i++;
    delay(58);
    }
  smartDelay(942); //Additional sensors may be added
  // Simply subtract the processing time required for sensor
  //from this 942

  if (millis() > 5000 && gps.charsProcessed() < 10)
    myFile.println(F("No GPS data received: check wiring"));

  myFile.close();
  }
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      myFile.print('*');
    myFile.print(' ');
  }
  else
  {
    myFile.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      myFile.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  myFile.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    myFile.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    myFile.print(sz);
  }
  
  if (!t.isValid())
  {
    myFile.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    myFile.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    myFile.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}

void sendRequest(byte packet[])
{
  while(!Serial.available())  //keep sending request until we start to get a response
  {
    Serial.write(readCO2,7);
    delay(50);
  }
  
  int timeout=0;  //set a timeoute counter
  while(Serial.available() < 7 ) //Wait to get a 7 byte response
  {
    timeout++;  
    if(timeout > 10)    //if it takes to long there was probably an error
      {
        while(Serial.available())  //flush whatever we have
          Serial.read();
          
          break;                        //exit and try again
      }
      delay(50);
  }
  
  for (int i=0; i < 7; i++)
  {
    response[i] = Serial.read();
  }  
}

unsigned long getValue(byte packet[])
{
    int high = packet[3];                        //high byte for value is 4th byte in packet in the packet
    int low = packet[4];                         //low byte for value is 5th byte in the packet

  
    unsigned long val = high*256 + low;                //Combine high byte and low byte with this formula to get value
    return val* valMultiplier;
}



