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

byte audioLevel = 1; // start off at one below max

int spectrumValue[7]; // to hold a2d values

int mode = 0;
int rollingWheelIndex = 0;
byte oddEven = 0;
int counter = 0;//used for various purposes by different modes
int brightnessFactor = 2;
int quietCounter = 0;

#define INPUT_FLOOR 80

#define MAX_VOLUME_REDUCTION 127


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

  pinMode(POT_CS, OUTPUT);
  pinMode(POT_SCK, OUTPUT);
  pinMode(POT_DATA, OUTPUT);
  digitalWrite(POT_CS, HIGH);//hold high when not writing
  adjustAudioLevel(0);
}

void loop()
{
  Serial.begin(9600);
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

  checkAudioLevels();

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
    Serial.print(spectrumValue[i]);
    Serial.print("\t");
    digitalWrite(EQ_STROBE, HIGH);
    delayMicroseconds(18);
  }
  Serial.println("");
}

// -1 to reduce level, +1 to increase level, 0 sets to max volume
void adjustAudioLevel(int change) {
  byte oldLevel = audioLevel;
  if (change > 0) {
    if (audioLevel < MAX_VOLUME_REDUCTION)
      audioLevel++;
  } else if (change < 0) {
    if (audioLevel > 0)
      audioLevel--;
  }
  Serial.print("Level: " );
  Serial.println(audioLevel);

  if(oldLevel != audioLevel)
    spi_out(audioLevel);

}

void checkAudioLevels() {
  // if the max for 3 channels is 1022, turn down if it's less than 500, turn it up
  byte up_count = 0;
  byte down_count = 0;
  for (int i = 0; i < 7; i++) {
    //Serial.print(maxValue[i]);
    //Serial.print("\t");
    if(spectrumValue[i]>1022)
      down_count++;   
    else if(spectrumValue[i]<200)
     up_count++; 
  }
  //Serial.print("\t");
 // Serial.print(up_count);
 // Serial.print("/");
 // Serial.print(down_count);
 // Serial.println("");
  if( (up_count-down_count)>3 ){
     adjustAudioLevel(-1);   
  }else if( (down_count-up_count)>3 ){
    adjustAudioLevel(1);  
  }
}

void spi_transfer(byte working) {

  for (int i = 1; i <= 8; i++) { // setup a loop of 8 iterations, one for each bit
    
    shiftOut(POT_DATA, POT_SCK, MSBFIRST, working);
   /* digitalWrite (POT_SCK, HIGH); // set clock high, the pot IC will read the bit into its register

    working = working << 1;

    digitalWrite(POT_SCK, LOW); // set clock low, the pot IC will stop reading and prepare for the next iteration (next significant bit
*/
  }

}


void spi_out( byte data_byte) { // SPI tranfer out function begins here

  digitalWrite (POT_CS, LOW); // set slave select low for a certain chip, defined in the argument in the main loop. selects the chip
  delayMicroseconds(1);
  spi_transfer(B00010001); // transfer the work byte, which is equal to the cmd_byte, out using spi

  spi_transfer(data_byte); // transfer the work byte, which is equal to the data for the pot

  digitalWrite(POT_CS, HIGH); // set slave select high for a certain chip, defined in the argument in the main loop. deselcts the chip

}






