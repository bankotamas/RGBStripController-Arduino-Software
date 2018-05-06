#include "SimpleTimer.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN            6
#define NUMPIXELS      60

#define ONE_WIRE_BUS 2

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define SIMPLE_COLOR  0
#define RAINBOW       1
#define RAINBOW_CYCLE 2
#define KNGIHT_RIDER  3
#define FADE          4
#define BLINK         5
#define GET_TEMP      6

uint8_t RED = 0;
uint8_t GREEN = 0;
uint8_t BLUE = 0;

const byte numChars = 64;
char receivedChars[numChars];  // an array to store the received data
uint8_t command = 0;
boolean newData = false;

SimpleTimer tempTimer;

/* Function prototypes */
void sendTempToPC();

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() 
{
  Serial.begin(115200);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.show();  // all pixel OFF

  sensors.begin();
  sendTempToPC();
  
  /* set timers */
  tempTimer.setInterval(60000L, sendTempToPC);
}

void loop() 
{
  tempTimer.run();
  recvWithEndMarker();
  showNewData();
}

void sendTempToPC()
{
  sensors.requestTemperatures();

  Serial.print("Temp:");
  Serial.println(sensors.getTempCByIndex(0)); 
}

void recvWithEndMarker() 
{
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  
  if (Serial.available() > 0) 
  {
    rc = Serial.read();

    if (rc != endMarker) 
    {
      receivedChars[ndx] = rc;
      ndx++;
      
      if (ndx >= numChars) 
      {
        ndx = numChars - 1;
      }
    }
    else 
      {
        ndx++;
        receivedChars[ndx] = '\0'; // terminate the string
        ndx = 0;
        newData = true;
      }
  }
}

void showNewData() 
{
  if (newData == true) 
  {
    parseData();
    newData = false;
  }
}

// Split the data into its parts
void parseData() 
{
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(receivedChars,",");
  command = atoi(strtokIndx);
  
  strtokIndx = strtok(NULL,",");
  RED = atoi(strtokIndx); 
          
  strtokIndx = strtok(NULL, ","); 
  GREEN = atoi(strtokIndx);   
        
  strtokIndx = strtok(NULL, ","); 
  BLUE = atoi(strtokIndx);
  
  switch(command)
  {
    case SIMPLE_COLOR:      
        set_color(RED, GREEN, BLUE);
        break;
    case RAINBOW:
        rainbow(20);
        break;
    case RAINBOW_CYCLE:
        rainbowCycle(20);
        break;
    case KNGIHT_RIDER:
        knightRider(1, 32, 8, pixels.Color(RED, GREEN, BLUE)); // Cycles, Speed, Width, RGB Color (red)
        break;
    case FADE:
        fadeColor(RED, GREEN, BLUE);
        break;
    case BLINK:
        blinkColor(RED, GREEN, BLUE);
    case GET_TEMP:
        sendTempToPC();
        break;
    default:
        break;
  }
  
}

void blinkColor(int red, int green, int blue)
{
  do
  {
    for(int i = 0; i < pixels.numPixels(); i++)
    {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    
    if (Serial.available() > 0) 
    {
      return 0;
    }
    
    pixels.show();
    delay(500);

    for(int i = 0; i < pixels.numPixels(); i++)
    {
      pixels.setPixelColor(i, pixels.Color(red, green, blue));
    }

    if (Serial.available() > 0) 
    {
      return 0;
    }
    
    pixels.show();
    delay(500);
    
  }while(!Serial.available() > 0);
}

void fadeColor(int red, int green, int blue)
{
  do
  {
    for(int i = 100; i >= 0; i--)
    {
      for(int k = 0; k < pixels.numPixels(); k++)
      {
        if (Serial.available() > 0) 
        {
          return 0;
        }
        
        pixels.setPixelColor(k, pixels.Color((red / 100) * i, (green / 100) * i, (blue / 100) * i));
      }

      pixels.show();
      delay(15);
    }
    
    delay(45);
    
    for(int i = 0; i <= 100; i++)
    {
      for(int k = 0; k < pixels.numPixels(); k++)
      {
        if (Serial.available() > 0) 
        {
          return 0;
        }
        
        pixels.setPixelColor(k, pixels.Color((red / 100) * i, (green / 100) * i, (blue / 100) * i));
      }

      pixels.show();
      delay(10);
    }
  }while(!Serial.available() > 0);
}

void set_color(int red, int green, int blue)
{
  for(int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(red,green,blue)); 
  }
  pixels.show();
}

// Cycles - one cycle is scanning through all pixels left then right (or right then left)
// Speed - how fast one cycle is (32 with 16 pixels is default KnightRider speed)
// Width - how wide the trail effect is on the fading out LEDs.  The original display used
//         light bulbs, so they have a persistance when turning off.  This creates a trail.
//         Effective range is 2 - 8, 4 is default for 16 pixels.  Play with this.
// Color - 32-bit packed RGB color value.  All pixels will be this color.
// knightRider(cycles, speed, width, color);
void knightRider(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color) 
{
  uint32_t old_val[NUMPIXELS]; // up to 256 lights!
  
  do
  {
    // Larson time baby!
    for(int i = 0; i < cycles; i++)
    {
      for (int count = 1; count < NUMPIXELS; count++) 
      {
        pixels.setPixelColor(count, color);
        old_val[count] = color;

        if (Serial.available() > 0) 
        {
          return 0;
        }
        
        for(int x = count; x > 0; x--) 
        {
          old_val[x-1] = dimColor(old_val[x-1], width);
          pixels.setPixelColor(x-1, old_val[x-1]); 
        }
        
        pixels.show();
        delay(speed);
      }
      
      for (int count = NUMPIXELS -1; count >= 0; count--) 
      {
        pixels.setPixelColor(count, color);
        old_val[count] = color;

        if (Serial.available() > 0) 
        {
          return 0;
        }
        
        for(int x = count; x <= NUMPIXELS ; x++) 
        {
          old_val[x-1] = dimColor(old_val[x-1], width);
          pixels.setPixelColor(x+1, old_val[x+1]);
        }
        
        pixels.show();
        delay(speed);
      }
    }
  }while(!Serial.available() > 0);
}

uint32_t dimColor(uint32_t color, uint8_t width) 
{
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

void rainbow(uint8_t wait) 
{
  uint16_t i, j;
  do
  {
    for(j=0; j<256; j++) 
    {
      for(i=0; i < pixels.numPixels(); i++) 
      {
        if (Serial.available() > 0) 
        {
          return 0;
        }
        
        pixels.setPixelColor(i, Wheel((i+j) & 255));
      }
      
      pixels.show();
      delay(wait);
    }
  }while(!Serial.available() > 0);
}

void rainbowCycle(uint8_t wait) 
{
  uint16_t i, j;
  do
  {
    for(j=0; j<256*5; j++) // 5 cycles of all colors on wheel
    { 
      for(i=0; i< pixels.numPixels(); i++) 
      {
        if (Serial.available() > 0) 
        {
          return 0;
        }
        
        pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
      }
      pixels.show();
      delay(wait);
    }
  }while(!Serial.available() > 0);
}

uint32_t Wheel(byte WheelPos) 
{
  WheelPos = 255 - WheelPos;
  
  if(WheelPos < 85) 
  {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  
  if(WheelPos < 170) 
  {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
