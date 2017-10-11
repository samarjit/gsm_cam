//https://forum.arduino.cc/index.php?topic=376911.msg3189385#msg3189385

#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SD.h>


#define csSD 53
#define chipSelect 53
#define  buttonPin  12

bool debug = 1;
bool roaming = true;
char* file_name = "IMAGE00.JPG";
File dataFile;
File dataFileCopy;
const int Sim900PowerPin = 46;
String apn_name = "APN";
String apn_username = ""; //not tested
String apn_password = ""; //not tested
String ftp = "54.84.255.118";
String ftp_user = "sim9";
String ftp_password = "samar@123";

int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button
byte answer = 0;
 
Adafruit_VC0706 cam = Adafruit_VC0706(&Serial2);

void setup() {
  if (debug) {Serial.begin(38400);}
  Serial1.begin(38400);
}

void setup1() {
  if (debug) {Serial.begin(38400);}
  Serial1.begin(38400);

 
  pinMode(buttonPin, INPUT);
  pinMode(csSD, OUTPUT);

  //init SD Card
 
  int i = 0;
  while (i < 5 && answer == 0) {
    if (!SD.begin(csSD)) {
      if (debug) {Serial.println("SD Card initialization failed!");}
      i++;
      delay(500);
    }
    else {
      if (debug) {Serial.println("SD Card initialization done.");}
      answer = 1;
    }
  }
  if (answer == 0) {/*Send SMS that init SD Card is failed*/}
  //done init SD card


  // Try to locate the camera
  if (cam.begin(115200)) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }
  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }

  cam.setImageSize(VC0706_320x240);
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_640x480) Serial.println("640x480");
  if (imgsize == VC0706_320x240) Serial.println("320x240");
  if (imgsize == VC0706_160x120) Serial.println("160x120");

}

void gprsUploadFile(){
  
  //check if file available
  answer = checkFile();
  
  if (answer == 0) {/*Send SMS that file is not avriable*/}
//
//  //GPRS part
//  else if(answer == 1) {answer = powerUp_Sim900();}
//  if (answer == 0) {/*Send SMS that powerUP is failed*/}
   {delay(10000); answer = initSim900();} //Delay to let startuo the SIM900 Module
  if (debug) {Serial.println("--- Answer initSim900: " + String(answer));}
  if (answer == 0) {/*Send SMS that Sim900 init failed*/}
  else if (answer == 1) {answer = initSim900FTP();}
  if (debug) {Serial.println("--- Answer initSim900FTP: " + String(answer));}
  if (answer == 0) {/*Send SMS that Sim900FTP init failed*/}
  else if (answer == 1) {answer = Sim900FTPSend();}
  if (debug) {Serial.println("--- Answer Sim900FTPSend: " + String(answer));}
  if (answer == 0) {/*Send SMS that Sim900FTPSend failed*/}
  
}

void takePicture(){
  
  if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");

  if(cam.stepFrame()){
    Serial.println("Step Frame!");
  }
  // Create an image with the name IMAGExx.JPG
  char filename[13];
  strcpy(filename, "IMAGE00.JPG");
  for (int i = 0; i < 100; i++) {
    filename[5] = '0' + i/10;
    filename[6] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }
  strcpy(file_name, filename);
  
  // Open the file for writing
  File imgFile = SD.open(filename, FILE_WRITE);
  Serial.print("Created filename ");Serial.println(filename);
  
  // Get the size of the image (frame) taken  
  uint16_t jpglen = cam.frameLength();
  Serial.print("Storing ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

  int32_t time = millis();
 //pinMode(10, OUTPUT);
  // Read all the data up to # bytes!
  byte wCount = 0; // For counting # of writes
  while (jpglen > 0) {
    // read 32 bytes at a time;
    uint8_t *buffer;
    uint8_t bytesToRead = min(64, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);
    imgFile.write(buffer, bytesToRead);
    if(++wCount >= 64) { // Every 2K, give a little feedback so it doesn't appear locked up
      Serial.print('.');
      wCount = 0;
    }
    //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
    jpglen -= bytesToRead;
  }
  imgFile.close();

  time = millis() - time;
  Serial.println("done!");
  Serial.print(time); Serial.println(" ms elapsed");
}

void loop() {
  if (Serial1.available()) {
    int inByte = Serial1.read();
    Serial.write(inByte);
  }

  // read from port 0, send to port 1:
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }
}

