#include "FastLED.h"

// ColorWavesWithPalettes
// Animated shifting color waves, with several cross-fading color palettes.
// by Mark Kriegsman, August 2015
//
// Color palettes courtesy of cpt-city and its contributors:
//   http://soliton.vm.bytemark.co.uk/pub/cpt-city/
//
// Color palettes converted for FastLED using "PaletteKnife" v1:
//   http://fastled.io/tools/paletteknife/
//

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif
#include "palettes.h"

#define DATA_PIN    7
//#define CLK_PIN   14
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    100
#define BRIGHTNESS  255

struct LEDStruct {
  CRGB strip[NUM_LEDS];
  CRGBPalette16 gCurrentPalette;
  CRGBPalette16 gTargetPalette;
  uint8_t gCurrentPaletteNumber; 
};

struct LEDStruct leds;
//CRGB leds[NUM_LEDS];

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
#define SECONDS_PER_PALETTE 30


void setup() {
  delay(2000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
//  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS)
  FastLED.addLeds<LED_TYPE,DATA_PIN, COLOR_ORDER>(leds.strip, NUM_LEDS)
    //.setCorrection(TypicalLEDStrip) // cpt-city palettes have different color balance
    .setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);


// Forward declarations of an array of cpt-city gradient palettes, and 
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
leds.gCurrentPaletteNumber = 0;

leds.gCurrentPalette = gGradientPalettes[0] ;
leds.gTargetPalette = gGradientPalettes[1] ;
}

void loop()
{
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    leds.gCurrentPaletteNumber = random8( 0, gGradientPaletteCount+1);
    //leds.gCurrentPaletteNumber = addmod8( leds.gCurrentPaletteNumber, 1, gGradientPaletteCount);
    leds.gTargetPalette = gGradientPalettes[ leds.gCurrentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( leds.gCurrentPalette, leds.gTargetPalette, 16);
  }
  
  colorwaves( leds );

  FastLED.show();
  FastLED.delay(20);
}


// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
//void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) 
void colorwaves( LEDStruct& ledarray)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( ledarray.gCurrentPalette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( ledarray.strip[pixelnumber], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette 
// across the defined LED strip.
//void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
//{
//  static uint8_t startindex = 0;
//  startindex--;
//  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
//}





// Gradient Color Palette definitions for 33 different cpt-city color palettes.
//    956 bytes of PROGMEM for all of the palettes together,
//   +618 bytes of PROGMEM for gradient palette code (AVR).
//  1,494 bytes total for all 34 color palettes and associated code.

// From ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb

// Gradient Color Palette definitions for 33 different cpt-city color palettes.
//    956 bytes of PROGMEM for all of the palettes together,
//   +618 bytes of PROGMEM for gradient palette code (AVR).
//  1,494 bytes total for all 34 color palettes and associated code.

// Gradient palette "ib_jul01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/xmas/tn/ib_jul01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.
