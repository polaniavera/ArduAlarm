#include <SoftwareSerial.h>
#include <SPI.h>

/*
[DEBUG] 
+CMT: "3167041680","","19/05/10,03:12:46-20"
mensaje
*/

SoftwareSerial gsmSerial(7, 8); // Tx, Rx
String readString="";
String number = "";
String message = "";
String users = "";
String onAlarm = "ON";
const int buzzer = 10;
String password = "1234";

void setup()
{
  gsmSerial.begin(9600); // Setting the baud rate of GSM Module
  Serial.begin(9600); // Setting the baud rate of Serial Monitor
  delay(200);
  pinMode(buzzer,OUTPUT);
  receiveMessage();
  getUsers();
  Serial.println("Device Setup Complete.");
}

void loop() {
  checkMessages(); // Check for messages
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
    }
    else{
      Serial.println("El modulo esta activo y escuchando mensajes SMS.");
    }
  }
}

void validate(){
  message.toUpperCase();
  if(message.substring(0, 1).equals("C")){
    setConfig();
  }else if(checkUser()){
    if(message.equals(onAlarm)>0){
    arm();
    }
    else{
      Serial.println("Comando Incorrecto");
    }
  }else{
    Serial.println("Usuario no encontrado");
  }
}

void setConfig(){
  //C1: Configuracion de user
  //C2: Configuracion de tiempos de salida relevada y salida voz
  //C3: Cambiar password
  //C4: Borrar
  //C5: test

  switch (message.substring(1, 2).toInt()) {
  case 1:
    //SET USER/TYPE 
    //FORMAT MSN [C1,CLAVE DE 4 DIGITOS, NUMERO DE 10 DIGITOS, NOMBRE SOLO HASTA 14 DIGITOS]
    String rtnUser = setUser();
    Serial.println(rtnUser);
    getUsers();
    break;
  case 2:
    //SET TIME OUTPUT: RELE & VOZ 
    //FORMAT MSN [C2,CLAVE 4 DIGITOS, MINUOTOS/SEGUNDOS DE 1 DIGITO, TIEMPO SALIDA RELE (DOS DIGITOS), TIMEPO SALIDA VOZ(2 DIGITOS)]
    String rtnTout = setTimeOut();
    Serial.println(rtnTout);
    break;
  case 3:
    //SET CHANGE/UPGRADE PASSWORD
    //FORMT MSN [C3,CLAVE (4 DIGITOS),CLAVE NUEVA (4DIGITOS), CLAVE NUEVA CONFIRMACION (4DIGITOS)]
    String rtnPass = setPassword();
    Serial.println(rtnPass);
    break;
  case 4:
    //SET DELETE
    //FORMAT MSN[C4,CLAVE(4DIGITOS), SELECCION DE BORRADO (1 o 2 DIGITO(s))]
    // SELECCION DE BORRADO, SI EL DIGITO ES IGUAL 'A' = BORRADO TOTAL
    //                       SI EL DIGITO ES IGUAL 'NUMERO ENTERO DEL 1-53'
    String rtnDel = deleteUsers();
    Serial.println(rtnDel);
    getUsers();
    break;
  case 5:
    //SET TEST SYSTEM
    //FORMAT MSN [C5,CLAVE(4DIGITOS)]
    //testea haciendo una llamada perdida a los numeros standar
    //y envia mensaje a contactos numeros especiales
    
    testSystem();
    break;
  default:
    Serial.println("Formato incorrecto");
    break;
  }
}

//********************************CONFIGURACION*************************************
//**********************************************************************************
String setUser(){
  if(!(message.charAt(2)==',' && message.charAt(7)==',' && message.charAt(18)==',')){
    return "formato incorrecto C1";
  }
  if(!checkPass()){
    return "Clave incorrecta";
  }
  String numero = message.substring(8,18);
  if(!CheckNumber(numero)){
    return "Numero telefonico incorrecto";
  }
  String nombre = message.substring(19,message.length());
  gsmSerial.println("AT+CPBS= \"SM\"");
  delay(300);
  gsmSerial.println("AT+CPBW=,\""+numero+"\",,\""+nombre+"\"");
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
  String unidad = message.substring(8,9);
  if(!(unidad.equals("M") || unidad.equals("S"))){
    return "Unidad incorrecta";
  }
  String tRele = message.substring(10,12);
  if(!CheckNumber(tRele)){
    return "Tiempo de relevo incorrecto";
  }
  String tVoz = message.substring(13,15);
  if(!CheckNumber(tVoz)){
    return "Tiempo de voz incorrecto";
  }

  return "UNIDAD:"+unidad+"\n"+"tiempo salida tele:"+tRele+"\n"+"tiempo salida voz:"+tVoz;
}

String setPassword(){
  //FMT MSN [C3,1234,5678,5678]
  if(!(message.charAt(2)==',' && message.charAt(7)==',' && message.charAt(12)==',')){
    return "Formato incorrecto C3";
  }
  if(!checkPass()){
    return "Clave incorrecta";
  }
  String newPass = message.substring(8,12);
  String confirmPass = message.substring(13,17);
  if(newPass.equals(confirmPass)){
    return "Clave antigua:"+message.substring(3,7)+"\n"+"clave nueva:"+newPass+"\n"+"confirmacion clave:"+confirmPass;  
  }else{
    return "Claves no coinciden";
  }
}

String deleteUsers(){
  for (int i=1; i<20; i++){
    //String num = "AT+CPBW="+(String)i+"\n\r";
    gsmSerial.println("AT + CPBW ="+(String)i+"\r");
    gsmSerial.println();
    delay(300);
  }
  String retorno2= gsmSerial.readString();
  //int largo= retorno2.length();
  retorno2 = retorno2.substring(retorno2.indexOf('\n' )+5,retorno2.length()-41);
  Serial.print("estado de procedimiento: "+retorno2);
}

void testSystem(){
  
}

//**********************************OPERACION***************************************
//**********************************************************************************
void arm () {
  Serial.println("ALERTA!");
  sendMessage();
  digitalWrite(buzzer,HIGH);
  delay(10000);
  digitalWrite(buzzer,LOW);
}

void sendMessage() {
  String mobileNumber = "";
  gsmSerial.println("AT+CMGF=1"); //Sets the GSM Module in Text Mode
  delay(1000);
  gsmSerial.println("AT+CMGS=\""+mobileNumber+"\"\r");
  delay(1000);
  gsmSerial.println("Alerta de seguridad!");
  delay(100);
  gsmSerial.println((char)26);
  delay(1000);
}

bool checkUser(){
  if(users.indexOf(number) > 0){
    return true;
  }
  return false;
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