void loop1() {
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      takePicture();
      gprsUploadFile();
    }
    delay(50);
  }
  lastButtonState = buttonState;

  
//  if (Serial1.available()) {
//    int inByte = Serial1.read();
//    Serial.write(inByte);
//  }
//
//  // read from port 0, send to port 1:
//  if (Serial.available()) {
//    int inByte = Serial.read();
//    Serial1.write(inByte);
//  }
}

byte initSim900() {
  byte answer = 0;
  int i = 0;
  while ( i < 10 && answer == 0){
    answer = sendATcommand("AT+CREG?","+CREG: 0,1", 1000);
    i++;
    delay(1000);
    if(answer != 0 ) break;
    answer = sendATcommand("AT+CREG?","+CREG: 0,5", 1000);
      i++;
      delay(1000);
    if(answer != 0 ) break;
  }
  /*if (roaming && answer == 0) {
    while (i < 20 && answer == 0){
      answer = sendATcommand("AT+CREG?","+CREG: 0,5", 1000);
      i++;
      delay(1000);
    }
  }*/
  if (answer == 1){answer = sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"","OK", 1000);}
  String com = "AT+SAPBR=3,1,\"APN\",\"" + apn_name + "\"";
  if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);}
  /*if (apn_username!= "") {
    com = "AT+SAPBR=3,1,\"USER\",\"" + apn_username + "\"";
    if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);};
    if (apn_password != "") {
      com = "AT+SAPBR=3,1,\"PWD\",\"" + apn_password + "\"";
      if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);}
    }
  }*/
  if (answer == 1){
    answer = 0;
    i = 0;
    while (i < 3 && answer == 0){
      answer = sendATcommand("AT+SAPBR=1,1", "OK", 10000);
      if (answer == 0){sendATcommand("AT+SAPBR=0,1", "OK", 10000);}
      i++;
    }
  }
  if (answer == 1){answer = sendATcommand("AT+SAPBR=2,1", "OK", 1000);}
  return answer;
}

byte initSim900FTP() {
  byte answer = 0;
  answer = sendATcommand("AT+FTPCID=1", "OK", 1000);
  String com = "AT+FTPSERV=\"" + ftp + "\"";
  if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);}
  if (answer == 1){answer = sendATcommand("AT+FTPPORT=21", "OK", 1000);}
  com = "AT+FTPUN=\"" + ftp_user + "\"";
  if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);}
  com = "AT+FTPPW=\"" + ftp_password + "\"";
  if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);}
  com = "AT+FTPPUTNAME=\"" + String(file_name) + "\"";
  if (answer == 1){answer = sendATcommand(string2char(com), "OK", 1000);}
  if (answer == 1){answer = sendATcommand("AT+FTPPUTPATH=\"/\"", "OK", 1000);}
  return answer;
}

