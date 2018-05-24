#include <Wire.h>  
int ADXAddress = 0xA7 >> 1;  // the default 7-bit slave address
byte buff[2]="";
int dataSum[3];
float Roll, Pitch, gOffset;
void setup()
{
  Wire.begin();
  Serial.begin(115200);
  delay(100);
  writeTo(ADXAddress, 0x2D, 8);  // enable to measute g data
}
void loop()
{
  float sumRoll,sumPitch;
  for(int i=0;i<100;i++){
    readFrom(ADXAddress, 0x32, 0);
    readFrom(ADXAddress, 0x34, 1);
    readFrom(ADXAddress, 0x36, 2);
    float x=float(dataSum[0]);
    float y=float(dataSum[1]);
    float z=float(dataSum[2]);
    gOffset=float(sqrt(x*x+y*y+z*z));
    sumRoll=sumRoll+float(atan2(x,gOffset)*360/PI);//with the function atan2, i can get the angle value
    sumPitch=sumPitch+float(atan2(y,gOffset)*360/PI);
  }
  Roll=sumRoll/100.0;
  Pitch=sumPitch/100.0;
  Serial.print("Angle\tX:");Serial.print(Roll);
  Serial.print("\tY:");Serial.println(Pitch);
}

/************************** Functions *********************************/
//Writes val to address register
void writeTo(int DEVICE, byte address, byte val) {
   Wire.beginTransmission(DEVICE); //start transmission
   Wire.write(address);        // send register address
   Wire.write(val);        // send value to write
   Wire.endTransmission(); //end transmission
}


//reads num bytes starting from address register on ACC in to buff array
void readFrom(int DEVICE, byte address, int num) {
  int j = 0;
  dataSum[num]=0;
  Wire.beginTransmission(DEVICE); //start transmission
  Wire.write(address);            // send register address
  Wire.endTransmission();         //end transmission
  Wire.beginTransmission(DEVICE); //start transmission to ACC
  Wire.requestFrom(ADXAddress, 2);// request 6 bytes from ACC
  while(Wire.available())         //ACC may send less than requested (abnormal)
  { 
    buff[j] = Wire.read();        // receive a byte
    j++;
  }
  dataSum[num]=int(buff[1]<<8|buff[0]);
  Wire.endTransmission();         //end transmission
}
    
