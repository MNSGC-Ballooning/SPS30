//SPS30 Code

//University of Minnesota - Candler MURI Project
//July 2019

//This code is derived from the datasheet for the SPS30 particle detector.
//The code will clean the system, parse data, and run startup and shutdown 
//operations. The code is tested on a Teensy 3.5/3.6 system. The SPS system
//operates using "frames" to send chunks of commands and data back and
//forth. These frames are made of a series of bytes that will always be
//sent together.

//The SPS30 system runs on 5V and draws .05A. The system can provide new data
//once every second.

#define SPS_SERIAL Serial1                                                      //Serial Pin Macro
#define SPS_LOOP 1500

int t = 0;                                                                      //Timer variable for the SPS30 loop. The SPS30 must read data no faster than 1Hz.

byte buffers[40] = {0}, systemInfo [5] = {0}, MassC[16] = {0};                  //Byte variables for collection and organization of data from the SPS30.
byte NumC[20] = {0}, AvgS[4] = {0}, data = 0, checksum = 0, SPSChecksum = 0;
bool goodLog = false;
int badLog = 0;

union mass                                                                      //Defines the union for mass concentration
{
  byte MCA[16];
  float MCF[4];
}m;

union num                                                                       //Defines the union for number concentration
{
  byte NCA[20];
  float NCF[5];
}n;

union avg                                                                       //Defines the union for average sizes
{
  byte ASA[4];
  float ASF;
}a;

void setup() 
{
  delay(100);
  SPS_SERIAL.begin(115200);                                                     //Serial initializations for SPS and computer.
  delay(100);                                                                   //SPS serial is critical to operation of the sensor.
  Serial.begin(115200);                                                         //The delays give time for intializations of the serial.
  delay(100);
  SPS_init(&SPS_SERIAL);                                                        //The SPS30 Initialization command will boot and clean the sensor.
}

void loop() 
{
   if(millis()-t > SPS_LOOP)                                                    //Loop Timer
  {
    t = millis();

  Serial.println(SPS_Update(&SPS_SERIAL));                                      //Serial print that runs the SPS Update. SPS_Update returns a data string.
  Serial.println("\n");
  }
}

void SPS_init(Stream *s)                                                        //SPS initialization code. Requires input of SPS serial stream.
{
    SPS_power_on(s);                                                            //Sends SPS active measurement command
    delay(100);
    SPS_fanClean(s);                                                            //Sends fan clean command. This takes 10 seconds. Then, an additional 10 seconds
    delay(20500);                                                               //are taken to get a clean, consistent flow through the system.
Serial.println ("System initialized!");
}

void SPS_power_on(Stream *s)                                                    //SPS Power on command. This sends and recieves the power on frame
{
  s->write(0x7E);                                                               //Send startup frame
  s->write((byte)0x00);
  s->write((byte)0x00);                                                         //This is the actual command
  s->write(0x02);
  s->write(0x01);
  s->write(0x03);
  s->write(0xF9);
  s->write(0x7E);

  delay (100);
  for (unsigned int q = 0; q<7; q++) data = s->read();                          //Read the response bytes
  data = 0;
}

void SPS_power_off(Stream *s)                                                   //SPS Power off command. This sends and recieves the power off frame
{
  s->write(0x7E);                                                               //Send shutdown frame
  s->write((byte)0x00);
  s->write(0x01);                                                               //This is the actual command
  s->write((byte)0x00);
  s->write(0xFE);
  s->write(0x7E);

  delay(100);
  for (unsigned int q = 0; q<7; q++) data = s->read();                          //Read the response bytes
  data = 0;
}

void SPS_fanClean(Stream *s)                                                    //SPS Power off command. This sends and recieves the power off frame
{
  s->write(0x7E);                                                               //Send clean frame
  s->write((byte)0x00);
  s->write(0x56);                                                               //This is the actual command
  s->write((byte)0x00);
  s->write(0xA9);
  s->write(0x7E);

  delay(100); 
  for (unsigned int q = 0; q<7; q++) data = s->read();                          //Read the response bytes
  data = 0;
}

