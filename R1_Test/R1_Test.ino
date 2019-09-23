#include <SPI.h>

const uint8_t SSpin = 15;
byte test = 0;
int i = 0;
uint16_t checksumR1 = 0;
//unsigned int checksumCalc = 0;

byte raw[64] = {0};
uint16_t com[16] = {0};

void setup() 
{
  pinMode(SSpin,OUTPUT);
  Serial.begin(9600);
  SPI.begin();         // intialize SPI in Arduino
  delay(5000);
  Serial.println("Starting up");
  OPCR1_on();
  delay(5000); // delay to allow fans to reach operating speed
}

void loop()
{
  if(i==10)
  {
    Serial.println("Shutting down");
    delay(10);
    OPCR1_off();
    exit(0);
  }
  Serial.println("Getting data hopefully");
  SPI.beginTransaction(SPISettings(750000, MSBFIRST, SPI_MODE1));
  digitalWrite(SSpin, LOW);     
  test = SPI.transfer(0x30);
  Serial.print("test byte: ");
  Serial.println(test,HEX);
  delay(10);
  test = SPI.transfer(0x30);
  Serial.print("test byte: ");
  Serial.println(test,HEX);
  
  delayMicroseconds(20);
  
  Serial.print("raw: ");
  for(int i = 0; i<64; i++)
  {
    raw[i] = SPI.transfer(0x30);
    Serial.print(raw[i]);
    Serial.print(" ");
    delayMicroseconds(20);
  }
 
  SPI.endTransaction();
  
  Serial.println();

  for (int x=0; x<16; x++){
    com[x] = bytes2int(raw[(x*2)], raw[(x*2+1)]);
  }
  
 
//  com[0] = bytes2int(raw[0],raw[1]);
//  com[1] = bytes2int(raw[2],raw[3]);
//  com[2] = bytes2int(raw[4],raw[5]);
//  com[3] = bytes2int(raw[6],raw[7]);
//  com[4] = bytes2int(raw[8],raw[9]);
//  com[5] = bytes2int(raw[10],raw[11]);
//  com[6] = bytes2int(raw[12],raw[13]);
//  com[7] = bytes2int(raw[14],raw[15]);
//  com[8] = bytes2int(raw[16],raw[17]);
//  com[9] = bytes2int(raw[18],raw[19]);
//  com[10] = bytes2int(raw[20],raw[21]);
//  com[11] = bytes2int(raw[22],raw[23]);
//  com[12] = bytes2int(raw[24],raw[25]);
//  com[13] = bytes2int(raw[26],raw[27]);
//  com[14] = bytes2int(raw[28],raw[29]);
//  com[15] = bytes2int(raw[30],raw[31]);

  Serial.print("Data: ");
  for(int i = 0; i<15; i++)
  {
    Serial.print(com[i]);
    Serial.print(" ");
  }
  Serial.println();

//  Serial.print("ChecksumR1: ");
//  checksumR1 = bytes2int(raw[62],raw[63]);
//  Serial.println(checksumR1);
//
//  Serial.print("ChecksumCalc: ");
//  checksumCalc = CalcCRC(raw,62);
//  Serial.println(checksumCalc);

 // delay(5000);

//  i++;
  
}


void OPCR1_on()
{
  byte inData[3] = {0};

  SPI.beginTransaction(SPISettings(750000, MSBFIRST, SPI_MODE1));  
  digitalWrite(SSpin, LOW);                                           
  inData[0] = SPI.transfer(0x03);                               
  delay(10);                                                          
  inData[1] = SPI.transfer(0x03);                               
  delay(10);
  inData[2] = SPI.transfer(0x03);
  delay(10);
  digitalWrite(SSpin, HIGH);                                          
  SPI.endTransaction();
  Serial.println(inData[0],HEX);
  Serial.println(inData[1],HEX);
  Serial.println(inData[2],HEX);
  if(inData[0] != 0x31 || inData[1] != 0xF3 || inData[2] != 0x03)
  {
    Serial.println("wrong bytes, trying again");
    delay(5000);
    OPCR1_on();
  }
}

void OPCR1_off()
{
  byte inData[3] = {0};
  
  SPI.beginTransaction(SPISettings(750000, MSBFIRST, SPI_MODE1));
  digitalWrite(SSpin, LOW);                                           
  inData[0] = SPI.transfer(0x03);                              
  delay(10);                                                         
  inData[1] = SPI.transfer(0x03);                              
  delay(10);
  inData[2] = SPI.transfer(0x00);
  delay(10);
  digitalWrite(SSpin, HIGH);                                          
  SPI.endTransaction();
  Serial.println(inData[0],HEX);
  Serial.println(inData[1],HEX);
  Serial.println(inData[2],HEX);
}

uint16_t bytes2int(byte LSB, byte MSB)
{
  uint16_t val = ((MSB << 8) | LSB);
  return val;
}

/*
unsigned int CalcCRC(unsigned char data[], unsigned char nbrOfBytes) 
{   
  #define POLYNOMIAL 0xA001 //Generator polynomial for CRC   
  #define InitCRCval 0xFFFF //Initial CRC value 
 
  unsigned char _bit; // bit mask   
  unsigned int crc = InitCRCval; // initialise calculated checksum   
  unsigned char byteCtr; // byte counter 
 
  // calculates 16-Bit checksum with given polynomial   
  for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++)   
  {     
    crc ^= (unsigned int)data[byteCtr];     
    for(_bit = 0; _bit < 8; _bit++)     
    {       
      if (crc & 1) //if bit0 of crc is 1       
      {         
        crc >>= 1;         
        crc ^= POLYNOMIAL;       
      } 
      else  crc >>= 1;
    }  
       return crc; 
  }
}
*/
