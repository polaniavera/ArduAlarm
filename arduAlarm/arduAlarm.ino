#include <SoftwareSerial.h>
#include <SPI.h>

/*
[DEBUG] 
+CMT: "3167041680","","19/05/10,03:12:46-20"
mensaje
*/

SoftwareSerial gsmSerial(7, 8); // Tx, Rx
String readString = "";
String number = "";
String message = "";
String users = "";
String answer = "";
const String configuration = "C";
const String fire = "I";
const String evacuation = "E";
const String minute = "M";
const String second = "S";
const String specialType = "161";
const String standardType = "129";
const String msgFire = "incendio";
const String msgEvacuation = "evacuacion";
const String msgGeneral = "hurto";
const int outFire = 10;
const int outEvacuation = 11;
const int outGeneral = 12;
String password = "1234";

void setup()
{
  gsmSerial.begin(9600); // Setting the baud rate of GSM Module
  Serial.begin(9600); // Setting the baud rate of Serial Monitor
  delay(200);
  pinMode(outFire,OUTPUT);
  pinMode(outEvacuation,OUTPUT);
  pinMode(outGeneral,OUTPUT);
  receiveMessage();
  getUsers();
/**********************************************************************
  Leer EEPROM
***********************************************************************/
  Serial.println("Device Setup Complete.");
}

void loop() {
  checkMessages();
/**********************************************************************
  Chequear llamada
***********************************************************************/
}

void checkMessages() {
  if (gsmSerial.available()>0){
    readString = gsmSerial.readString();
    if(readString.length()>10) {
      int startNumber = readString.indexOf('"');
      int EndNumber = readString.indexOf('"', startNumber+1);
      number = readString.substring(startNumber+1, EndNumber);
      int startMessage = readString.lastIndexOf('"');
      message = readString.substring(startMessage+1,readString.length()-1);
      message = message.substring(message.indexOf('\n')+1,message.length()-1);
      validate();
    }else{
      Serial.println("El modulo esta activo y escuchando mensajes SMS.");
    }
  }
}

void validate(){
  message.toUpperCase();
  if(message.substring(0, 1).equals(configuration)){
    setConfig();
  }else if(checkUser()){
    if(message.equals(fire)){
      setAlarm(outFire, msgFire);
    }else if(message.equals(evacuation)){
      setAlarm(outEvacuation, msgEvacuation);
    }else{
      setAlarm(outGeneral, msgGeneral);
    }
  }else{
    answer = "Usuario no encontrado";
    sendMessage(number);
    Serial.println(answer);
  }
}

void setConfig(){
  //C1: Usuario
  //C2: Tiempos de sirena y perifoneo
  //C3: Cambiar password
  //C4: Borrar usuario
  //C5: test
  switch (message.substring(1, 2).toInt()) {
  case 1:
    //[C1,CLAVE,TIPO[1/0](1),NUMERO(10),NOMBRE(MAX 14)]
    answer = setUser();
    getUsers();
    break;
  case 2:
    //[C2,CLAVE,UNIDAD[M/S](1),TIEMPO RELE(2),TIEMPO VOZ(2)]
    answer = setTimeOut();
    break;
  case 3:
    //[C3,CLAVE,CLAVE NUEVA(4),CLAVE NUEVA CONFIRMACION(4)]
    answer = setPassword();
    break;
  case 4:
    //[C4,CLAVE(4),BORRADO[A/1-53](1/2))]
    answer = deleteUsers();
    getUsers();
    break;
  case 5:
    //[C5,CLAVE]
    answer = testSystem();
    break;
  default:
    answer = "Formato incorrecto";
    break;
  }
  sendMessage(number);
  Serial.println(answer);
}

//********************************CONFIGURACION*************************************
//**********************************************************************************
String setUser(){
  //Tipo usuario: 1 especial (161), 0 standard (129)
  if(!(message.charAt(2)==',' && message.charAt(7)==',' && message.charAt(9)==',' && message.charAt(20)==',')){
    return "formato incorrecto C1";
  }
  if(!checkPass()){
    return "Clave incorrecta";
  }
  String type = "";
  if(message.substring(8,9).equals("1")){
    type = specialType;
  }else if(message.substring(8,9).equals("0")){
    type = standardType;
  }else{
    return "Tipo de usuario incorrecto";
  }
  String numberUser = message.substring(10,20);
  if(!CheckNumber(numberUser)){
    return "Numero telefonico incorrecto";
  }
  String nameUser = message.substring(21, message.length());
  gsmSerial.println("AT+CPBS= \"SM\"");
  delay(300);
  gsmSerial.println("AT+CPBW=,\""+numberUser+"\","+type+",\""+nameUser+"\"");
  delay(300);
  return "Contacto almacenado";
}

