#include "FastLED.h"
#include "Bounce2.h"	

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
#define switchA 3

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

// KY-040 Rotary Module variables
int pinA = 21;
int pinB = 22;
int pinSW = 23;
int lastPinSWstate = 1;
int pinSWstate;
int pinALast;
int aVal;
int rotateCount = 0;
int rotary_function = 0;
int palette_index;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
Bounce debouncer = Bounce();

void setup() {
  delay(2000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
//  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS)
  FastLED.addLeds<LED_TYPE,DATA_PIN, COLOR_ORDER>(leds.strip, NUM_LEDS)
    //.setCorrection(TypicalLEDStrip) // cpt-city palettes have different color balance
    .setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // Set up switches
  pinMode(switchA, INPUT_PULLUP);

  // Set up rotary encoder
  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinSW, INPUT_PULLUP); // or pinMode(pinSW,INPUT); if I use the 10k resistor
  pinALast = digitalRead(pinA);
  debouncer.attach(pinSW);
  debouncer.interval(100);

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



  checkDial();

  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
	  if (digitalRead(switchA)) {
		  leds.gCurrentPaletteNumber = random8(0, gGradientPaletteCount + 1);
		  //leds.gCurrentPaletteNumber = addmod8( leds.gCurrentPaletteNumber, 1, gGradientPaletteCount);
		  leds.gTargetPalette = gGradientPalettes[leds.gCurrentPaletteNumber];
	  }
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


void checkDial() {
	//debouncer.update();
	//if (debouncer.fell()) { pinSWstate = 0; }
	//else { pinSWstate = 1; }

	//if (!pinSWstate) {	// If pinSW was pressed, update what the dial does
	//	rotary_function += 1;
	//	if (rotary_function > 3) {	// If above max rotary modes, loop back to 0
	//		rotary_function = 0;
	//	}
	//	//Serial.print("Button Function: ");	// Add back if we use Serial data again
	//	//Serial.println(rotary_function);
	//}

	aVal = digitalRead(pinA);   // Read pinA
	if ((aVal != pinALast)) {//&&(aVal==LOW)){      // If pinA has changed, update things.   Added the &&
		rotateCount = !rotateCount;   // If at 0, change to 1... if at 1 change to 0 and don't update.
		if (rotateCount) {    // Need to let it change twice
			switch (rotary_function) {

			case 0: // If button is in stage 0:  Increase or decrease mode based on case order
				if (digitalRead(pinB) != aVal) {
					leds.gCurrentPaletteNumber = addmod8(leds.gCurrentPaletteNumber, 1, gGradientPaletteCount);
					leds.gTargetPalette = gGradientPalettes[leds.gCurrentPaletteNumber];
				}
				else {
          if (leds.gCurrentPaletteNumber = 0){
            leds.gCurrentPaletteNumber = gGradientPaletteCount;
          }else{
					  leds.gCurrentPaletteNumber = submod8(leds.gCurrentPaletteNumber, 1, gGradientPaletteCount);
					  leds.gTargetPalette = gGradientPalettes[leds.gCurrentPaletteNumber];
				  }
				}
				break;

			//case 1: // If button is in stage 1:		Update palettes based on palette index 
			//	if (digitalRead(pinB) != aVal) {
			//		if (transitioning == 0) {
			//			palette_change(actual_leds, 1);
			//		}
			//		else {
			//			palette_change(new_leds, 1);
			//			palette_change(old_leds, 1);
			//		}
			//	}
			//	else {
			//		if (transitioning == 0) {
			//			palette_change(actual_leds, 0);
			//		}
			//		else {
			//			palette_change(new_leds, 0);
			//			palette_change(old_leds, 0);
			//		}
			//	}
			//	Serial.print("Palette number: ");
			//	Serial.println(palette_index);
			//	break;

			//case 2:		// If button is in stage 2:		Adjust delay speed   TODO: consider proportionally updating new and old speeds?
			//	if (digitalRead(pinB) != aVal) {
			//		if (transitioning > 0) {
			//			delay_change(old_leds, 0);
			//			delay_change(new_leds, 0);
			//		}
			//		else {
			//			delay_change(actual_leds, 0);
			//		}
			//	}
			//	else {
			//		if (transitioning > 0) {
			//			delay_change(old_leds, 1);
			//			delay_change(old_leds, 1);
			//		}
			//		else {
			//			delay_change(actual_leds, 1);
			//		}
			//	}
			//	Serial.print("Delay: ");
			//	Serial.println(actual_leds.this_delay);
			//	break;

			//case 3:		// If button in stage 3:	Adjust brightness
			//	if (digitalRead(pinB) != aVal) {
			//		overall_bright++;
			//	}
			//	else {
			//		overall_bright--;
			//	}
			//	constrain(overall_bright, 0, max_bright);
			//	FastLED.setBrightness(overall_bright);
			//	Serial.print("Brightness: ");
			//	Serial.println(overall_bright);
			//	break;

			}
		}
	}
	pinALast = aVal;
}
