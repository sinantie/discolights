#include <arduinoFFT.h>
#include <Adafruit_NeoPixel.h>
//#ifdef __AVR__
//#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
//#endif

// Function button pin
#define FUNC_PIN  2
// Which pin on the Arduino is connected to the NeoPixels?
#define NEO_PIN        6 // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 24 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

#define SAMPLES 128             //Must be a power of 2
#define SAMPLING_FREQUENCY 8000 //Hz, must be less than 10000 due to ADC
#define DEBUG false

// Push button parameters
int buttonState, reading;            // the current reading from the input pin
int lastButtonState = HIGH;  // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// function modes
typedef enum {
  FFT_FULL_BRIGHT = 0,  // FFT at full brightness (80%)
  FFT_MED_BRIGHT = 1,   // FFT at medium brightness (50%)
  FLASHING_FULL_BRIGHT = 2,         // VU meter mode
  FLASHING_MED_BRIGHT  = 3         // all-LEDs-flash-at-once mode
} FUNCTION;
#define NUM_FUNCTIONS 4

int currentFunction = FFT_FULL_BRIGHT;

// 9216 / 128 = 72 /16 = 24
arduinoFFT FFT = arduinoFFT();

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];


void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  pixels.clear(); // Set all pixel colors to 'off'

  pinMode(FUNC_PIN, INPUT_PULLUP); // Init function button
}


void loop() {

  /**Handle Function Button*/
  reading = digitalRead(FUNC_PIN);
  if (reading != lastButtonState) {    
    lastDebounceTime = millis(); // reset the debouncing timer
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {    
    if (reading != buttonState) { // if the button state has changed
      buttonState = reading;

      // only switch mode if the new button state is HIGH
      if (buttonState == LOW) {
        currentFunction = (currentFunction + 1) % NUM_FUNCTIONS;        
      }
    }
  }  
  lastButtonState = reading; // save the reading. Next time through the loop, it'll be the lastButtonState  

  fftMode(currentFunction);  
}

void fftMode(FUNCTION functionMode) {   
  /*SAMPLING*/
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();    //Overflows after around 70 minutes!

    vReal[i] = analogRead(A0);
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) {
    }
  }

  /*FFT*/
  //  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  //  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  //  Serial.print("off");
  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.

  if (functionMode == FLASHING_FULL_BRIGHT || functionMode == FLASHING_MED_BRIGHT) {
    int band = 1;
    neoPixelConditionalColor(vReal[band], 0, true);
    double value = (vReal[band] / 1024) * 100;    
    pixels.setBrightness(functionMode == FLASHING_FULL_BRIGHT ? value : 10);
    pixels.show();   // Send the updated pixel colors to the hardware.
  }
  else {
    for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
      int shine = i + 1;
      neoPixelConditionalColor(vReal[shine], i, false);    
      pixels.setBrightness(functionMode == FFT_FULL_BRIGHT ? 100 : 30);
      pixels.show();   // Send the updated pixel colors to the hardware.
      //    delay(500); // Pause before next pass through loop        
      if (DEBUG) {
        /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/
        Serial.print(shine);
        Serial.print(": ");
        Serial.print((shine * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
        Serial.print("Hz ");
        Serial.println(vReal[shine], 1);    // View only this line in serial plotter to visualize the bins  
      }    
    }
  }
}

void neoPixelConditionalColor(double value, int i, bool fill) {
  if (value <= 50) {
      neoPixelConditionalColor(i, pixels.Color(0, 0, 0), fill); // black
    }
  if (value > 50 && value <= 200) {
      neoPixelConditionalColor(i, pixels.Color(0, 0, 153), fill); // blue
    }
  if (value > 200 && value <= 300) {
    neoPixelConditionalColor(i, pixels.Color(102, 0, 204), fill); // purple
  }

  if (value > 301 && value <= 500) {
    neoPixelConditionalColor(i, pixels.Color(0, 153, 76), fill); // green
  }
  if (value > 501 && value <= 700) {
    neoPixelConditionalColor(i, pixels.Color(255, 255, 0), fill); // yellow
  }
  if (value > 701 && value <= 1000) {
    neoPixelConditionalColor(i, pixels.Color(255, 128, 0), fill); // orange
  }
  if (value >  1000) {
    neoPixelConditionalColor(i, pixels.Color(255, 0, 0), fill); // red
  }
}

void neoPixelConditionalColor(int i, uint32_t color, bool fill) {
  if (fill) {
    pixels.fill(color);
  }
  else {
    pixels.setPixelColor(i, color);
  }
}

void testNeopixels() {
     // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  for (int i = 0; i < NUMPIXELS - 5; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(0, 0, 153)); // blue
    pixels.setPixelColor(i+1, pixels.Color(102, 0, 204)); // purple
    pixels.setPixelColor(i+2, pixels.Color(0, 153, 76)); // green
    pixels.setPixelColor(i+3, pixels.Color(255, 255, 0)); // yellow
    pixels.setPixelColor(i+4, pixels.Color(255, 128, 0)); // orange
    pixels.setPixelColor(i+5, pixels.Color(255, 0, 0)); // red
    pixels.setBrightness(30);
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(500); // Pause before next pass through loop
  }
}
