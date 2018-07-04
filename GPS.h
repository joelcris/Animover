

//#ifndef _ADAFRUIT_GPS_H
#define _ADAFRUIT_GPS_H    /

#include "SoftwareSerial.h"

#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"

#define MAXLINELENGTH 68   //120 RMC~[66]

// we double buffer: read one line in and leave one for the main program
volatile char line1[MAXLINELENGTH];
volatile char line2[MAXLINELENGTH];
// our index into filling the current line
volatile uint8_t lineidx=0;
// pointers to the double buffers
volatile char *currentline;
volatile char *lastline;
volatile boolean recvdflag;
//volatile boolean inStandbyMode;

class GPS{
  public:
  void begin(uint16_t baud);
  
  GPS(SoftwareSerial *ser); // Constructor when using SoftwareSerial
  
  char *lastNMEA(void);
  boolean newNMEAreceived();
  void common_init(void);

  uint8_t parseHex(char c);
  
  char read(void);
  boolean parse(char *);
  //boolean parse(char *);

  uint8_t hour, minute, seconds, year, month, day;
  //uint16_t milliseconds;
  
  float latitude, longitude;
  int32_t latitude_fixed, longitude_fixed;
  float latitudeDegrees, longitudeDegrees;
  //float geoidheight, altitude;
  float speed;
  char lat, lon;
  boolean fix;
  //uint8_t fixquality, satellites;
  

 private:

  //boolean paused;
  
  
    SoftwareSerial *gpsSwSerial;
  
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////7
//#endif

boolean GPS::parse(char *nmea) {
  // do checksum check

  // first look if we even have one
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += parseHex(nmea[strlen(nmea)-2]);
    
    // check checksum 
    for (uint8_t i=1; i < (strlen(nmea)-4); i++) {
      sum ^= nmea[i];
    }
    if (sum != 0) {
      // bad checksum :(
      //return false;
    }
  }
  int32_t degree;
  long minutes;
  char degreebuff[10];
  // look for a few common sentences
//  
  if (strstr(nmea, "$GPRMC")) {
   // found RMC
    char *p = nmea;

    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

//    milliseconds = fmod(timef, 1.0) * 1000;

    p = strchr(p, ',')+1;
    // Serial.println(p);
    if (p[0] == 'A') 
      fix = true;
    else if (p[0] == 'V')
      fix = false;
    else
      return false;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      long degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      long minutes = 50 * atol(degreebuff) / 3;
      latitude_fixed = degree + minutes;
      latitude = degree / 100000 + minutes * 0.000006F;
      latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
      latitudeDegrees += int(latitude/100);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'S') latitudeDegrees *= -1.0;
      if (p[0] == 'N') lat = 'N';
      else if (p[0] == 'S') lat = 'S';
      else if (p[0] == ',') lat = 0;
      else return false;
    }
    
    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      longitude_fixed = degree + minutes;
      longitude = degree / 100000 + minutes * 0.000006F;
      longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
      longitudeDegrees += int(longitude/100);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'W') longitudeDegrees *= -1.0;
      if (p[0] == 'W') lon = 'W';
      else if (p[0] == 'E') lon = 'E';
      else if (p[0] == ',') lon = 0;
      else return false;
    }
    // speed
    p = strchr(p, ',')+1;
//    if (',' != *p)
//    {
//      speed = atof(p);
//    }
    
    // angle
    p = strchr(p, ',')+1;
//    if (',' != *p)
//    {
//      angle = atof(p);
//    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
    }
    // we dont parse the remaining, yet!
    return true;
  }

  return false;
}


GPS::GPS(SoftwareSerial *ser)

{
  common_init();     // Set everything to common state, then...
  gpsSwSerial = ser; // ...override gpsSwSerial with value passed.
}

char GPS::read(void) {
  char c = 0;
  
  //if (paused) return c; //revisar pause()


    if(!gpsSwSerial->available()) return c;
    c = gpsSwSerial->read();

 

  //Serial.print(c);

//  if (c == '$') {        
//    currentline[lineidx] = 0;
//    lineidx = 0;
//  }
  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    //Serial.println("----");
    //Serial.println((char *)lastline);
    //Serial.println("----");
    lineidx = 0;
    recvdflag = true;
  }

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH-1;

  return c;
}

void GPS::common_init(void) {
#ifdef __AVR__
  gpsSwSerial = NULL; // Set both to NULL, then override correct
#endif
//  gpsHwSerial = NULL; // port pointer in corresponding constructor
  recvdflag   = false;
  //paused      = false;
  lineidx     = 0;
  currentline = line1;
  lastline    = line2;

  hour = minute = seconds = year = month = day =
    //fixquality = satellites = 0; // uint8_t
  lat =0; // char
  fix = false; // boolean
//  milliseconds = 0; // uint16_t
  latitude = longitude 
     = 0.0; // float
}

void GPS::begin(uint16_t baud){
#ifdef __AVR__
  if(gpsSwSerial) 
    gpsSwSerial->begin(baud);
  
#endif

  delay(10);
}

boolean GPS::newNMEAreceived(void) {
  return recvdflag;
}

char *GPS::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}
uint8_t GPS::parseHex(char c) {   //<--- After * comes checksum in HEX
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
    // if (c > 'F')
    return 0;
}
