#include <ss_oled.h>
#include <Wire.h>
#include <dht.h>
#include <stdio.h>
#include <math.h>
#include <AirQuality.h>

#define DHT22PIN 2

dht DHT22;
AirQuality airqualitysensor;

///////////////////////DUST SENSOR
int pin_dust = 4;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
///////////////////////////////////////


#define SDA_PIN 18
#define SCL_PIN 19

#define RESET_PIN -1

#define OLED_ADDR -1
// Rotation de l'écran
#define FLIP180 180
//Ca inverse le blanc et le noir
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 0

#define MY_OLED OLED_128x128
#define OLED_WIDTH 128
#define OLED_HEIGHT 128


SSOLED ssoled;

void setup() {
int rc;
// The I2C SDA/SCL pins set to -1 means to use the default Wire library
// If pins were specified, they would be bit-banged in software
// This isn't inferior to hw I2C and in fact allows you to go faster on certain CPUs
// The reset pin is optional and I've only seen it needed on larger OLEDs (2.4")
//    that can be configured as either SPI or I2C
//
  
  // oledInit(SSOLED *, type, oled_addr, rotate180, invert, bWire, SDA_PIN, SCL_PIN, RESET_PIN, speed)
  rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  oledFill(&ssoled, 0, 1);
  oledWriteString(&ssoled, 0,10,0,(char *)"Projet tuteure", FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,0,2,(char *)"Start...", FONT_NORMAL, 0, 1);
  pinMode(2,INPUT);
  pinMode(A0,INPUT);
  Serial.begin(9600);
  
  airqualitysensor.init(A0);
  int Q_air=50;

  /////////////////////DUST SENSOR
  pinMode(pin_dust,INPUT);
  starttime = millis();//get the current time;
  ////////////////////////////////////
} 


void loop() {

  /////////////////////////DUST SENSOR
  duration = pulseIn(pin_dust, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;
 
  if ((millis()-starttime) > sampletime_ms)//if the sampel time == 30s
  {
      ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
      concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
      lowpulseoccupancy = 0;
      starttime = millis();
  }
  /////////////////////////////////
  
  DHT22.read(DHT22PIN);
  int previous_milli;
  int Q_air;
  
  long int milli=millis();
  
  if(milli-previous_milli>2000){
    Q_air=airqualitysensor.first_vol;
    previous_milli=milli;
  }
  airqualitysensor.slope();
  float hum=DHT22.humidity;
  int hum_int = floor(hum);
  int hum_dec = hum*10-hum_int*10;
  char affich[32]="";
  sprintf(affich, "Humidite:%d.%d%%", hum_int,hum_dec);

  float tempe=DHT22.temperature;
  int temp_int = floor(tempe);
  int temp_dec = tempe*10-temp_int*10;
  char affich2[32]="";
  sprintf(affich2, "Temperature:%d.%d",temp_int,temp_dec);

  char affich3[32]="";
  sprintf(affich3, "Air_qualite:%d", Q_air);

 
    int particules_int = floor(concentration);
    int particules_dec = concentration*10-particules_int*10;
    char affich4[32]="";
    sprintf(affich4, "C.Part:%d.%d",particules_int,particules_dec);
  
  
  oledWriteString(&ssoled, 0,10,0,(char *)"Projet tuteure", FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,0,2,(char *)" Ecrit par Julien P ", FONT_SMALL, 1, 1);
  oledWriteString(&ssoled, 0,0,4,affich, FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,0,5,affich2, FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0,0,6,affich3, FONT_NORMAL, 0, 1);
  if(concentration>=100)
  {
    oledWriteString(&ssoled, 0,0,7,affich4, FONT_NORMAL, 0, 1);
  }
  delay(2000);
} 



ISR(TIMER2_OVF_vect)
{
    if(airqualitysensor.counter==122)//set 2 seconds as a detected duty
    {
        airqualitysensor.last_vol=airqualitysensor.first_vol;
        airqualitysensor.first_vol=analogRead(A0);
        airqualitysensor.counter=0;
        airqualitysensor.timer_index=1;
        PORTB=PORTB^0x20;
    }
    else
    {
        airqualitysensor.counter++;
    }
}