String setTimeOut(){
  if(!(message.charAt(2)==',' && message.charAt(7)==',' && message.charAt(9)==',' && message.charAt(12)==',')){
    return "formato incorrecto C2";
  }
  if(!checkPass()){
    return "Clave incorrecta";
  }
  String unity = message.substring(8,9);
  if(!(unity.equals(minute) || unity.equals(second))){
    return "Unidad incorrecta";
  }
  String tRele = message.substring(10,12);
  if(!CheckNumber(tRele)){
    return "Tiempo de relevo incorrecto";
  }
  String tVoice = message.substring(13,15);
  if(!CheckNumber(tVoice)){
    return "Tiempo de voz incorrecto";
  }
/**********************************************************************
  Pasar valores de tOut a EEPROM
***********************************************************************/
  return "Unidad:"+unity+", tiempo tele:"+tRele+", tiempo voz:"+tVoice;
}

String setPassword(){
  if(!(message.charAt(2)==',' && message.charAt(7)==',' && message.charAt(12)==',')){
    return "Formato incorrecto C3";
  }
  if(!checkPass()){
    return "Clave incorrecta";
  }
  String newPass = message.substring(8,12);
  String confirmPass = message.substring(13,17);
  if(newPass.equals(confirmPass)){
/**********************************************************************
  Pasar valores de clave a EEPROM
***********************************************************************/
    return "Clave nueva:"+newPass+", clave antigua:"+message.substring(3,7);  
  }else{
    return "Claves no coinciden";
  }
}

String deleteUsers(){
/**********************************************************************
  Pendiente
***********************************************************************/
  for (int i=1; i<20; i++){
    //String num = "AT+CPBW="+(String)i+"\n\r";
    gsmSerial.println("AT + CPBW ="+(String)i+"\r");
    gsmSerial.println();
    delay(300);
  }
  String retorno2= gsmSerial.readString();
  //int largo= retorno2.length();
  retorno2 = retorno2.substring(retorno2.indexOf('\n' )+5,retorno2.length()-41);
  return "estado de procedimiento: "+retorno2;
}

String testSystem(){
/**********************************************************************
  Pendiente
***********************************************************************/
  return "Test";
}

//**********************************OPERACION***************************************
//**********************************************************************************
void setAlarm(int out, String msg){
  digitalWrite(out,HIGH);
  answer = "El usuario con numero telefonico "+number+" activo alarma de "+msg;
  Serial.println(answer);
  smsSpecialUsers();
  delay(10000);
  digitalWrite(out,LOW);
}

void smsSpecialUsers(){
  char delimiter[] = "+";
  String temp = "";
  String numSpecial = "";
  
  //Convertir String users a char[]
  char charCon[users.length() + 1];
  users.toCharArray(charCon, users.length() + 1);
  
  //Pasar char[] a String, obtener tipo y enviar sms
  char* ptr = strtok(charCon, delimiter);
  while(ptr != NULL) {
    temp = String(ptr);
    int startNumber = temp.indexOf('"');
    int endNumber = temp.indexOf('"', startNumber+1);
    if(temp.substring(endNumber+2, endNumber+5).equals(specialType)){
      numSpecial = temp.substring(startNumber+1, endNumber);
      sendMessage(numSpecial);
    }
    ptr = strtok(NULL, delimiter);
  } 
}

void sendMessage(String n) {
  gsmSerial.println("AT+CMGF=1"); //Sets the GSM Module in Text Mode
  delay(1000);
  gsmSerial.println("AT+CMGS=\""+n+"\"\r");
  delay(1000);
  gsmSerial.println(answer);
  delay(100);
  gsmSerial.println((char)26);
  delay(1000);
/**********************************************************************
  Revisar respuesta
***********************************************************************/  
  Serial.println("mensaje enviado a "+n);
}

bool checkUser(){
  return users.indexOf(number) > 0 ? true : false;
}

//*******************************UTILIDADES*****************************************
//**********************************************************************************
void receiveMessage() {
  gsmSerial.println("AT+CNMI=2,2,0,0,0"); // AT Command to receive a live SMS
  delay(1000);
}

void getUsers(){
  gsmSerial.println("AT+CPBS= \"SM\"");
  gsmSerial.println( "AT+CPBR=1,53\n\r");
  delay(200);
  users = gsmSerial.readString();
}

bool checkPass(){
  String clave= message.substring(3,7);
  return clave.equals(password) ? true : false;
}

bool CheckNumber(String n){
  int stringLength = n.length();
  if (stringLength == 0) {
    return false;
  }
  for(int i = 0; i < stringLength; i++) {
    if (isDigit(n.charAt(i))) {
      continue;
    }else {
     return false;
    }
  }
  return true;
}
