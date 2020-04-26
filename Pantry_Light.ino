#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>


#define REED_PIN    8
#define LED_PIN     6
#define NUM_LEDS    175 //175

int brightness = 255;
int color_temp = 3450;
int seconds_until_timeout = 600;
int party_every = 100;
int superparty_every = 10;
int parties = 0;
int timer_state = 0;
unsigned long time_turned_on_at;
unsigned long time_now;
unsigned long total_time_on;


#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

int colors_red, colors_green, colors_blue;
int state = 0;
int led_state = 0;
int opens = 0;

void setup() {
    delay(300); // power-up safety delay
    pinMode(REED_PIN, INPUT_PULLUP);
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.setCorrection(CRGB(255,235,248)); //Changed from 255, 224, 248 on 3.28.20
    // Turn our current led on to white, then show the leds
    kToRGB(color_temp);
    // Color temp mode- convert colors' relative values to incorporate brightnesss
    colors_red = colors_red*(brightness/255.);
    colors_green = colors_green*(brightness/255.);
    colors_blue = colors_blue*(brightness/255.);
}


void loop() {
  if (lights_should_be_on()){
    if (led_state == 0) {
      opens += 1;
      if (opens > 0 and opens % party_every == 0) {
        party(parties);
        parties += 1;
      }
      lights_on();
      led_state = 1;
    }
  }
  else {
    lights_off();
    led_state = 0;
  }
}

bool lights_should_be_on(){
  if (door_open()){
    time_now = millis();
    total_time_on = (time_now - time_turned_on_at) / 1000;
    if (timer_state == 0 or total_time_on < seconds_until_timeout) {
        return true;
    }                     
    else {
      return false;
    }
  }
  else {
    return false; 
  }
}

void lights_on(){
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    leds[whiteLed] = CRGB(colors_red, colors_green, colors_blue);
    // Show the leds
    FastLED.show();
  }
  time_turned_on_at = millis();
  timer_state = 1;
}

void lights_off() {
  FastLED.clear();
  FastLED.show(); 
}

void party(int parties) {
  // For every 1000th open, blink red, Caitlin's fav :]
  if (parties > 0 and parties % superparty_every == 0) {
    superparty();
  }
  
  else {
    for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      leds[whiteLed] = CRGB(220, 150, 25);
      // Show the leds
      FastLED.show();
    }
    for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      leds[whiteLed] = CRGB(242, 1, 115);
      // Show the leds
      FastLED.show();
    }
    for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      leds[whiteLed] = CRGB(75, 10, 241);
      // Show the leds
      FastLED.show();
    }
  }
}

void superparty() {
  for(int redLed = 0; redLed < NUM_LEDS; redLed += 2) {
    leds[redLed] = CRGB(255, 1, 10);
    // Show the leds
    FastLED.show();
  }
  for(int redLed = 1; redLed < NUM_LEDS; redLed += 2) {
    leds[redLed] = CRGB(255, 1, 10);
    // Show the leds
    FastLED.show();
  }
  delay(75);

  // Blink red 5 times
  for(int blink_n = 1; blink_n < 6; blink_n++) {
    lights_off();
    delay(50);
    fill_solid(leds, NUM_LEDS, CRGB(255,0,0));
    FastLED.show();
    delay(50);
  }
}

boolean door_open(){
  float cum_high_ct = 0.0;
  int proximity;
  
  // High frequency filter (LPF)
  for(int lpf_count = 0; lpf_count < 5; lpf_count++) {
    // Read the reed switch and report its state
    proximity = digitalRead(REED_PIN);
    cum_high_ct += proximity;
    delay(75);
  }
  
  if (cum_high_ct >= 4) {
    return true;
  }
  timer_state = 0;
  return false;
}

void kToRGB(float temp){
  float red,green,blue;

  // Run the calibration routine on the color temperature for this LED strip
  temp = temp - 1250;

  // Use algorithm from Tanner (see sources at bottom) to convert kelvin to RGB
  temp = temp / 100;

  if (temp <= 66) {
    // red
    red = 255;

    // green
    green = temp;
    green = 99.4708025861 * log(green) - 161.1195681661;
  } else {
    // red
    red = temp - 60;
    red = 329.698727446 * pow(red,-0.1332047592);

    // green
    green = temp - 60;
    green = 288.1221695283 * pow(green,-0.0755148492);
  }

  // Handle blue seperately
  if (temp >= 66) {
    blue = 255;
  } else {
    if (temp <= 19) {
      blue = 0;
    } else {
      blue = temp - 10;
      blue = 138.5177312231 * log(blue) - 305.0447927307;
    }
  }
  colors_red = (int) round(min(255,max(red,0)));
  colors_green = (int) round(min(255,max(green,0)));
  colors_blue = (int) round(min(255,max(blue,0)));
}
