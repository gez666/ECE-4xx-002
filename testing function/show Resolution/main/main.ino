#include <Wire.h>  
int ADXAddress = 0xA7 >> 1;  // the default 7-bit slave address
byte buff[2]="";
int dataSum[3];
float xMax,yMax,zMax;
void setup()
{
  Wire.begin();
  Serial.begin(115200);
  delay(100);
  writeTo(ADXAddress, 0x2D, 8);  // enable to measute g data
}
void loop()
{
  xMax=0;
  yMax=0;
  zMax=0;
  Serial.print("请分别将X,Y,Z轴垂直向下放置，并缓慢匀速的晃动，以保证在某一时刻，该轴是与g方向重合，从而测的真是的灵敏度！");
  Serial.print("程序20s输出一次！");
  for(int i=0;i<2000;i++){
    readFrom(ADXAddress, 0x32, 0);
    readFrom(ADXAddress, 0x34, 1);
    readFrom(ADXAddress, 0x36, 2);
    if(abs(dataSum[0])>xMax){
      xMax=abs(dataSum[0]);
    }
    if(abs(dataSum[1])>yMax){
      yMax=abs(dataSum[1]);
    }
    if(abs(dataSum[2])>zMax){
      zMax=abs(dataSum[2]);
    }
    delay(10);
  }
  //print to UART monitor
  Serial.print("X:");Serial.print(xMax);Serial.print("LSB/g\t");
  Serial.print("Y:");Serial.print(yMax);Serial.print("LSB/g\t");
  Serial.print("Z:");Serial.print(zMax);Serial.println("LSB/g\t");
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
    

