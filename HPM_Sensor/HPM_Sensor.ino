#define HPM_SERIAL Serial1
#define HPM_LOOP 1000

unsigned long prevTime = 0;

struct HPMdata{
 uint16_t PM1_0, PM2_5, PM4_0, PM10_0, checksum, checksumR;
}localData;

unsigned short nTot = 0;

void setup() {
Serial.begin(9600);
while (!Serial) ;

HPM_SERIAL.begin(9600);
initHPM();

}

void loop() {
  if((millis()-prevTime)>=HPM_LOOP){
    prevTime = millis();


    
  }

}

void initHPM(){
  powerON();
  delay(5000);
}

void powerON(){
  byte checkIt;
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x96);

  delay(100);
  checkIt = HPM_SERIAL.read();
  if (checkIt == 0xA5A5) return;

  delay(100);
  powerON();
}

void powerOFF(){
  byte checkIt;
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x02);
  HPM_SERIAL.write(0x95);

  delay(100);
  checkIt = HPM_SERIAL.read();
  if (checkIt == 0xA5A5) return;

  delay(100);
  powerOFF();
}

bool readDataHPM(){
  byte inputArray[32] = {0};
  
  HPM_SERIAL.write(0x68);
  HPM_SERIAL.write(0x01);
  HPM_SERIAL.write(0x04);
  HPM_SERIAL.write(0x93);

  if (!HPM_SERIAL.available()){
    return false;
  }

  if (HPM_SERIAL.peek() != 0x42){
    for (unsigned short i = 0; i<32; i++) inputArray[i] = HPM_SERIAL.read();
    return false;
  }

  if (HPM_SERIAL.available() < 32){
    return false;
  }

  for (int i = 0; i<32; i++){
    inputArray[i] = HPM_SERIAL.read();
    localData.checksum += inputArray[i];
  }

  localData.checksumR = bytes2int(inputArray[31],inputArray[30]);

  if (localData.checksum != localData.checksumR){
    return false;
  }

  localData.PM1_0 = bytes2int(inputArray[5],inputArray[4]);
  localData.PM2_5 = bytes2int(inputArray[7],inputArray[6]);
  localData.PM4_0 = bytes2int(inputArray[9],inputArray[8]);
  localData.PM10_0 = bytes2int(inputArray[11],inputArray[10]);

  return true;
}

String logUpdateHPM(){
  String localDataLog = nTot + ',';
  
  if (readDataHPM()){
    nTot++;

    localDataLog += localData.PM1_0 + ',' + localData.PM2_5 + ',' + localData.PM4_0 + ',' + localData.PM10_0;
  } else {
    localDataLog += "-,-,-,-";
  }

  return localDataLog;
}
 
 uint16_t bytes2int(byte LSB, byte MSB)
{
  uint16_t val = ((MSB << 8) | LSB);
  return val;
}
