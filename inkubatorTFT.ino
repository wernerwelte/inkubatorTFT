#include <OneWire.h> 
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>    // Hardware-specific library
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include "SparkFunBME280.h"

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define GREY    0x73AE
#define BROWN   0xC2AA
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define ROMdelta 10
#define ROMsleep 20

BME280 myBME280; //Uses I2C address 0x76 (jumper closed)

MCUFRIEND_kbv tft;
//MCUFRIEND_kbv tft  = new MCUFRIEND_kbv(A3, A2, A1, A0, A4);
 
void startFan();
void stopFan();
void render_tft(void);

int8_t isDryEnough( double wert[] );
float  get_temperatur(void);
bool Touch_getXY( void );

float temperatur;

void renderLCD0();
void render1();
void renderLCD2();
void renderLCD3();

String s_t_i, s_t_a, s_a_i, s_a_a, s_temperatur;
String on, off, heat_state; 

double t_i, t_a, r_i, r_a, p_i, p_a, a_i, a_a, wert[ 8 ] ;

int8_t relaisPin   = 1;

int8_t lcdState  = 1;
int8_t fanActive = 0;
int8_t delta     = 0;
float  soll_temperatur = EEPROM.read( ROMdelta );
int8_t sleepTime       = EEPROM.read( ROMsleep );
float  ist_temperatur = 20;
long counter     = 0;

// TFT  Calibrate
const uint8_t  XP      = 8  , XM    = A2 , YP     = A3, YM     = 9;  //240x320 ID=0x9341
const uint16_t TS_LEFT = 114, TS_RT = 912, TS_TOP = 70, TS_BOT = 891;

int pixel_x, pixel_y;     // Touch_getXY() updates global vars

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

Adafruit_GFX_Button on_btn, off_btn;

#define MINPRESSURE 200
#define MAXPRESSURE 1000

int16_t  BOXSIZE;
uint8_t  PENRADIUS   = 1;
uint16_t ID, oldcolor, currentcolor;
uint8_t  Orientation = 0;    //PORTRAIT

uint8_t  changed = 0;

void setup()
{
  pinMode( relaisPin, OUTPUT );
  digitalWrite( relaisPin, HIGH );

  soll_temperatur = 10;
  
  on = String  ( "ON"  );
  off = String ( "OFF" );
  heat_state = off;
    
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  //Serial.begin(9600);//
  //while(!Serial);                                                                Serial.println("START INIT");

  Wire.begin();

  // ---- TFT- test ----
  tft.reset();
  ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(Orientation);
  tft.fillScreen(BLACK);


tft.fillRect( 0 , 0   , 240 , 60  , BLUE  );
tft.fillRect( 0 , 60  , 240 , 200 , BLACK );
tft.fillRect( 0 , 260 , 240 , 60  , BLUE  );
tft.fillRect( 0 , 120 , 240 , 60  , BLUE  );

off_btn.initButton  ( &tft,  60, 220 , 110, 60, BLACK,WHITE, BLACK, "-", 5 );
on_btn.initButton   ( &tft, 180, 220 , 110, 60, BLACK,WHITE, BLACK, "+", 5 );

on_btn.drawButton  ( false );
off_btn.drawButton ( false );

myBME280.setI2CAddress( 0x76 ); //Connect to a second sensor
if( myBME280.beginI2C( ) == false ){ Serial.println("Sensor BME280 connect failed"); }

render1();
}


void loop()
{
 
  counter++;

  bool down = Touch_getXY();
  on_btn.press(down && on_btn.contains(pixel_x, pixel_y));
  off_btn.press(down && off_btn.contains(pixel_x, pixel_y));
    
  if (on_btn.justReleased())
  { on_btn.drawButton();
  }
    
  if (off_btn.justReleased())
  { off_btn.drawButton();
    soll_temperatur--;  
  render1();
  }
    
  if (on_btn.justPressed()) 
  { on_btn.drawButton( true );
    soll_temperatur++;  
  render1();
  }
    
  if (off_btn.justPressed()) 
  { off_btn.drawButton( true );
  }
   
  if (counter % 1500 == 0)
  { 
   
     ist_temperatur = get_temperatur();
   // Serial.print("  Temperature = ");
   // Serial.print(ist_temperatur);
   // Serial.println(" Celsius, ");
    
    if( soll_temperatur > ist_temperatur )
    { heat_state = on;
      digitalWrite( relaisPin, LOW );
    }
    else
    { heat_state = off;
      digitalWrite( relaisPin, HIGH );
    }
    

    //Serial.println(heat_state);

    
     render1( );
    /*
    if ( isDryEnough( wert ) )   {  startFan();    }
    else                         {  stopFan();     }
    */
  }
  
}



