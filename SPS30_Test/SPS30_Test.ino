// test code for SPS30

#define SPS_SERIAL Serial1

byte data = 0;
byte SPSChecksum = 0;
byte systemInfo [5] = {0};
int t = 0;
int i = 0;
byte MassC[16] = {0};
byte NumC[20] = {0};
byte AvgS[4] = {0};
byte buffers[40] = {0};
byte checksum = 0;
String dataLog = "";

union mass // define the union for mass concentration
{
  byte MCA[16];
  float MCF[4];
}m;

union num // define the union for number concentration
{
  byte NCA[20];
  float NCF[5];
}n;

union avg
{
  byte ASA[4];
  float ASF;
}a;

void setup() 
{
  delay(100);
  SPS_SERIAL.begin(115200);
  delay(100);
  Serial.begin(115200);
  delay(1000);
  SPS_init(&SPS_SERIAL);

  // Send the "frame" to enter measurment mode
}

void loop() 
{
   if(millis()-t > 1500)                                               //Only read new data every second
  {
    t = millis();

  Serial.println(SPS_Update(&SPS_SERIAL));
  Serial.println("\n");
  }
}

void SPS_init(Stream *s)
{
    SPS_power_on(s);
    delay(100);
    SPS_fanClean(s);
    delay(10500);
Serial.println ("System initialized!");
}

void SPS_power_on(Stream *s)
{
  // send startup frame
  s->write(0x7E);
  s->write((byte)0x00);
  s->write((byte)0x00); // this is the actual command
  s->write(0x02);
  s->write(0x01);
  s->write(0x03);
  s->write(0xF9);
  s->write(0x7E);

  // read the response bytes
  delay(100);
  
  for (unsigned int q = 0; q<7; q++) data = s->read();
}

void SPS_power_off(Stream *s)
{
  // send shutdown frame
  s->write(0x7E);
  s->write((byte)0x00);
  s->write(0x01);       // this is the actual command
  s->write((byte)0x00);
  s->write(0xFE);
  s->write(0x7E);

  // read the response bytes
  delay(100);
  
  for (unsigned int q = 0; q<7; q++) data = s->read();
}

void SPS_fanClean(Stream *s)
{
  // send clean frame
  s->write(0x7E);
  s->write((byte)0x00);
  s->write(0x56);       // this is the actual command
  s->write((byte)0x00);
  s->write(0xA9);
  s->write(0x7E);

  // read the response bytes
  delay(100);
  
  for (unsigned int q = 0; q<7; q++) data = s->read();
}

bool SPS_read_data(Stream *s)
{ 
  s->write(0x7E);
  s->write((byte)0x00);
  s->write(0x03); // this is the actual command
  s->write((byte)0x00);
  s->write(0xFC);
  s->write(0x7E);

 if (! s->available()){
      Serial.println ("Failed reading: serial not available!");
    return false;
  }

   if (s->peek() != 0x7E){
     Serial.println ("Failed reading: bad startbyte!");
     for (unsigned short j = 0; j<60; j++){
        data = s->read();
     }
     data = 0;
    return false;
   }
   

  // need to read in the 40 bytes from the response

  if (s->available() < 47){
    Serial.println ("Failed reading: not enough bytes!");
    return false;
  }

    for(unsigned short j = 0; j<5; j++) // info like start bit, command and length
    {
        systemInfo[j] = s->read();
        if (j != 0) checksum += systemInfo[j];  
        Serial.print(systemInfo[j], HEX);
        Serial.print("     ");
    }
    Serial.println();
   
   if (systemInfo[3] != (byte)0x00){
     Serial.println ("Failed reading: system failure!");
     for (unsigned short j = 0; j<60; j++){
        data = s->read();
     }
     data = 0;
    return false;
   }

byte stuffByte = 0;
  for(unsigned short buffIndex = 0; buffIndex < 40; buffIndex++){
    buffers[buffIndex] = s->read();
      if (buffers[buffIndex] == 0x7D) {
      stuffByte = s->read();
      Serial.println("Stuffed!");
      if (stuffByte == 0x5E) buffers[buffIndex] = 0x7E;
      if (stuffByte == 0x5D) buffers[buffIndex] = 0x7D;
      if (stuffByte == 0x31) buffers[buffIndex] = 0x11;
      if (stuffByte == 0x33) buffers[buffIndex] = 0x13;
    }
    checksum += buffers[buffIndex];
  }

    SPSChecksum = s->read();
    data = s->read(); 

    if (data != 0x7E){
       Serial.println ("Failed reading: bad end byte!");
       for (unsigned short j = 0; j<60; j++){
         data = s->read();
       }
       data = 0;
      
     return false;
    }

   Serial.println (SPSChecksum,HEX);
   Serial.println(data,HEX);

    data = 0;

    for(unsigned short j = 0; j<16; j++) // mass concentration
    {

        MassC[j] = buffers[j];
    }
    
    for(unsigned short j = 0; j<20; j++) // number concetration
    {
        NumC[j] = buffers[j+16];
    }

    for (unsigned short j=0; j<4; j++)
    {
        AvgS[j] = buffers[j+36];
    }

    checksum = checksum & 0xFF;
    checksum = ~checksum;
    Serial.println(checksum,HEX);

    if (checksum != SPSChecksum){
      Serial.println ("Failed reading: bad checksum!");
     for (unsigned short j = 0; j<60; j++){
        data = s->read();
     }
      data = 0;
      checksum = 0;
      return false;
    }
    
   checksum = 0;

  Serial.println ("Successful reading!");
  return true;
}

String SPS_Update(Stream *s){
    String dataLogLocal = "";   
    if (SPS_read_data(s)){                                                  //Send the frame to read data

//Flip order of every four bytes
unsigned short flip = 0;                                              //index of array to flip
unsigned short result = 0;                                           //index of array that will be the result

for (unsigned short flipMax = 4; flipMax<17; flipMax+=4){             //For loop that will loop through the flipping mechanism
  
  result = flipMax - 1;                                               //The result starts one lower than the flipMax because of counting from zero
    for (flip; flip<flipMax; flip++){                                 //Flipping mechanism
       m.MCA[flip] = MassC[result];                                   //flip mass
       n.NCA[flip] = NumC[result];                                    //flip number count
    result--;
   }
}

 unsigned short p = 19;
 for (unsigned short q = 16; q<20; q++){                              //number has an extra four bytes. Same process as above.
  n.NCA[q] = NumC[p];
  p--;
 }
p = 3;
 for (unsigned short q = 0; q<4; q++){                              //number has an extra four bytes. Same process as above.
  a.ASA[q] = AvgS[p];
  p--;
 }

   // convert to float and print or save...
//   Serial.println();
//   Serial.print("Mass Concentration: ");
   for(unsigned short k = 0; k<4; k++)
   {
      if (k==0) {
         dataLogLocal += String(m.MCF[k]) + ',';
        } else {
         dataLogLocal += String(m.MCF[k]-m.MCF[k-1]) + ',';
        }
   }
//   Serial.println();
//   Serial.print("Number Concentration: ");
   for(unsigned short k = 0; k<5; k++)
   {
      if (k==0) {
        dataLogLocal += String(n.NCF[k]) + ',';
      } else {
        dataLogLocal += String(n.NCF[k]-n.NCF[k-1]) + ',';
        Serial.println (n.NCF[k]-n.NCF[k-1]);       
      }
   }
   
      dataLogLocal += String(a.ASF);
      
  return dataLogLocal;
} else {
  return "-,-,-,-,-,-,-,-,-,-";
}
}