bool SPS_read_data(Stream *s){                                                  //SPS data request. The system will pull data and ensure the accuracy.                                                                   
  s->write(0x7E);                                                               //The read data function will return true if the data request is successful.
  s->write((byte)0x00);
  s->write(0x03);                                                               //This is the actual command
  s->write((byte)0x00);
  s->write(0xFC);
  s->write(0x7E);

 if (! s->available()){                                                         //If the given serial connection is not available, the data request will fail.
      Serial.println ("Failed reading: serial not available!");
    return false;
  }

   if (s->peek() != 0x7E){                                                      //If the sent start byte is not as expected, the data request will fail.
     Serial.println ("Failed reading: bad start byte!");
     for (unsigned short j = 0; j<60; j++) data = s->read();                    //The data buffer will be wiped to ensure the next data pull isn't corrupt.
     data = 0;
    return false;
   }

  if (s->available() < 47){                                                     //If there are not enough data bytes available, the data request will fail. This
    Serial.println ("Failed reading: not enough bytes!");
    return false;                                                               //will not clear the data buffer, because the system is still trying to fill it.
  }

    for(unsigned short j = 0; j<5; j++){                                        //This will populate the system information array with the data returned by the                  
        systemInfo[j] = s->read();                                              //by the system about the request. This is not the actual data, but will provide
        if (j != 0) checksum += systemInfo[j];                                  //information about the data. The information is also added to the checksum.
        Serial.print(systemInfo[j], HEX);
        Serial.print("     ");
    }
    Serial.println();
   
   if (systemInfo[3] != (byte)0x00){                                            //If the system indicates a malfunction of any kind, the data request will fail.
     Serial.println ("Failed reading: system failure!");
     for (unsigned short j = 0; j<60; j++) data = s->read();                    //Any data that populates the main array will be thrown out to prevent future corruption.
     data = 0;
    return false;
   }

byte stuffByte = 0;
  for(unsigned short buffIndex = 0; buffIndex < 40; buffIndex++){               //This loop will populate the buffer with the data bytes.
    buffers[buffIndex] = s->read();
      if (buffers[buffIndex] == 0x7D) {                                         //This hex indicates that byte stuffing has occurred. The
        stuffByte = s->read();                                                  //series of if statements will determine the original value
        Serial.println("Stuffed!");                                             //based on the following hex and replace the data.
        if (stuffByte == 0x5E) buffers[buffIndex] = 0x7E;
        if (stuffByte == 0x5D) buffers[buffIndex] = 0x7D;
        if (stuffByte == 0x31) buffers[buffIndex] = 0x11;
        if (stuffByte == 0x33) buffers[buffIndex] = 0x13;
    }
    checksum += buffers[buffIndex];                                             //The data is added to the checksum.
  }

    SPSChecksum = s->read();                                                    //The provided checksum byte is read.
    data = s->read();                                                           //The end byte of the data is read.

    if (data != 0x7E){                                                          //If the end byte is bad, the data request will fail.
       Serial.println ("Failed reading: bad end byte!");
       for (unsigned short j = 0; j<60; j++) data = s->read();                  //At this point, there likely isn't data to throw out. However,
       data = 0;                                                                //The removal is completed as a redundant measure to prevent corruption.
       return false;
    }

   Serial.println (SPSChecksum,HEX);
   Serial.println(data,HEX);

    checksum = checksum & 0xFF;                                                 //The local checksum is calculated here. The LSB is taken by the first line.
    checksum = ~checksum;                                                       //The bit is inverted by the second line.
    Serial.println(checksum,HEX);

    if (checksum != SPSChecksum){                                               //If the checksums are not equal, the data request will fail.  
      Serial.println ("Failed reading: bad checksum!");
      for (unsigned short j = 0; j<60; j++) data = s->read();                   //Just to be certain, any remaining data is thrown out to prevent corruption.
      data = 0;
      checksum = 0;
      return false;
    }
    
   checksum = 0;
   data = 0;

    for(unsigned short j = 0; j<16; j++) MassC[j] = buffers[j];                 //The mass concentration data is removed from the buffer.
    for(unsigned short j = 0; j<20; j++) NumC[j] = buffers[j+16];               //The number concentration data is removed from the buffer.
    for (unsigned short j=0; j<4; j++) AvgS[j] = buffers[j+36];                 //The average size data is removed from the buffer.

  Serial.println ("Successful reading!");
  return true;                                                                  //If the reading is successful, the function will return true.
}

String SPS_Update(Stream *s){                                                   //This function will parse the data and form loggable strings.
    String dataLogLocal = "";   
    if (SPS_read_data(s)){                                                      //Read the data and determine the read success.
       goodLog = true;                                                          //The data is sent in reverse. This will flip the order of every four bytes
       badLog = 0;
       
unsigned short flip = 0;                                                        //Index of array to flip
unsigned short result = 0;                                                      //Index of array that will be the result

for (unsigned short flipMax = 4; flipMax<21; flipMax+=4){                       //This will loop through the main flipping mechanism
  result = flipMax - 1;                                                         //The result starts one lower than the flipMax because of counting from zero
    for (flip; flip<flipMax; flip++){                                           //Flipping mechanism. This flips the results of the data request.
      if (flipMax < 5)  a.ASA[flip] = AvgS[result];                             //Flips average size. Average size only has four bytes.
      if (flipMax < 17) m.MCA[flip] = MassC[result];                            //Flips mass. Mass has four less bytes than number.
       n.NCA[flip] = NumC[result];                                              //Flips number count.
    result--;
   }
}

   for(unsigned short k = 0; k<4; k++){                                         //This loop will populate the data string with mass concentrations.
      if (k==0) {
         dataLogLocal += String(m.MCF[k]) + ',';                                //Each bin from the sensor includes all of the particles from the bin
        } else {                                                                //below it. This will show the number of particles that reside invidually
         dataLogLocal += String(m.MCF[k]-m.MCF[k-1]) + ',';                     //in each of the four bins.
        }
   }

   for(unsigned short k = 0; k<5; k++){                                         //This loop will populate the data string with number concentrations.
      if (k==0) {
        dataLogLocal += String(n.NCF[k]) + ',';
      } else {                                          
        dataLogLocal += String(n.NCF[k]-n.NCF[k-1]) + ',';                      //These bins are compiled in the same manner as the mass bins.     
      }
   }
    dataLogLocal += String(a.ASF);                                              //This adds the average particle size to the end of the bin.
      
   return dataLogLocal;
  } else {
   return "-,-,-,-,-,-,-,-,-,-";                                                //If there is bad data, the string is populated with failure symbols.
  badLog ++;
  if (badLog >= 5){
    goodLog = false;
  }
  }
}
