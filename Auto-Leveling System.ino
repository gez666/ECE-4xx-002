 #include <SPI.h>    //SPI protocol 
#include <Wire.h>     //I2C protocol 
#include "RTClib.h"   //RTC
#include "Adafruit_MCP9808.h"   //MCP9808 lib
#include <SD.h>   //SD card
#include "Adafruit_BluefruitLE_SPI.h"   //bluetooth lib
#include "BluefruitConfig.h"    //bluetooth header file
#include <Adafruit_MotorShield.h>   //stepper motor lib 
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.7.7"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);   //
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();   //define variable 
RTC_PCF8523 rtc;    //申明RTC的对象
//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};   // define arrary 
#define SD_pin 10    
Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x60);     

#define s1_num 0    
#define s1_PWMA 8   
#define s1_PWMB 13  
#define s1_AIN1 10
#define s1_AIN2 9
#define s1_BIN1 11
#define s1_BIN2 12

#define s2_num 1
#define s2_PWMA 2
#define s2_PWMB 7
#define s2_AIN1 4
#define s2_AIN2 3
#define s2_BIN1 5
#define s2_BIN2 6


Adafruit_StepperMotor *myStepper1 = AFMS.setstepperpin(s1_num,s1_PWMA,s1_PWMB,s1_AIN1,s1_AIN2,s1_BIN1,s1_BIN2); 
Adafruit_StepperMotor *myStepper2 = AFMS.setstepperpin(s2_num,s2_PWMA,s2_PWMB,s2_AIN1,s2_AIN2,s2_BIN1,s2_BIN2);
 
//error function
void error(const __FlashStringHelper*err) {
  Serial.println(err);    
  while (1);    
}
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);    // function prototypes over in packetparser.cpp
float parsefloat(uint8_t *buffer);    
void printHex(const uint8_t * data, const uint32_t numBytes);    
extern uint8_t packetbuffer[];    //
//define the ADXL345 data address
#define Register_ID 0  
#define Register_2D 0x2D  
#define Register_X0 0x32  
#define Register_X1 0x33  
#define Register_Y0 0x34  
#define Register_Y1 0x35  
#define Register_Z0 0x36  
#define Register_Z1 0x37  
  
int ADXAddress = 0xA7 >> 1;  // the default 7-bit slave address
int X0,X1,X_out;
int Y0,Y1,Y_out;
int Z0,Z1,Z_out;
double Roll,Pitch;

//boolean balance = 1;
unsigned long time_start, start;

bool bNeedMotor = false;

void get_angle()
{
  int count_x=0, count_y=0, count_z=0;   
    
for(int i=0; i<20; i++)
{
  //measure the ADXL345 and show to Serial
  //X  
  Wire.beginTransmission(ADXAddress); // transmit to device
  Wire.write(Register_X0);
  Wire.write(Register_X1);
  Wire.endTransmission();
  Wire.requestFrom(ADXAddress,2);//
  if(Wire.available()<=2)
  {
    X0 = Wire.read();
    X1 = Wire.read();
    X1=X1<<8;//
    X_out=X0+X1;
    count_x += X_out;
  }
  //Y
  Wire.beginTransmission(ADXAddress); // transmit to device
  Wire.write(Register_Y0);
  Wire.write(Register_Y1);
  Wire.endTransmission();
  Wire.requestFrom(ADXAddress,2); 
  if(Wire.available()<=2)
  {
    Y0 = Wire.read();
    Y1 = Wire.read();
    Y1=Y1<<8;
    Y_out=Y0+Y1;
    count_y += Y_out;
  }
  //Z
  Wire.beginTransmission(ADXAddress); // transmit to device
  Wire.write(Register_Z0);
  Wire.write(Register_Z1);
  Wire.endTransmission();
  Wire.requestFrom(ADXAddress,2); 
  if(Wire.available()<=2)
  {
    Z0 = Wire.read();
    Z1 = Wire.read();
    Z1=Z1<<8;
    Z_out=Z0+Z1;
    count_z += Z_out;
  }
  delay(1);
}


 X_out = count_x /10;
 Y_out = count_y /10;
 Z_out = count_z /10;

 
  
  //transform to angle

  // adjust angle value here based on real situation 
  Roll=(float)(atan2(X_out,Z_out)*180/PI)-3.0;
  Pitch=(float)(atan2(Y_out,Z_out)*180/PI);//PI是圆周率3.1415926
  

}

void sendFileByBLE()
{
    bool bNeed = false;
    bool bSend = false;
    while(ble.available())
    {
        int c = ble.read();
        if(c == 's'
            || c == 'S')
        {
            bNeed = true;
        }
        else if(c == 'f'
          || c == 'F')
        {
            bSend = true;    
        }
    }

    if(!bNeed && !bSend)
    {
        return;
    }

    File dataFile = SD.open("datalog.txt");//open datalog.txt   
    if (dataFile) 
    {
        // read from the file until there's nothing else in it:
        while(dataFile.available())
        {       
            ble.write(dataFile.read());
        }
      
        // close the file:
        dataFile.close();

        if(bSend)
        {
            return;
        }

        if(!SD.remove("datalog.txt"))
        {
            Serial.println("Remove datalog.txt failed!");
        }
    } 
    else 
    {
        // if the file didn't open, print an error:
        Serial.println("error opening datalog.txt");
    }

}



