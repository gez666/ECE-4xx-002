#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif
/*-------------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.7.7"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
//ADXL345
#include <Wire.h> 
int ADXAddress = 0xA7 >> 1;  // the default 7-bit slave address
byte buff[2]="";
int dataSum[3];

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

void setup(void)
{
  while (!Serial);  // required for Flora & Micro
  delay(500);

  Wire.begin();
  Serial.begin(115200);
  delay(100);
  writeTo(ADXAddress, 0x2D, 8);  // enable to measute g data
  
  Serial.println(F("Adafruit Bluefruit App Controller Example"));
  Serial.println(F("-----------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }
  /* Disable command echo from Bluefruit */
  ble.echo(false);
  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();
  ble.verbose(false);  // debug info is a little annoying after this point!
  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }
  Serial.println(F("******************************"));
  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) ){
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }
  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);
  Serial.println(F("******************************"));
}

void loop(void)
{
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;
  float sumXg, sumYg, sumZg, gOffset;;
  sumXg=0.0;sumYg=0.0;sumZg=0.0;gOffset=0.0;
  // Accelerometer
  if (packetbuffer[1] == 'A') {
    for(int i=0;i<100;i++){
      readFrom(ADXAddress, 0x32, 0);
      readFrom(ADXAddress, 0x34, 1);
      readFrom(ADXAddress, 0x36, 2);
      float x=float(dataSum[0]);
      float y=float(dataSum[1]);
      float z=float(dataSum[2]);
      gOffset=float(sqrt(x*x+y*y+z*z));
      sumXg=sumXg+float(x/gOffset);
      sumYg=sumYg+float(y/gOffset);
      sumZg=sumZg+float(z/gOffset);
      delay(10);
    }
    Serial.print("ADXL345\tX:");Serial.print(sumXg/100.0);Serial.print("g\t");
    Serial.print("Y:");Serial.print(sumYg/100.0);Serial.print("g\t");
    Serial.print("Z:");Serial.print(sumZg/100.0);Serial.print("g\t");
    
    float x, y, z;
    x = parsefloat(packetbuffer+2);
    y = parsefloat(packetbuffer+6);
    z = parsefloat(packetbuffer+10);
    Serial.print("Iphone data\tX:");Serial.print(x); Serial.print("g\t");
    Serial.print("Y:");Serial.print(y); Serial.print("g\t");
    Serial.print("Z:");Serial.print(z); Serial.println("g\t");
  }
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
