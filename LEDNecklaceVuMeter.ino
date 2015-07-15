/** Angular Teardrop Necklace by Jeremy Hull jchull@gmail.com **/

#include "FastLED.h"

#define LED_PIN 11
#define CLOCK_PIN 10
#define CHIPSET APA102
#define EQ_READ 0 // read from multiplexer using analog input 0
#define EQ_STROBE 2 // strobe is attached to digital pin 2
#define EQ_RESET 3 // reset is attached to digital pin 3

#define POT_SCK 13
#define POT_CS 20
#define POT_DATA 7

#define P0_ADD 0x00
#define P0_INC 0x04
#define P0_DEC 0x08

byte audioLevel = 1; // start off at one below max

int spectrumValue[7]; // to hold a2d values

int mode = 0;
int rollingWheelIndex = 0;
byte oddEven = 0;
int counter = 0;//used for various purposes by different modes
int brightnessFactor = 2;
int quietCounter = 0;

#define INPUT_FLOOR 80

#define MAX_VOLUME_REDUCTION 128


// Params for width and height
const uint8_t matrixWidth = 7;
const uint8_t matrixHeight = 5;

#define NUM_LEDS (matrixWidth * matrixHeight + 4)
CRGB leds[NUM_LEDS];

// pixel map
unsigned int matrix[7][5] =
{
  {
    9, 8, 7, 6, 5
  }
  ,
  {
    14, 13, 12, 11, 10
  }
  ,
  {
    19, 18, 17, 16, 15
  }
  ,
  {
    24, 23, 22, 21, 20
  }
  ,
  {
    29, 28, 27, 26, 25
  }
  ,
  {
    34, 33, 32, 31, 30
  },
  {
    39, 38, 37, 36, 35
  }
};


void setup()
{
  //analogReference(INTERNAL);
  pinMode(EQ_READ, INPUT);
  pinMode(EQ_STROBE, OUTPUT);
  pinMode(EQ_RESET, OUTPUT);
  analogReference(DEFAULT); //inernal = 1.1v, external = aref


  digitalWrite(EQ_RESET, LOW);
  digitalWrite(EQ_STROBE, HIGH);

  FastLED.addLeds<CHIPSET, LED_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS).setCorrection(0xFFFFFF);
  FastLED.setBrightness(255 / brightnessFactor); //0-255

  
}

void loop()
{
//  Serial.begin(9600);
  readLevels();
  updateMatrix();
 
  rollingWheelIndex ++;
  if (rollingWheelIndex > 254)
    rollingWheelIndex = 0;
}

void updateMatrix() {
  for (int x = 0; x < matrixWidth; x++) { // 7 columns of pixels
    for (int y = 0; y < matrixHeight; y++) {
      if ( (spectrumValue[x] > INPUT_FLOOR) && spectrumValue[x] > (1023/matrixHeight)*(y+1) )
        leds[matrix[x][y]] = CHSV( (rollingWheelIndex + (10 * y)) % 255, 255, 255);
      else
        leds[matrix[x][y]] = CRGB(0, 0, 0); //OFF
    }
  }
  // set the simple color for 0-4
  for (int a = 0; a < 5; a++) {
    leds[a]  = CHSV( rollingWheelIndex, 255, 255);
  }

  FastLED.setBrightness(255 / brightnessFactor);
  FastLED.show();
  FastLED.delay(25);


}

void readLevels() {
  digitalWrite(EQ_RESET, HIGH);
  delay(1); //100ns reset pulse width
  digitalWrite(EQ_RESET, LOW);
  delayMicroseconds(72); //Reset to strobe delay 72us
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(EQ_STROBE, LOW);
    delayMicroseconds(36); // 36us required to allow the output to settle
    spectrumValue[i] = analogRead(EQ_READ);
    spectrumValue[6] = spectrumValue[6]*3;
//    Serial.print(spectrumValue[i]);
//    Serial.print("\t");
    digitalWrite(EQ_STROBE, HIGH);
    delayMicroseconds(18);
  }
//  Serial.println("");
}













