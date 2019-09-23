#define HPM_SERIAL Serial1
#define HPM_LOOP 6000

unsigned long prevTime = 0;

struct HPMdata{
 uint16_t PM1_0, PM2_5, PM4_0, PM10_0, checksum, checksumR;
}localData;

unsigned short nTot = 0;
bool autoSend = true;
bool goodLog = false;
unsigned short badLog = 0;
unsigned short counter = 0;

void setup() {
Serial.begin(9600);
while (!Serial) ;

HPM_SERIAL.begin(9600);
Serial.println("Serial connected!");

initHPM();
autoSendOff();
Serial.println("System on!");
}

void loop() {

  if((millis()-prevTime)>=HPM_LOOP){
    prevTime = millis();

    Serial.println();
    Serial.println();
    Serial.println(logUpdateHPM());
    Serial.println();
    Serial.println();
  }

}

void initHPM(){
  powerON();
  delay(5000);
}

void powerON(){
  counter++;
  Serial.println(String(counter));
  byte checkIt[2] = {0};
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x96);

  delay(20);
  checkIt[0] = HPM_SERIAL.read();
  checkIt[1] = HPM_SERIAL.read();
  if ((checkIt[0] == 0xA5)&&(checkIt[1] == 0xA5)) return;

  delay(100);
  powerON();
}

void powerOFF(){
  byte checkIt[2] = {0};
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x02);
  HPM_SERIAL.write(0x95);

  delay(20);
  checkIt[0] = HPM_SERIAL.read();
  checkIt[1] = HPM_SERIAL.read();
  if ((checkIt[0] == 0xA5)&&(checkIt[1] == 0xA5)) return;

  delay(100);
  powerOFF();
}

void autoSendOn(){
  byte checkIt[2] = {0};
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x40);
  HPM_SERIAL.write(0x57);

  delay(20);
  checkIt[0] = HPM_SERIAL.read();
  checkIt[1] = HPM_SERIAL.read();
  if ((checkIt[0] == 0xA5)&&(checkIt[1] == 0xA5)){
    autoSend = true;
    return;
  }

  delay(100);
  autoSendOn();
}

void autoSendOff(){
  byte checkIt[2] = {0};
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x20);
  HPM_SERIAL.write(0x77);

  delay(20);
  checkIt[0] = HPM_SERIAL.read();
  checkIt[1] = HPM_SERIAL.read();
  if ((checkIt[0] == 0xA5)&&(checkIt[1] == 0xA5)){
    autoSend = false;
    return;
  }

  delay(100);
  autoSendOff();
}

bool readDataHPM(){
//If badLog and recieving start bytes for other type, perform auto switch??
  
  if (autoSend){
    Serial.println("Auto send!");
    byte inputArray[32] = {0};
  
    if (!HPM_SERIAL.available()){
      Serial.println("Serial not available!");
      return false;
    }
  
    if (HPM_SERIAL.peek() != 0x42){
      Serial.println(HPM_SERIAL.read());
      Serial.println("Bad start byte!");
      return false;
    }
  
    if (HPM_SERIAL.available() < 32){
      Serial.println("Not enough bytes!");
      return false;
    }
    localData.checksum = 0;
    for (int i = 0; i<32; i++){
      inputArray[i] = HPM_SERIAL.read();
      Serial.println();
      Serial.print(inputArray[i], HEX);
      if (i<30) localData.checksum += inputArray[i];
    }
  
    localData.checksumR = bytes2int(inputArray[31],inputArray[30]);
    Serial.println();
    Serial.println(String(localData.checksum) + "," + String(localData.checksumR));
    Serial.println();
   if (localData.checksum != localData.checksumR){
     Serial.println("Checksum Failure!");
     return false;
   }

   localData.PM1_0 = bytes2int(inputArray[5],inputArray[4]);
   localData.PM2_5 = bytes2int(inputArray[7],inputArray[6]);
   localData.PM4_0 = bytes2int(inputArray[9],inputArray[8]);
   localData.PM10_0 = bytes2int(inputArray[11],inputArray[10]);

   return true;
  } else {
   Serial.println("Data request!");
   byte head;
   byte len;
   byte cmd;

   for (unsigned short i = 0; i<32; i++) HPM_SERIAL.read();

   HPM_SERIAL.write(0x68);
   HPM_SERIAL.write(0x01);
   HPM_SERIAL.write(0x04);
   HPM_SERIAL.write(0x93);

   delay(50);
   
   if (!HPM_SERIAL.available()){
     Serial.println("Serial not available!");
     return false;
   }

   if (HPM_SERIAL.peek() == 0x96){
      HPM_SERIAL.read();
      HPM_SERIAL.read();
      Serial.println("Failure bytes!");
      return false;
   }

    head = HPM_SERIAL.read();
    len = HPM_SERIAL.read();
    cmd = HPM_SERIAL.read();

    Serial.println(head, HEX);
    Serial.println(len, HEX);
    Serial.println(cmd, HEX);

   if (head != 0x40){
     Serial.println("Bad start byte!");
     return false;
   }  

    if (HPM_SERIAL.available()<(len)){
     Serial.println("Not enough bytes!"); 
     return false;
   }

   if (cmd != 0x04){
     Serial.println("Command error!");
     return false;
   }

   uint16_t *inputArray = new uint16_t[len];
   unsigned short i = 0;

   while ((i<len)||(HPM_SERIAL.available())){
     inputArray[i] = HPM_SERIAL.read();
     Serial.println(inputArray[i], HEX);
     i++;
   }

   localData.checksum = 65536 - (head + len + cmd);
   for (unsigned short i = 0; i<len-1; i++) localData.checksum -= inputArray[i];
   localData.checksum = localData.checksum % 256;
   localData.checksumR = inputArray[(len-1)];
  
   Serial.println(String(localData.checksum) + "," + String(localData.checksumR));
   if (localData.checksum != localData.checksumR){
     Serial.println("Bad checksum!");
     return false;
   }

   localData.PM1_0 = inputArray[0]*256 + inputArray[1];
   localData.PM2_5 = inputArray[2]*256 + inputArray[3];
   localData.PM4_0 = inputArray[4]*256 + inputArray[5];
   localData.PM10_0 = inputArray[6]*256 + inputArray[7];
  
   delete [] inputArray;
   Serial.println("Good read!");
   return true;
  }
}

String logUpdateHPM(){
  String localDataLog = String(nTot) + ",";
  
  if (readDataHPM()){
    nTot++;
    goodLog = true;
    badLog = 0;

    localDataLog += String(localData.PM1_0) + "," + String(localData.PM2_5) + "," + String(localData.PM4_0) + "," + String(localData.PM10_0);
  } else {
    localDataLog += "-,-,-,-";
    badLog++;
    if (badLog >= 5) goodLog = false;
  }

  return localDataLog;
}
 
 uint16_t bytes2int(byte LSB, byte MSB)
{
  uint16_t val = ((MSB << 8) | LSB);
  return val;
}
