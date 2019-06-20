#include <GPRS_Shield_Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define PIN_TX    7
#define PIN_RX    8
#define BAUDRATE  9600
#define MESSAGE_LENGTH 20
#define OUT_FIRE 10
#define OUT_EVACUATION 11
#define OUT_GENERAL 12

char gprsBuffer[64];
char *p,*s;
char message[MESSAGE_LENGTH];
char phone[16];
char datetime[24];

GPRS gprs(PIN_TX,PIN_RX,BAUDRATE);

/*
[DEBUG] 
+CMT: "3167041680","","19/05/10,03:12:46-20"
mensaje
*/

const String msgFire = "incendio";
const String msgEvacuation = "evacuacion";
const String msgGeneral = "hurto";
char password[5] = "1234";

void setup()
{
  pinMode(OUT_FIRE,OUTPUT);
  pinMode(OUT_EVACUATION,OUTPUT);
  pinMode(OUT_GENERAL,OUTPUT);

  gprs.checkPowerUp();
  Serial.begin(9600);
  while(!gprs.init()){
    delay(1000);
    Serial.println(F("Initialization failed!"));
  }
  
  while(!gprs.isNetworkRegistered()){
    delay(1000);
    Serial.println(F("Network has not registered yet!"));
  }
  Serial.println(F("Device Setup Complete."));
}

void loop() {
  if(gprs.readable()) {
    sim900_read_buffer(gprsBuffer,32,DEFAULT_TIMEOUT);
    //Serial.print(gprsBuffer);
    
    if(NULL != strstr(gprsBuffer,"RING")) {
      gprs.answer();
    }else if(NULL != (s = strstr(gprsBuffer,"+CMT:"))) {
      if(NULL != (s = strstr(s,"\r\n"))){
        p = s+2;
        int i = 0;
        while((*p != '\r')&&(i < MESSAGE_LENGTH-1)) {
          message[i++] = *(p++);
        }
        message[i] = '\0';
        Serial.println(message);
        validate();
      }

//    int messageIndex = gprs.isSMSunread();
//    if (messageIndex > 0) {
//      gprs.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);
//      gprs.deleteSMS(messageIndex);
//      Serial.println(phone);  
//      Serial.println(datetime);        
//      Serial.println(message);    
//    }
    
    }
    sim900_clean_buffer(gprsBuffer,32);
  }
  else { 
    delay(100);
  }
}

void validate(){
  //message.toUpperCase();
  if(message[0] == 'C'){
    setConfig();
  }else if(true){ // validar usuario
    if(message == 'I'){
      //setAlarm(OUT_FIRE, msgFire);
    }else if(message == 'E'){
      //setAlarm(OUT_EVACUATION, msgEvacuation);
    }else{
      //setAlarm(OUT_GENERAL, msgGeneral);
    }
  }else{
    Serial.println(F("Usuario no encontrado"));
  }
}

void setConfig(){
  //C1: Usuario
  //C2: Tiempos de sirena y perifoneo
  //C3: Cambiar password
  if(message[1] == '1'){
    setUser();
  }else if(message[1] == '2'){
    //setTimeOut();
  }else if(message[1] == '3'){
    //setPassword();
  }else{
    Serial.println(F("Formato incorrecto"));
  }
}

//********************************CONFIGURACION*************************************
//**********************************************************************************
void setUser(){
  if(!(message[2]==',' && message[7]==',' && message[9]==',' && message[20]==',')){
    Serial.println(F("formato incorrecto C1"));
    return;
  }
  char num[11];
  strncpy(num, message+10, 10);
  num[11] = '\0';
  
  char nam[15];
  strncpy(nam, message+21, 14);
  nam[15] = '\0';
  //gsmSerial.println("AT+CPBS= \"SM\"");
  gprs.addBookEntry(1, num, 129, nam);
}
