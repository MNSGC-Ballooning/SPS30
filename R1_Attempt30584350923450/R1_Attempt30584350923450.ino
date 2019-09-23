#include <SPI.h>

const byte CS = 15;
byte raw[64] = {0};
float data[30] = {0};
unsigned short loopy = 0;

union{
  byte byteInput[4];
  float floatOutput;
}sfr,sp,a,b,c;

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  SPI.begin();
  pinMode(CS,OUTPUT);
  digitalWrite(CS,HIGH);
  delay (2500);
  Serial.println("Attempting to power on...");
  turnOn();
  Serial.println("Power on successful!");
  delay(5000);
  Serial.println("Beginning measurements...");
}

void loop() {
  Serial.println("Attempting data collection...");
  digitalWrite(CS,LOW);
  SPI.beginTransaction(SPISettings(700000, MSBFIRST, SPI_MODE1));

  byte test = 0x00;
  
  do{
  delay(10);
  test = SPI.transfer(0x30);
  Serial.println(test,HEX);
  loopy++;
  
  if (loopy > 20){
    digitalWrite(CS, HIGH);                                          
    SPI.endTransaction();
    delay(1000);
    loopy = 0;
    SPI.beginTransaction(SPISettings(700000, MSBFIRST, SPI_MODE1));
    digitalWrite(CS,LOW);
  }
  } while (test != 0xF3);

  Serial.println();
  
  for (unsigned short i = 0; i < 64; i++){
    delayMicroseconds(20);
    raw[i] = SPI.transfer(0x30);
    Serial.print(raw[i],HEX);
    Serial.print(" ");
  }
  digitalWrite(CS,HIGH);
  SPI.endTransaction();
  
  Serial.println();
  
  for (unsigned short x=0; x<16; x++){
    data[x] = bytes2int(raw[(x*2)], raw[(x*2+1)]);
   }

  for (int x = 16; x<20; x++){
    data[x] = raw[x+16];
  }

  for (unsigned short x = 0; x<4; x++){
    sfr.byteInput[x] = raw[x+36];
  }
  data[20] = sfr.floatOutput;

  data[21] = bytes2int(raw[40], raw[41]);
  data[22] = bytes2int(raw[42], raw[43]);

  for (unsigned short x = 0; x<4; x++){
    sp.byteInput[x] = raw[x+44];
  }
  data[23] = sp.floatOutput;

  data[24] = raw[48];
  data[25] = raw[49];

  for (unsigned short x = 0; x<4; x++){
    a.byteInput[x] = raw[x+50];
  }
  data[26] = a.floatOutput;

  for (unsigned short x = 0; x<4; x++){
    b.byteInput[x] = raw[x+54];
  }
  data[27] = b.floatOutput;

  for (unsigned short x = 0; x<4; x++){
    c.byteInput[x] = raw[x+58];
  }
  data[28] = c.floatOutput;

  data[29] = bytes2int(raw[62],raw[63]);

  Serial.println();
  for (unsigned short x = 0; x < 30; x++){
    Serial.print(data[x]);
    Serial.print(" "); 
  }
  Serial.print(String(CalcCRC(raw,62)));
  
  Serial.println();

  delay(5000);
}

void turnOn(){
  byte inData[3] = {0}; 
  int loopy = 0; 
  
  Serial.println("Starting...");
  digitalWrite(CS,LOW);
  SPI.beginTransaction(SPISettings(700000, MSBFIRST, SPI_MODE1));
 
  do{
  inData[0] = SPI.transfer(0x03);
  Serial.println(inData[0],HEX);
  delay(10);
  loopy++;
  
  if (loopy > 20){
    digitalWrite(CS, HIGH);                                          
    SPI.endTransaction();
    delay(1000);
    loopy = 0;
    SPI.beginTransaction(SPISettings(700000, MSBFIRST, SPI_MODE1));
    digitalWrite(CS,LOW);
  }
  } while (inData[0] != 0xF3);

  inData[1] = SPI.transfer(0x03);
  Serial.println(inData[1],HEX);
  delay(10);
  inData[2] = SPI.transfer(0x01);
  Serial.println(inData[2],HEX);
  delay (10);

  digitalWrite(CS, HIGH);                                          
  SPI.endTransaction();
  
  if (inData[1] != 0x03){
    Serial.println("Init failure! Trying again...");
    delay(5000);
    turnOn();
  }
}

void turnOff(){
  byte inData[3] = {0}; 
  int loopy = 0; 
  
  Serial.println("Starting...");
  digitalWrite(CS,LOW);
  SPI.beginTransaction(SPISettings(700000, MSBFIRST, SPI_MODE1));
 
  do{
  inData[0] = SPI.transfer(0x03);
  Serial.println(inData[0],HEX);
  delay(10);
  loopy++;
  
  if (loopy > 20){
    digitalWrite(CS, HIGH);                                          
    SPI.endTransaction();
    delay(1000);
    loopy = 0;
    SPI.beginTransaction(SPISettings(700000, MSBFIRST, SPI_MODE1));
    digitalWrite(CS,LOW);
  }
  } while (inData[0] != 0xF3);

  inData[1] = SPI.transfer(0x00);
  Serial.println(inData[1],HEX);
  delay(10);
  inData[2] = SPI.transfer(0x00);
  Serial.println(inData[2],HEX);
  delay (10);

  digitalWrite(CS, HIGH);                                          
  SPI.endTransaction();
  
  if (inData[1] != 0x03){
    Serial.println("Init failure! Trying again...");
    delay(5000);
    turnOn();
  }
}

uint16_t bytes2int(byte LSB, byte MSB)
{
  uint16_t val = ((MSB << 8) | LSB);
  return val;
}

unsigned int CalcCRC(unsigned char data[], unsigned char nbrOfBytes) {
    #define POLYNOMIAL 0xA001 //Generator polynomial for CRC
    #define InitCRCval 0xFFFF //Initial CRC value
    unsigned char _bit; // bit mask
    unsigned int crc = InitCRCval; // initialise calculated checksum 
    unsigned char byteCtr; // byte counter
    // calculates 16-Bit checksum with given polynomial
    
    for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
      crc ^= (unsigned int)data[byteCtr]; 
      for(_bit = 0; _bit < 8; _bit++) {
        if (crc & 1) //if bit0 of crc is 1
        {
            crc >>= 1;
            crc ^= POLYNOMIAL; 
        } else crc >>= 1;
      }
    }
    return crc; 
}