byte Sim900FTPSend (){
  byte answer = 0;
  if (answer = sendATcommand("AT+FTPPUT=1", "+FTPPUT:1,1,", 60000) == 1) {
    int data_size = 0;
    while(Serial1.available()==0);
    char aux = Serial1.read();
    do{
      data_size *= 10;
      data_size += (aux-0x30);
      while(Serial1.available()==0);
      aux = Serial1.read();       
    } while(aux != 0x0D);
    dataFile = SD.open(file_name);
    String XcomA = "";
    String XcomB = "";
    XcomA.concat("AT+FTPPUT=2,");
    XcomA.concat(data_size);
    XcomA.concat("\"");
       
    XcomB.concat("+FTPPUT:2,");
    XcomB.concat(data_size);
    XcomB.concat("\"");
       
    char XxcomA[XcomA.length()];
    char XxcomB[XcomB.length()];
       
    XcomA.toCharArray(XxcomA,XcomA.length());
    XcomB.toCharArray(XxcomB,XcomB.length());
       
    if (dataFile) {
      int archivosize = dataFile.size();
      while(dataFile.available() and answer == 1){
        while(archivosize >= data_size){
          if (answer = sendATcommand(XxcomA,XxcomB,3000) == 1) {
            for(int d = 0; d < data_size; d++){
              Serial1.write(dataFile.read());
              archivosize -= 1;
            }
          }
          else {answer = 0;}
        }
        String ScomA = "";
        String ScomB = "";
        ScomA.concat("AT+FTPPUT=2,");
        ScomA.concat(archivosize);
        ScomA.concat("\"");
               
        ScomB.concat("+FTPPUT:2,");
        ScomB.concat(archivosize);
        ScomB.concat("\"");
               
        char CcomA[ScomA.length()];
        char CcomB[ScomB.length()];
               
        ScomA.toCharArray(CcomA,ScomA.length());
        ScomB.toCharArray(CcomB,ScomB.length());
                       
        if (sendATcommand(CcomA,CcomB,3000) == 1) {
          for(int t = 0; t < archivosize; t++){
            Serial1.write(dataFile.read());
          }
        }
      }
      // close the file:
      dataFile.close();
    }
    delay(500);
    if (sendATcommand("AT+FTPPUT=2,0", "+FTPPUT:1,0", 30000)==1){
      if (debug) {Serial.println("File " + String(file_name) + " uploaded..." );}
    }          
  }
  else {answer = 0;}
  return answer;
}

unsigned char sendATcommand(char* ATcommand, char* expected_answer1, unsigned int timeout) {
  unsigned char x = 0;
  unsigned char answer = 0;
  char response[100];
  unsigned long previous;
  memset(response, '\0', 100);
  delay(100);
  while( Serial1.available() > 0) Serial1.read();
  Serial1.println(ATcommand);
  x = 0;
  previous = millis();
  do{
    if(Serial1.available() != 0){   
      response[x] = Serial1.read();
      x++;
      if (strstr(response, expected_answer1) != NULL) {
        answer = 1;
      }
    }
  } while((answer == 0) && ((millis() - previous) < timeout));
  if (debug) {Serial.println(response);}
  return answer;
}

byte powerUp_Sim900() {
  byte answer = 0;
  int i = 0;
  powerSim900();
  while (i < 2 && answer == 0){
    answer = sendATcommand("AT+CREG?","+CREG:", 1000);
    i++;
    delay(500);
  }
  if (answer == 0) {powerSim900();}
  i = 0;
  while (i < 2 && answer == 0){
    answer = sendATcommand("AT+CREG?","+CREG:", 1000);
    i++;
    delay(500);
  }
  return answer; //send 1 if power up
}

byte powerDown_Sim900() {
  byte answer = 0;
  int i = 0;
  powerSim900();
  while (i < 2 && answer == 0){
    answer = sendATcommand("AT+CREG?","+CREG:", 1000);
    i++;
    delay(500);
  }
  if (answer == 1) {powerSim900();}
  i = 0;
  while (i < 2 && answer == 1){
    answer = sendATcommand("AT+CREG?","+CREG:", 1000);
    i++;
    delay(500);
  }
  return answer; //send 0 if power up
}

void powerSim900() {
  pinMode(Sim900PowerPin, OUTPUT);
  digitalWrite(Sim900PowerPin,LOW);
  delay(1000);
  digitalWrite(Sim900PowerPin,HIGH);
  delay(2000);
  digitalWrite(Sim900PowerPin,LOW);
  delay(5000);
}

char* string2char(String command){
  if(command.length() != 0){
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

byte checkFile() {
  byte answer = 0;
  if(SD.exists(file_name)) {answer = 1;}
  return answer;
}

