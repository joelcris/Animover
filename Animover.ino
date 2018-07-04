/*
 * Ejemplo del código para los nodos
 * Código para el nodo 3
  */
#include "GPS.h"
#include <DS3231.h>
#include <avr/sleep.h>
#include <SD.h>
#include <SPI.h>

#define N "3" 

SoftwareSerial mySerial(8, 7);
GPS myGPS(&mySerial);
#define GPSCOM

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

DS3231 clock;
RTCDateTime dt;

#define  system_start 7
   
#define system_stop 23

#define rxInt 15

uint8_t  alarmSet = 0;

uint8_t interval_time= 5;

boolean start=true;

int logNum=0; 

int logRes=180;

int counter =0;

char filename[12];

int reminder=0;

int updt=0;

String logName="";

char incomingByte = 0;

int tHour=0;
int tMinute=0;
int tSecond=0;

void alarm(){}

void setup(){
  Serial.begin(9600);

  pinMode (4, OUTPUT);    
  digitalWrite (4, HIGH);
  delay(2000);
     
  ADCSRA = ADCSRA & B01111111;
  ACSR = B10000000;
  DIDR0 = DIDR0 | B00111111;

  pinMode (5, OUTPUT);     
  digitalWrite (5, LOW);
  pinMode (6, OUTPUT);     
  digitalWrite (6, LOW);
  pinMode (9, OUTPUT);     
  digitalWrite (9, LOW);     


  clock.begin();
  clock.armAlarm1(false);
  clock.clearAlarm1();
  clock.enableOutput(false);
  pinMode(2, INPUT_PULLUP); 

  #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
    cbi(PORTC, 4);
    cbi(PORTC, 5);
  #else
    cbi(PORTD, 0);
    cbi(PORTD, 1);
  #endif

  pinMode(10, OUTPUT);
  
  dt = clock.getDateTime(); 
  tHour= dt.hour;
  tMinute= dt.minute;
  tSecond=dt.second;

  Serial.print(dt.hour);   Serial.print(":");
  Serial.print(dt.minute); Serial.print(":");
  Serial.print(dt.second);

  Serial.print(F(" SysStrt "));
  Serial.print(N);
  
  firstInit();

  if (!SD.begin(10)) { 
    Serial.print(F(" SD failure!"));
    return;
    }
  
  Serial.flush();
  }

void loop(){
  Serial.print(" ");
  Serial.print(N);
  Serial.flush();
  
  off();
  
  sleepNow();
  
  delay(2000);

  dt = clock.getDateTime(); 
  tHour= dt.hour;
  tMinute= dt.minute;
  tSecond=dt.second;
 

  if( tHour >= system_stop){
    Serial.print(F(" >= system_stop"));
    delay(2000);
    resetUntil(system_start);                   
  }
    
  else if( tHour > system_stop){   
    Serial.print(F(" tHour > system_stop)"));
    resetUntil(system_start);
  }
  
  else if( (tHour < system_start) ){ 
    Serial.print(F(" tHour < system_start"));
    resetUntil(system_start);                   
  }

  else if((tHour==21) && (tMinute==0)){

    on();
      
    tHour++;
    resetUntil(tHour); 
   
    Serial.print(F(" Tx "));
    Serial.print(N);
    Serial.println(tHour);
    
    sending();

    counter=logNum;
  }
  
  else if((tHour>= 19) && (tMinute==0)){ 

    on();
      
    tHour++;
    resetUntil(tHour); 
      
    while (dt.minute<(rxInt)){         
      
      recieve();   
      dt = clock.getDateTime();
    }
  }

  else if(start==false) {    

    on();
      
    resetAlarm(); 
    
    nextLog();

    startLog();
  }

  else{
    
    Serial.print(N);
    Serial.println(F(" ERROR"));
  }
  
}

void startLog(){

  for (int i=0;i<logRes;i++){

    logg();
  }

  logNum++; 
}


void firstInit(){
  
  if( (tHour > system_stop) && (tMinute > rxInt) ){
    
    resetUntil(system_start);
    start=false; 
  }

  else if( tHour >=system_stop){

    resetUntil(system_start);
    start=false;
  }
  
  else if( (tHour < system_start) ){ 

    resetUntil(system_start);                   
    start=false;
  }

  else {

    alarmSet=(dt.minute);
    resetAlarm(); 
    start=false;
  }    
}

void on(){
  
  pinMode (4, OUTPUT);
  digitalWrite (4, HIGH);

  pinMode (10, OUTPUT);
  pinMode (11, OUTPUT);
  pinMode (12, OUTPUT);
  pinMode (13, OUTPUT);
  
  delay(2000);

  SPI.begin();

  if (!SD.begin(10)) { 
    Serial.println(F("Card failure!"));
    return;
    }

    #ifdef GPSCOM  
      mySerial.begin(9600);
      delay(50);
      mySerial.println(F("$PMTK251,9600*17"));
      delay(50);
      mySerial.println(F("$PGCMD,33,0*6D")); 
      delay(50);
      mySerial.println(F("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29")); 
      delay(50);
      mySerial.println(F("$PMTK300,1000,0,0,0,0*1C"));  
      delay(50);
      mySerial.end();
  #endif

}