void setup()
{
  
  Serial.begin(9600);    
  Serial.print(F("Initialising the Bluefruit LE module: ")); 
  Wire.begin();
 
  delay(100);
  if (! rtc.begin()) {    //enable RTC
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! rtc.initialized()) {
    Serial.println("RTC is NOT running!");
  
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  delay(100);
  if (!tempsensor.begin()) {    //enable tempsensor
    Serial.println("Couldn't find MCP9808!");
    while (1);
  }else{
    Serial.println("MCP9808 Connected");
  }
  
  delay(100);
  //begin transmition between adxl345 and mcu
  Wire.beginTransmission(ADXAddress);
  Wire.write(Register_2D);
  Wire.write(8);//measuring enable  
  Wire.endTransmission();// stop transmitting  
  
  delay(100);
  if (!SD.begin(SD_pin)) {//judge function
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }else{
    Serial.println("card initialized.");
  }

  //enable stepper motor
  delay(100);
  AFMS.begin();
  myStepper1->setSpeed(10);//
  myStepper2->setSpeed(10);// set stepper motor rotation speed 10 rpm, 10 round per mintute
  
  delay(100);
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));    
  }
  else
  {
    Serial.println( F("OK!") );    
  }
  if ( FACTORYRESET_ENABLE )
  {
    Serial.println(F("Performing a factory reset: "));    // Perform a factory reset to make sure everything is in a known state
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));    
    }
  }

  ble.echo(false);    // Disable command echo from Bluefruit
  Serial.println("Requesting Bluefruit info:");    // Print Bluefruit information 
  ble.info();    
  Serial.println(F("Please use Adafruit Bluefruit LE app to connect bluetooth and open the plotter."));    
  Serial.println();    
  ble.verbose(false);    // debug info is a little annoying after this point!
  Serial.println(F("******************************"));    
  // LED Activity command is only supported from 0.7.7
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION))
  {
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));    // Change Mode LED Activity
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);    
  }
  ble.setMode(BLUEFRUIT_MODE_DATA);    // Set Bluefruit to DATA mode
  Serial.println(F("******************************")); 
  time_start = millis(); 
  start = millis();  
}

void loop()
{ 
    sendFileByBLE();
    uint32_t cur_time = millis();
    get_angle();
    if((cur_time - start) >= 5000)//every 5 second write in and output data
    {
        start = cur_time;
        DateTime now = rtc.now();
        Serial.print(now.year(), DEC);//
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print("\t");//制表符table
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.print("\t");

        //measure the temperature and show to Serial
        float c = tempsensor.readTempC();   //adjust angle here based on real envirment，for example float c = tempsensor.readTempC()-3.3;
        Serial.print("Temp: "); 
        Serial.print(c); 
        Serial.print("℃\t");      

        Serial.print("X:");
        Serial.print(Roll);
        Serial.print(" Y:");
        Serial.print(Pitch);
        Serial.println("\t");
  
        //open sd card
        File dataFile = SD.open("datalog.txt", FILE_WRITE);//open datalog.txt
        // if the file is available, write to it:
  
        if (dataFile)
        {
            //write the time to sd card
            dataFile.print(now.year(), DEC);
            dataFile.print('/');
            dataFile.print(now.month(), DEC);
            dataFile.print('/');
            dataFile.print(now.day(), DEC);
            dataFile.print("\t");
            dataFile.print(now.hour(), DEC);
            dataFile.print(':');
            dataFile.print(now.minute(), DEC);
            dataFile.print(':');
            dataFile.print(now.second(), DEC);
            dataFile.print("\t");

            //write the temperature to sd card
            dataFile.print("Temp: "); 
            dataFile.print(c); 
            dataFile.print("℃\t"); 

            //write the angle to sd card
            dataFile.print("X:");
            dataFile.print(Roll);
            dataFile.print(" Y:");
            dataFile.print(Pitch);
            dataFile.println();//   
          //
            dataFile.close();
        } 
        else
        {
            Serial.println("Error opening datalog.txt");//
            SD.remove("datalog.txt");//
        }


        //plot on app(only the angle,not include the time,temperature and so on)
        ble.print(now.hour(), DEC);//
        ble.print(':');
        ble.print(now.minute(), DEC);
        ble.print(':');
        ble.print(now.second(), DEC);
        ble.print("\t");
        ble.print(Roll);
        ble.print(",");
        ble.println(Pitch);  //
    }

   delay(50);


    if(cur_time - time_start > (1000))//every 1 hour run the stepper motor
	{
		bNeedMotor = true;
		time_start = cur_time;
	}

    if(!bNeedMotor)
    {
        return;
    }

    
    if(Roll>1.0 ||Roll<-1.0  || Pitch>1.0 || Pitch <-1.0)
    {
        
        if (Roll>0.90)
        {   //
            myStepper1->step(12, BACKWARD, SINGLE);
        }
        else if(Roll<-0.90)
        {//
            myStepper1->step(12, FORWARD, SINGLE);
        }
        myStepper1->release();
        delay(500);
        
        if (Pitch>0.90)
        {   //
            myStepper2->step(12, FORWARD, SINGLE); 
        }
        else if(Pitch<-0.90)
        {//
            myStepper2->step(12, BACKWARD, SINGLE); 
        }
        myStepper2->release()
        ;//
        delay(500);
    } 
	
	
	if((Roll < 1.0 && Roll > -1.0) && (Pitch < 1.0 && Pitch > -1.0))
	{
		bNeedMotor = false;
	}
    
}



