#include <SoftwareSerial.h>
#include <EEPROM.h>

//const byte HC12RxdPin = 2;                      // "RXD" Pin on HC12
//const byte HC12TxdPin = 3;                      // "TXD" Pin on HC12

SoftwareSerial gsmSerial(7, 8); // Tx, Rx SIM900
//SoftwareSerial HC12(HC12TxdPin, HC12RxdPin); //Tx
String readString = "";
String number = "";
String message = "";
//String users = "";
String names = "";
String answer = "";
const String msgEmergencia = "incendio";

const String msgGeneral = "hurto";
const int outEmergencia = 6;  // emergencia de incendio o evacuacion
const int outGeneral = 5 ;    //alarma de robo 

String password = "0419";
int tiempRele= 30;
int UnidadTiemp= 0; //si es 0=unidad segundos ; si es 1 = unidad minutos

//Tx
const byte HC12ledPin = 13;
const byte HC12SetPin = 9;

void setup()
{
  gsmSerial.begin(9600); // Setting the baud rate of GSM Module
  //HC12.begin(9600);
  Serial.begin(9600);
  pinMode(outEmergencia,OUTPUT);
  pinMode(outGeneral,OUTPUT);
 
  
  while(!gprsInit()){
    delay(3000);
    //Serial.println(F("Initialization failed"));
  }
  
  while(!isNetworkRegistered()){
    delay(3000);
    //Serial.println(F("Network failed"));
  }

  delay(1000);
  setTx();
  
  receiveMessage();
  //actualiza();
  Serial.println(F("Device Setup Complete."));

}
  void actualiza(){
    int part1= EEPROM.read(1);
    int part2= EEPROM.read(2);
    if (part1<10){password= ( "0"+(String)part1);}else {password= (String)part1; }
    if (part2<10){password= (password + "0"+(String)part2);}else {password= (password +(String)part2); }
    //Serial.println(password);

  //actualizo las variables de tiempo de alarma y unidad de tiempo

    UnidadTiemp= EEPROM.read(3);
    tiempRele = EEPROM.read(4);
  }

void eraseReg(){
    EEPROM.write(1,4);
    EEPROM.write(2,19);
    EEPROM.write(3,0);
    EEPROM.write(4,30);
}


void receiveMessage() {
gsmSerial.println("ATE 0"); //desactiva el eco de la respuesta at
delay(500);
gsmSerial.println("AT+CMGF=1\r");
delay(500);
gsmSerial.println("AT+CNMI=2,2,0,0,0"); // AT Command to receive a live SMS
delay(500);
gsmSerial.println("AT+CLIP=1"); // AT Command activa idenficador de llamada
delay(500);

//

}

void loop() {
 checkMessages();

} 

void checkMessages() {

delay(1500);
  if (gsmSerial.available()>0){
    //Serial.println("checkMessages");
   readString = gsmSerial.readString();
    delay(400);
        
       if(readString.indexOf("RING")>=0){
              Serial.println(F("llamada entrante"));

             int startNumber = readString.indexOf('"');
             int EndNumber = readString.indexOf('"', startNumber+1);
             number = readString.substring(startNumber+1, EndNumber);

             int startType = readString.indexOf(',', EndNumber+1);
             int endType = readString.indexOf(',', startType+1);
   
             int startVacio = readString.indexOf('"', endType+1);
             int endVacio = readString.indexOf('"', startVacio+1);

             int startNames = readString.indexOf('"', endVacio+1);
             int endNames = readString.indexOf('"', startNames+1);

             names = readString.substring(startNames+1, endNames);
    
    
             validateCall();
                        
           delay(300);

           
   } else if(readString.indexOf("+CMT")>=0){//IDENTIFICA SI ES MENSAJE DE TEXTO
    Serial.println(F("mensaje"));
                    int startNumber = readString.indexOf('"');
                    int EndNumber = readString.indexOf('"', startNumber+1);
                    number = readString.substring(startNumber+1, EndNumber);
      
                   int startNames = readString.indexOf('"', EndNumber+1);
                   int EndNames = readString.indexOf('"', startNames+1);
                   names = readString.substring(startNames+1, EndNames);
        
                   int startMessage = readString.lastIndexOf('"');
                   message = readString.substring(startMessage+1,readString.length()-1);
                   message = message.substring(message.indexOf('\n')+1,message.length()-1);
                   validate();
                   //Serial.println(password);
     //Serial.println(F("SMS: "+ message + "\n" + "nombre..:"+ names + "\n" + "numero..:"+number));
   
   }
 }                                
   
}