void off(){

  delay(500);

  digitalWrite (4,LOW);
  SPI.end() ;

  pinMode (10, INPUT);
  pinMode (11, INPUT);
  pinMode (12, INPUT);
  pinMode (13, INPUT);
}

void sleepNow() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0,alarm, LOW); 
        
  sleep_mode();

  sleep_disable();
  detachInterrupt(0);
}

void logg(){
  mySerial.begin(9600); 
        
  while(!myGPS.newNMEAreceived()) { 
    char c=myGPS.read();
  }   

  myGPS.parse(myGPS.lastNMEA());  

  if(myGPS.fix){

    File data = SD.open(filename, FILE_WRITE);        

    data.print(myGPS.hour, DEC); data.print(':');           
    data.print(myGPS.minute, DEC); data.print(':');     
    data.print(myGPS.seconds, DEC);                    
    data.print(",");                                  
    data.print(myGPS.latitudeDegrees, 6);              
    data.print(",");                                  
    data.println(myGPS.longitudeDegrees, 6);            

    data.close();
  }

  else{
    Serial.print(F("error in node "));
    Serial.println(N);
  }

  mySerial.end();
}

void resetAlarm(){

  clock.clearAlarm1();

  if (start){
    
    uint8_t t= ((alarmSet/interval_time)+1)*interval_time;  
    uint8_t tHolder= t;
    t=t-alarmSet;
    alarmSet=tHolder;

    if(alarmSet >= 60){
      alarmSet=0;
  }
    
  clock.setAlarm1(0, 0, alarmSet,0 , DS3231_MATCH_M_S); 
  Serial.print(N);
  Serial.print(F(" init at minute: "));
  Serial.print(alarmSet);    
  }
  
  else{
  
    alarmSet=alarmSet + (interval_time*2);  
    if(alarmSet >= 60){
      alarmSet=0;
    }

  clock.setAlarm1(0, 0, alarmSet,0 , DS3231_MATCH_M_S);
  }
}

void nextLog(){

  strcpy(filename, "N3-000.TXT");
  
  for (int i = updt; i <= logNum; i++){
    
    filename[3] = '0' + i/100; 
    reminder=i%100;
    filename[4] = '0' + reminder/10; 
    filename[5] = '0' + reminder%10;
    
    if (! SD.exists(filename)) {
      updt=i;
      break;
     }
   }
}

void sending(){

  delay(3000);
  strcpy(filename, "N3-000.TXT");
  for (int i = counter; i < logNum; i++) { 
    
    filename[3] = '0' + i/100; 
    reminder=i%100;
    filename[4] = '0' + reminder/10;
    filename[5] = '0' + reminder%10;
    delay(500);
    File data = SD.open(filename);
    if (data) {
      Serial.print("$");
      Serial.print(filename);
      Serial.print("~");

      delay(500);
      
      byte clientBuf[64];
      int clientCount = 0;
      
      while (data.available()) {
        clientBuf[clientCount] = data.read();
        clientCount++;

        if(clientCount > 63){
                Serial.write(clientBuf,64);
                delay(3);
                clientCount = 0;
        }
      }

        if(clientCount > 0) Serial.write(clientBuf,clientCount);            
        
        data.close();

        Serial.print('*');
        delay(1000);
        Serial.print('*');
        delay(1000);
        Serial.print('*');
    }
  }
}


void recieve(){

  if(Serial.available()>0){
           
    incomingByte=Serial.read();
          
    if (incomingByte=='$'){

      while(true){
 
      if(Serial.available()>0){
        incomingByte=Serial.read();
                  
        if (incomingByte=='~'){
     
            File data = SD.open(logName, FILE_WRITE);
            while(true ){
              while (!Serial.available()){
                dt = clock.getDateTime();  
                          
                if (dt.minute>=rxInt){
                  break;
                }
              }
            
            incomingByte=Serial.read();
            if(incomingByte=='*') {
              data.close();
              break;
            }
                        
            data.print(incomingByte);
          }

            data.close();
            
            logName="";
            break;
        }
                            
        else if(incomingByte!='\n') {
          logName+=incomingByte;     
        }
                 
      } 

      dt = clock.getDateTime();  
      if (dt.minute>=rxInt){
        break;
      }
      }
    }
  }
}

void resetUntil( uint8_t T){
  
  clock.clearAlarm1();

  clock.setAlarm1(0, T, 0,0 , DS3231_MATCH_H_M_S); 
    
}