void render1( )
{
char  ist_temper[ 6 ]; 
char soll_temper[ 6 ]; 
dtostrf(  ist_temperatur, 3, 1,  ist_temper );
dtostrf( soll_temperatur, 3, 1, soll_temper );

tft.setTextColor( BLACK );
tft.setCursor( 10 , 15  ) ;  tft.setTextSize( 4 ); tft.println( "IST:" );
tft.setCursor( 10 , 135 ) ;  tft.setTextSize( 4 ); tft.println( "SOLL:" );
tft.setCursor( 10 , 275 ) ;  tft.setTextSize( 4 ); tft.println( "HEAT:" );

tft.setTextColor( WHITE );
tft.fillRect ( 130 , 0   , 110 , 60 , BLUE );
tft.setCursor( 130 , 15  ) ;  tft.setTextSize( 4 ); tft.println( ist_temper );

tft.fillRect ( 130 , 120 , 110 , 60 , BLUE );
tft.setCursor( 130 , 135 ) ;  tft.setTextSize( 4 ); tft.println( soll_temper );
 
tft.fillRect ( 130 , 260  , 110 , 60 , BLUE);
tft.setCursor( 130 , 275 ) ;  tft.setTextSize( 4 ); tft.println( heat_state );
}


/*
void startFan()
{
  digitalWrite( relaisPin, LOW  );
  for ( int8_t i = sleepTime; i > 0; i-- )
  {                                                                                  // Serial.print( "SLEEP:" ); Serial.print( i ); Serial.println( " Sekunden" );
    for (int8_t j = 0; j < 5; j++)
    {
      delay( 100 );
      {  //getWeatherValues  ( mySensor1, mySensor1, wert );                         // Serial.println( "getWeatherValues" );  // ermittelte Daten werden in das Array 'wert' gespeichert
         //renderLCD(  wert );                                                       // Serial.println( "renderLCD" );
      }
    }
  }
}


void stopFan()
{
  digitalWrite( relaisPin, HIGH );
}

*/
/*
int8_t isDryEnough( double wert[] )
{
delta = wert[ 3 ] - wert[ 7 ];

if ( delta >soll_temperatur )  // Außen-AbsLuftFeuchte < Innen-AbsLuftFeuchte
{  fanActive = 1;   //  Serial.print( "I:" ); Serial.print( wert[ 7 ] );  Serial.print( "  A: " ); Serial.print( wert[ 3 ]  ); Serial.print( "  Diff:" ) ; Serial.print( delta );Serial.print( "  Delta:" );  Serial.print(soll_temperatur );  Serial.println ( "   RELAIS EIN"    );
}
else
{  fanActive = 0;    // Serial.print( "I:" ); Serial.print( wert[ 7 ] ); Serial.print( "  A: " ); Serial.print( wert[ 3 ] ); Serial.print( "  Diff:" ) ; Serial.print( delta ); Serial.print( "  Delta:" );  Serial.print(soll_temperatur );   Serial.println ( "   RELAIS AUS"    );
}
return fanActive;
}
*/

double rnd( double value )
{
//  Serial.println("-----");
//  Serial.println(value);
  value =  round ( value * 10 ) ;
  value =  value / 10 ;
//  Serial.println(value);
  return  value;
}


bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
    }
    return pressed;
}
 
float  get_temperatur(void)
{
  float temperatur;
  //Serial.print(" Humidity : ");
  //Serial.print(myBME280.readFloatHumidity(), 0);

  //Serial.print(" Pressure : ");
  //Serial.print(myBME280.readFloatPressure(), 0);

  //Serial.print(" Temp : ");
  temperatur = (myBME280.readTempC());
  
  //Serial.print(temperatur, 2);
 // Serial.print(myBME280.readTempF(), 2);

 // Serial.println();


 return temperatur;

   
   
 
}