void validateCall(){

  if (validateRegUser() && validateTypeUser()){ //si el usuario esta registrado y es usuario especial 
        gsmSerial.println("ATA\r"); //contesta la alarma y se activa tiempo al aire , tambien activara la alarma y 
                                    //se enviara mensaje de texto a usuarios especiales, se hara llamda a grupo de reaccion y mensaje a grupos de accion

  bool flagCall = false;
  int count = 0;
  while(!flagCall){
     String ansCall = gsmSerial.readString();
     if(ansCall.indexOf("NO CARRIER") >= 0 || count >= 20){
      flagCall = true;
     }
     delay(800);
     count++;
     //Serial.println("flagcall while");
  }
        //Serial.println("break");
        gsmSerial.println("ATH\r"); //hang up
        delay(200);
        setAlarm(outGeneral, msgGeneral);
        getUser();
        //Serial.println("contacto especial");
  } else if (validateRegUser() && ! validateTypeUser()){ //usario estandar solo puede activar alarma y se enviara mensaje de activacion a todos los usuarios especiales 
         gsmSerial.println("ATH\r");     
         setAlarm(outGeneral, msgGeneral); 
         //Serial.println("usuario standar");
  } else {
          //Serial.println("contacto no registrado");  
          gsmSerial.println("ATH\r"); //colgar
  }
    
  
}



void validate(){
  message.toUpperCase();
  password = message.substring(5,9);
  //Serial.println(message);
  // Serial.println(password);
if(message.startsWith("#C*")){
    if(checkPass(password)){
        setConfig();}
    } else {
      //Serial.println("mensaje");
              //valido usuario
              if (validateRegUser() && validateTypeUser()){ //si el usuario esta registrado y es usuario especial 
                  setAlarm(outGeneral, msgGeneral);
                  getUser();
                   //Serial.println("contacto especial");
                      } else if (validateRegUser() && ! validateTypeUser()){
                                 
                                 //Serial.println("usuario standar");
                                       } else {
                                                //Serial.println("contacto no registrado");  
         
                                                 }  
         } 
          
}


boolean validateRegUser(){ //la funcion es verdadera si el usuario esta registrado
   if(names.equals("")){  
      return false;    
      } else{ return true;}
}

boolean validateTypeUser(){ //la funcion es verdadera si el usuario es ESPECIAL (el nombre inicia con 1xxx), si es STANDAR es falsa
   if(names.startsWith("1")){
    return true;}
    else {return false; }
  }

 void setConfig(){
  
  if(message.startsWith("#C*1,")){
           //#C*1,0419,1,200,3167041680,christian puen  este seria el tamaño maximo permitido del msn 41
           //01234567890123456789012345678901234567890
           //Serial.println("m1");
           if((message.charAt(11)==',' && message.charAt(15)==',' && message.charAt(26)==',')){
                  answer= setUser();
                  //Serial.println(answer);
                 } else{
                  Serial.println(F("formato incorrecto m1"));
                 }

        } 
           else if(message.startsWith("#C*2,")){
                               //#C*2,0419,M,99
                              //01234567890123
                     message = message.substring(10,14); 
                     if(message.startsWith("M,")){ //si es minutos guardaremos un 1 en la variabel unidadTiemp
                                           //Serial.println("M");
                                           answer = setTimeOut(1);
                                           //Serial.println(answer);
                          } else if(message.startsWith("S,")){ //si es segundos guardaremos un 0 en la variable unidadTiemp
                                           answer = setTimeOut(0);
                                           //Serial.println(answer);
                                  } else { 
                                    Serial.println(F("formato incorrecto"));
                                    }
                    
              }
                   else if(message.startsWith("#C*3,")){
                                        //#C*3,0419,1234,1234
                                       //0123456789012345678
                                       message = message.substring(10,19);
                                       answer= setPassword(password);
                                       //Serial.println(answer);
                
                          }
                                else if(message.startsWith("#C*4,")){
                                                    //#C*4,0419
                                                answer=deleteUsers();
                                                //Serial.println(answer); 
                                     }
                                           else if(message.startsWith("#C*5,")){
                                                           //Serial.println("m5");
                                                  } else {
                                                    Serial.println(F("incorrecto"));
                                                    }


    }
 
