// test code for SPS30

byte data = 0;
int t = 0;
int i = 0;
byte MassC[16] = {0};
byte NumC[20] = {0};
byte checksum = 0;

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

void setup() 
{
  Serial1.begin(115200);
  delay(100);
  Serial.begin(115200);
  delay(100);

  // Send the "frame" to enter measurment mode
  SPS_power_on();
}

void loop() 
{
  if(millis()-t > 1500) // only read new data every second
  {
    t = millis();
    // Send the frame to read data
    SPS_read_data();

    checksum = checksum - (0x7E) - (0x7E);
    checksum = checksum & 15;
    checksum = ~checksum;
    Serial.print("Checksum: ");
    Serial.println(checksum,HEX);
    

   // flip order of every four bytes
   m.MCA[0] = MassC[3];
   m.MCA[1] = MassC[2];
   m.MCA[2] = MassC[1];
   m.MCA[3] = MassC[0];
   
   m.MCA[4] = MassC[7];
   m.MCA[5] = MassC[6];
   m.MCA[6] = MassC[5];
   m.MCA[7] = MassC[4];

   m.MCA[8] = MassC[11];
   m.MCA[9] = MassC[10];
   m.MCA[10] = MassC[9];
   m.MCA[11] = MassC[8];

   m.MCA[12] = MassC[15];
   m.MCA[13] = MassC[14];
   m.MCA[14] = MassC[13];
   m.MCA[15] = MassC[12];

   n.NCA[0] = NumC[3];
   n.NCA[1] = NumC[2];
   n.NCA[2] = NumC[1];
   n.NCA[3] = NumC[0];

   n.NCA[4] = NumC[7];
   n.NCA[5] = NumC[6];
   n.NCA[6] = NumC[5];
   n.NCA[7] = NumC[4];

   n.NCA[8] = NumC[11];
   n.NCA[9] = NumC[10];
   n.NCA[10] = NumC[9];
   n.NCA[11] = NumC[8];

   n.NCA[12] = NumC[15];
   n.NCA[13] = NumC[14];
   n.NCA[14] = NumC[13];
   n.NCA[15] = NumC[12];

   n.NCA[16] = NumC[19];
   n.NCA[17] = NumC[18];
   n.NCA[18] = NumC[17];
   n.NCA[19] = NumC[16];

   // convert to float and print or save...
   Serial.print("Mass Concentration: ");
   for(int k = 0; k<4; k++)
   {
    Serial.print(m.MCF[k]);
    Serial.print(" ");
   }
   Serial.println();
   Serial.print("Number Concentration: ");
   for(int k = 0; k<5; k++)
   {
    Serial.print(n.NCF[k]);
    Serial.print(" ");
   }

    
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
  }
}

void SPS_power_on()
{
  // send startup frame
  Serial1.write(0x7E);
  Serial1.write(0x00);
  Serial1.write(0x00); // this is the actual command
  Serial1.write(0x02);
  Serial1.write(0x01);
  Serial1.write(0x03);
  Serial1.write(0xF9);
  Serial1.write(0x7E);

  // read the response bytes
  delay(100);
  data = Serial1.read();
  data = Serial1.read();
  data = Serial1.read();
  data = Serial1.read();
  data = Serial1.read();
  data = Serial1.read();
  data = Serial1.read();
}

void SPS_read_data()
{
  Serial1.write(0x7E);
  Serial1.write(0x00);
  Serial1.write(0x03); // this is the actual command
  Serial1.write(0x00);
  Serial1.write(0xFC);
  Serial1.write(0x7E);

  // need to read in the 40 bytes from the response
  while(Serial1.available())
  {
    for(int j = 0; j<5; j++) // info like start bit, command and length
    {
      if(Serial1.available())
      {
        data = Serial1.read();
        checksum += data;
      }
      Serial.print(data,HEX);
      Serial.print(" ");
    }
    //Serial.println();
    data = 0;
    for(int j = 0; j<16; j++) // mass concentration
    {
      if(Serial1.available())
      {
        MassC[j] = Serial1.read();
        checksum += MassC[j];
        Serial.print(MassC[j],HEX);
        Serial.print(" ");
      }
    }
    for(int j = 0; j<20; j++) // number concetration
    {
      if(Serial1.available())
      {
        NumC[j] = Serial1.read();
        checksum += NumC[j];
        Serial.print(NumC[j],HEX);
        Serial.print(" ");
      }
    }
    for(int j = 0; j<6; j++) //average particle size and end bits
    {
      if(Serial1.available())
      {
        data = Serial1.read();
        checksum += data;
        if(j == 4)
        {
          checksum -= data;
        }
      }
      Serial.print(data,HEX);
      Serial.print(" ");
    }
    Serial.println();
    data = 0;
  }
}
