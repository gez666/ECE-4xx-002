#include <Wire.h>  
int ADXAddress = 0xA7 >> 1;  // the default 7-bit slave address
byte buff[2]="";
int dataSum[3];

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  delay(100);
  writeTo(ADXAddress, 0x2D, 8);  // enable to measute g data
}
void loop()
{
  float xMax,yMax,zMax;
  float sumXg,sumYg,sumZg,gOffset;
  float sumRoll,sumPitch,Roll,Pitch;
  xMax=250;yMax=254;zMax=255;
  sumXg=0;sumYg=0;sumZg=0;gOffset=0;
  for(int i=0;i<100;i++){
    readFrom(ADXAddress, 0x32, 0);
    readFrom(ADXAddress, 0x34, 1);
    readFrom(ADXAddress, 0x36, 2);
    float x=float(dataSum[0]);
    float y=float(dataSum[1]);
    float z=float(dataSum[2]);
    //分辨率
    if(abs(x)>xMax){
      xMax=abs(x);
    }
    if(abs(y)>yMax){
      yMax=abs(y);
    }
    if(abs(z)>zMax){
      zMax=abs(z);
    }
    //加速度
    gOffset=float(sqrt(x*x+y*y+z*z));
    sumXg=sumXg+float(x/gOffset);
    sumYg=sumYg+float(y/gOffset);
    sumZg=sumZg+float(z/gOffset);
    //角度
    sumRoll=sumRoll+float(atan2(x,gOffset)*360/PI);//with the function atan2, i can get the angle value
    sumPitch=sumPitch+float(atan2(y,gOffset)*360/PI);
  }
  //灵敏度
  Serial.print("分辨率\tX:");Serial.print(xMax);Serial.print("LSB/g\t");
  Serial.print("Y:");Serial.print(yMax);Serial.print("LSB/g\t");
  Serial.print("Z:");Serial.print(zMax);Serial.println("LSB/g\t");
  //加速度
  Serial.print("Acceleration\tX:");Serial.print(sumXg/100.0);Serial.print("g\t");
  Serial.print("Y:");Serial.print(sumYg/100.0);Serial.print("g\t");
  Serial.print("Z:");Serial.print(sumZg/100.0);Serial.println("g");
  //角度
  Roll=sumRoll/100.0;
  Pitch=sumPitch/100.0;
  Serial.print("Angle\tX:");Serial.print(Roll);
  Serial.print("\tY:");Serial.println(Pitch);Serial.println();
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
    