//posicion grap 1-5  special 6-
////********************************CONFIGURACION*************************************
////**********************************************************************************
String setUser(){  
  //Serial.println("ingreso a se user");
   //[#C*1,1234,1,200,1234567890,ABCDFGHIJKLMNO]
   //[01234567890123456789012345678901234567890]
   //Tipo CONTACTO: 1 GRUPO ACCION (1-5), 2 CONTACTO SPECIAL (6-20), 3 CONTACTO STANDAR (21-80)
message= message.substring(10,41);
int pos = message.substring(2,5).toInt();
if(message.startsWith("1,")){
     if(!checkPos(pos,1)){
      return "posicion incorrecta";
     }
     //Serial.println("tipo1");
      }
        else if(message.startsWith("2,")){
                 if(!checkPos(pos,2)){
                      return "posicion incorrecta";
                      }
                //  Serial.println("tipo2");
    
           } 
             else if(message.startsWith("3,")){
                      if(!checkPos(pos,3)){
                           return "posicion incorrecta";
                          }
                     //  Serial.println("tipo3");
             
               }
                 else {return "tipo incorrecto";}

 String numberUser = message.substring(6,16);
 if(!CheckNumber(numberUser)){
   return "Numero telefonico incorrecto";
 }
  String nameUser = message.substring(17, message.length());
  gsmSerial.println("AT+CPBS=\"SM\"");
  delay(500); 
  gsmSerial.println("AT+CPBW="+ (String)pos+",\""+numberUser+"\",,\""+nameUser+"\"");
  //Serial.println(pos);
  delay(500);
 //Serial.println(nameUser);
 return "Contacto almacenado";
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

boolean checkPos(int pos, int type){
  if(type==1){
    //pos debe estar entre 1 y 5 q pertenece al grupo de accion, total de 5 usuarios grupo de reaccion
    if(!(pos>0 && pos<=5)){return false;}
    }
  if(type==2){
    //pos debe estar entre 6 y 45 que pertenece al grupo de contactos speciales Para un total de 40 usuarios especiales
    if(!(pos>5 && pos<=45)){return false;}
    }
  if(type==3){
    //pos debe estar entre 46 y 200 que pertenece al grupo contactos standar, total de 155 usuarios standar
    if(!(pos>45 && pos<=200)){return false;}
    }
    
   return true;
  
}


String setTimeOut(int unity){
  //[#CX2,CLAVE,UNIDAD[M/S](1),TIEMPO RELE(2)]
  UnidadTiemp = unity;
  tiempRele = message.substring(2,4).toInt();
  if(!CheckNumber(tiempRele)){
    return "Tiempo de relevo incorrecto";
  }
  
 EEPROM.write(3,UnidadTiemp);
 EEPROM.write(4,tiempRele);
 

  return "Unidad: tiempo tele:"+ (String)tiempRele+" unidad: "+ (String)unity+" .";
}

String setPassword(String passwordd){
   //[#Cx3,CLAVE,CLAVE NUEVA(4),CLAVE NUEVA CONFIRMACION(4)]
   //[#Cx3,1234,1234,1234]
   //[0123456789012345678]
  String newPass = message.substring(0,4);
  String confirmPass = message.substring(5,9);
  if(newPass.equals(confirmPass)){
   int passwords = newPass.substring(0,2).toInt();
   EEPROM.write(1,passwords);
   passwords = newPass.substring(2,4).toInt();
   EEPROM.write(2,passwords);
    return "Clave nueva:"+newPass+", clave antigua:"+passwordd;
     
  }else{
    return "Claves no coinciden";
  }
}

String deleteUsers(){
/**********************************************************************
  Pendiente
***********************************************************************/
  for (int i=1; i<201; i++){
 
    gsmSerial.println("AT + CPBW ="+(String)i+"\r");
    gsmSerial.println();
    delay(200);
  }
  //hard restart**********************************************************
  return "OK";
}

String testSystem(){
/**********************************************************************
  Pendiente
***********************************************************************/
  return "Test";
}
//
////**********************************OPERACION***************************************
////**********************************************************************************
void setAlarm(int out, String msg){ //e suena con tono 1 ..y si diferente tono2.
//  
//  digitalWrite(out,HIGH);
//  answer = "El usuario con numero telefonico "+number+" activo alarma de "+msg;
//  Serial.println(answer);
//  //sendMessage(number);
//  //smsSpecialUsers();
//  delay(1000);
  //digitalWrite(out,LOW);
  Serial.println(F("Enviando Alarma..."));
  //gsmSerial.end();
  //delay(3000);
  //HC12.begin(9600);
  //delay(3000);
  //HC12.println("ON");
  digitalWrite(HC12ledPin, HIGH);
  //delay(1000);
  //HC12.end();

  delay(5000);
  //gsmSerial.begin(9600);
  
  digitalWrite(HC12ledPin, LOW);
  //delay(3000);  
}


void sendMessage(String n) {

  //Serial.println("mensaje enviado a "+n);
  //Serial.println("Enviando SMS...");
  gsmSerial.print("AT + CMGF = 1\r"); //Comando AT para mandar un SMS
  delay(1000);
  gsmSerial.println("AT + CMGS =\""+n+"\""); //Numero al que vamos a enviar el mensaje
  delay(1000);
  gsmSerial.println(answer);// Texto del SMS
  delay(100);
  gsmSerial.println((char)26);//Comando de finalizacion ^Z
  delay(100);
  gsmSerial.println();
  delay(5000); // Esperamos un tiempo para que envíe el SMS

  delay(17000);
  
  bool ok = true;
  while(ok){
   // Serial.println("intento while");
    String msn = gsmSerial.readString();
    //Serial.println(msn);
    delay(1500);
    if ((msn.indexOf("ERROR")>=0)){ok= false;
      Serial.println(F("SMS NO enviado")); 
    delay(1000);}
     else if (msn.indexOf("OK OK ")){ok= false;
      Serial.println(F("SMS  enviado")); 
     delay(1000); }
  }
    
}




////*******************************UTILIDADES*****************************************
////**********************************************************************************

bool checkPass(String clave){
 
  //actualiza();
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



//////////   ******************CONEXION MODULO GSM ******************************


bool gprsInit(){
  if(!checkCmd(F("AT"), F("OK"))){
    return false;
  }
  if(!checkCmd(F("AT + CFUN = 1"), F("OK"))){
    return false;
  }
  return true;
}

bool isNetworkRegistered(){
  int count = 0;
  while(count < 3){
    gsmSerial.println(F("AT + CREG?"));
    delay(500);
    String respMod = gsmSerial.readString();
    delay(500);
    
  if(respMod.indexOf("+CREG: 0,1")>=0 || respMod.indexOf("+CREG: 0,5")>=0){   
    break;
   }
    count++;
    delay(300);
  }
  if(count == 3){
    return false;
  }
  return true;
}

bool checkCmd(String cmd, String resp){
  gsmSerial.println(cmd);
  delay(2000);
  String respMod = gsmSerial.readString();
  if(respMod.indexOf(resp) >= 0){
    return true;
  }
  return false;
}

//**********************************************
///////////////////////////////////////////////////////////////////////////
void getUser(){

  for ( int i=6; i<=45; i++){
  delay(500);
 // Serial.println((String)i);
  String sms = findREgSim(i);
  delay(1000);
//Serial.println(sms);
  answer= "ALARMA GENERAL GENERADA POR EL USUARIO CASALOTEBARRIO CEL 3167041680. indice: " + (String)i;
 if (CheckNumber(sms)){
  
  //sendMessage(sms);
  
 }else {delay(500);}
   
 }  
}

String findREgSim(int i){
    
    gsmSerial.println( "AT + CPBR="+(String)i +"\r");
    delay(1000);
   String temp =gsmSerial.readString();
  // delay(500);
    return temp = getNum(temp);
            
}

String getNum(String m){
  int startNumber = m.indexOf('"');
  int EndNumber = m.indexOf('"', startNumber+1);
  return m.substring(startNumber+1, EndNumber);
  /*
  int startType = tempo.indexOf(',', EndNumber+1);
  int endType = tempo.indexOf(',', startType+1);
  
  int startName = tempo.indexOf('"', endType+1);
  int endName = tempo.indexOf('"', startName+1);
  names = tempo.substring(startName+1, endName);
  */
}

void setTx(){
  pinMode(HC12SetPin, OUTPUT);                  
  digitalWrite(HC12SetPin, HIGH); 
  delay(1000);
  pinMode(HC12ledPin, OUTPUT);                  
  digitalWrite(HC12ledPin, LOW);
  //delay(5000);
  //HC12.begin(9600);
  //Serial.begin(9600);
}
