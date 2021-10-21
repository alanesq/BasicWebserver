//
//    neopixel demo of using VECTOR and CLASS - 20oct21
//

const bool serialDebug = 1;
#include <vector>      // https://github.com/janelia-arduino/Vector/blob/master


//            --------------------------- settings -------------------------------


  #define NUM_NEOPIXELS   60        // Number of LEDs in the string
  
  #define NEOPIXEL_PIN    5         // Neopixel data gpio pin (Note: should have 330k resistor ideally) (5 = D1 on esp8266)

  int g_PowerLimit =      800;      // power limit in milliamps for Neopixels (USB usually good for 800)

  int g_Brightness =      32;       // LED maximum brightness scale (1-255)

  int iLED = 22;                    // onboard indicator led
  

//            --------------------------------------------------------------------


//#define FASTLED_INTERRUPT_RETRY_COUNT 1       // attempt to stop the first led blinking when using esp8266
#define FASTLED_INTERNAL                      // Suppress build banner
#include <FastLED.h>                          // https://github.com/FastLED/FastLED
CRGB g_LEDs[NUM_NEOPIXELS] = {0};             // Frame buffer for FastLED


// ----------------------------------------------------------------
//                            -Oscillators
// ---------------------------------------------------------------- 
// generate oscillating pixels
//  vector info: https://www.codeguru.com/cplusplus/c-tutorial-a-beginners-guide-to-stdvector-part-1/

class Oscillators {
  
  private:
    CRGB* oLEDarray;                // led data location
    int oMaxNum;                   // Maximum number of oscillators
    int oCounter = 0;              // Number of Oscillators running
    std::vector<int> oPosition;    // Oscillator position
    std::vector<bool> oDirection;  // Oscillator direction
    std::vector<CRGB> oColour;     // Oscillator colour
    
  public:
    Oscillators(CRGB* o_LEDarray, size_t o_maxNum) {
      this->oLEDarray = o_LEDarray;
      this->oMaxNum = o_maxNum;
      addone();
    }

    ~Oscillators() {
    }

    void addone() {
      if (oCounter == oMaxNum) return;
      oCounter ++;
      if (serialDebug) Serial.println("Adding one - " + String(oCounter));
      oColour.push_back(CHSV(random(255), 255, 255));
      (random(2) == 1) ? oDirection.push_back(0) : oDirection.push_back(1);
      oPosition.push_back(random(NUM_NEOPIXELS-2)+1);
    }

    void removeone() {
      if (oCounter == 0) return;
      oCounter --;
      if (serialDebug) Serial.println("Removing one - " + String(oCounter));
      oColour.pop_back();
      oDirection.pop_back();
      oPosition.pop_back();
    }    

    void move() {
      for(int i=0; i<oPosition.size(); i++) {
        oDirection[i] ? oPosition[i]++ : oPosition[i]--;
        if (oPosition[i] == 0  || oPosition[i] == NUM_NEOPIXELS-1) oDirection[i]=!oDirection[i];  // reverse direction
      }
    }

    void show() {
      for(int i=0; i<oPosition.size(); i++) {
        oLEDarray[oPosition[i]] = oColour[i];
      }
    }
}; 


// ----------------------------------------------------------------
//                            -Setup
// ---------------------------------------------------------------- 

void setup() {
  Serial.begin(serialSpeed); while (!Serial); delay(50);       // start serial comms   
  Serial.println("\n\n\n\Starting\n");

  pinMode(iLED, OUTPUT);
  
  if (serialDebug) Serial.println("Initialising neopixels");
  pinMode(NEOPIXEL_PIN, OUTPUT);
  FastLED.addLeds<WS2812B, NEOPIXEL_PIN, GRB>(g_LEDs, NUM_NEOPIXELS);   // Add the LED strip to the FastLED library
  FastLED.setBrightness(g_Brightness);
  //set_max_power_indicator_LED(2);                                       // Light the builtin LED if we power throttle
  FastLED.setMaxPowerInMilliWatts(g_PowerLimit);                        // Set the power limit, above which brightness will be throttled

}

// ----------------------------------------------------------------
//                            -Loop
// ---------------------------------------------------------------- 

void loop() {

  // Oscillators test
    static Oscillators otest(&g_LEDs[0], 5);
    EVERY_N_MILLISECONDS(50) {          // refresh display every 50ms
      FastLED.clear(false);  
      otest.move();
      otest.show();      
      FastLED.show(g_Brightness); 
    }
    EVERY_N_MILLISECONDS(2000) {        // change number of oscillators every 2 seconds
      if (random(2) == 1) otest.addone();
      else otest.removeone();    
    }


  EVERY_N_MILLISECONDS(1000) { 
    digitalWrite(iLED, !digitalRead(iLED));
  }
    
}

// ---------------------------------------------------------------- 
