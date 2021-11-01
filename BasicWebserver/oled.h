/**************************************************************************************************
 *  
 *      OLED display Menu System - i2c version SSD1306 - 01Nov21
 * 
 *      part of the BasicWebserver sketch
 *                                   
 * 
 **************************************************************************************************
      
 The sketch displays a menu on the oled and when an item is selected it sets a 
 flag and waits until the event is acted upon.  Max menu items on a 128x64 oled 
 is four.
 
See the section "customise the menus below" for how to create custom menus inclusing selecting a value, 
choose from a list or display a message.
 
 Notes:   text size 1 = 21 x 8 characters
          text size 2 = 10 x 4

 for more oled info    see: https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/
                    

 
 **************************************************************************************************/

//                                        Settings

//    // esp8266
//    #define encoder0PinA  14                  // Rotary encoder gpio pin, 14 = D5 on esp8266 
//    #define encoder0PinB  12                  // Rotary encoder gpio pin, 12 = D6 on esp8266
//    #define encoder0Press 13                  // Rotary encoder button gpio pin, 13 = D7 on esp8266
//    #define OLEDC 0                           // oled clock pin (set to 0 for default) 
//    #define OLEDD 0                           // oled data pin

    //  esp32 lolin lite
    #define encoder0PinA  25                  // Rotary encoder gpio pin 
    #define encoder0PinB  33                  // Rotary encoder gpio pin 
    #define encoder0Press 32                  // Rotary encoder button gpio pin 
    #define OLEDC 26                          // oled clock pin (set to 0 for default) 
    #define OLEDD 27                          // oled data pin

    int OLEDDisplayTimeout = 10;              // inactivity trigger time on rotary encoder (seconds)
    int itemTrigger = 1;                      // rotary encoder - counts per tick (varies between encoders usually 1 or 2)


 //  ************************************************************************************************  
 

// forward declarations 
    void menuItemActions();
    void Main_Menu();
    void menu2();
    bool confirmActionRequired();  
    void ICACHE_RAM_ATTR doEncoder();
    void setMenu(int, String);
    int enterValue(String, int, int, int, int);
    void menuItemSelection();
    bool menuCheck();
    void staticMenu();
    int chooseFromList(int, String, String[]);
    void reWaitKeypress(int);
    void storeSettings();
    void exitMenu(); 
    void displayTimeOLED();

  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>

  // rotary encoder  
    volatile int encoder0Pos = 0;             // current value selected with rotary encoder (updated in interrupt routine)
    volatile bool encoderPrevA;               // used to debounced rotary encoder 
    volatile bool encoderPrevB;               // used to debounced rotary encoder 
    bool reButtonState = 1;                   // current debounced state of the button
    uint32_t reButtonTimer = 0;               // time button state last changed
    int reButtonMinTime = 500;                // minimum milliseconds between allowed button status changes
      
  // oled menu
    // int itemTrigger = 2;                      // rotary encoder - counts per tick
    const int topLine = 18;                   // y position of second bank of menu display 
    const byte menuMax = 5;                   // max number of menu items
    const byte lineSpace1 = 9;                // line spacing textsize 1
    const byte lineSpace2 = 17;               // line spacing textsize 2
    String menuOption[menuMax];               // options displayed in menu
    byte menuCount = 0;                       // which menu item is curently highlighted 
    String menuTitle = "";                    // current menu ID number (blank = none)
    byte menuItemClicked = 100;               // menu item has been clicked flag (100=none)
    uint32_t lastREActivity = 0;              // time last activity was seen on rotary encoder
  
  // oled SSD1306 display connected to I2C (SDA, SCL pins)
    #define OLED_ADDR 0x3C                    // OLED i2c address
    #define SCREEN_WIDTH 128                  // OLED display width, in pixels
    #define SCREEN_HEIGHT 64                  // OLED display height, in pixels
    #define OLED_RESET -1                     // Reset pin # (or -1 if sharing Arduino reset pin)
    // #define OLED_RESET LED_BUILTIN           // I have seen this is required for esp8266?
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// -------------------------------------------------------------------------------------------------


// called from setup 

void oledSetup() {

  // configure gpio pins
    pinMode(encoder0Press, INPUT_PULLUP);
    pinMode(encoder0PinA, INPUT);
    pinMode(encoder0PinB, INPUT);

  // initialise the oled display
    if (OLEDC == 0) Wire.begin();
    else Wire.begin(OLEDD, OLEDC);
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
      Serial.println(("\nError initialising the oled display"));
    }

  // Interrupt for reading the rotary encoder position
    encoder0Pos = 0;   
    attachInterrupt(digitalPinToInterrupt(encoder0PinA), doEncoder, CHANGE); 

  Main_Menu();    // start the main menu displaying - see menuItemActions() to edit the menus
}


// -------------------------------------------------------------------------------------------------


// called from loop - service the menus

void oledLoop() {
    // if a oled menu is active service it 
    if (menuTitle != "") {                                  // if a menu is active
      menuCheck();                                          // check if encoder selection button is pressed
      menuItemSelection();                                  // check for change in menu item highlighted
      staticMenu();                                         // display the menu 
      menuItemActions();                                    // act if a menu item has been clicked
    } 
}


/* -------------------------------------------------------------------------------------------------
//                                        customise the menus below
// -------------------------------------------------------------------------------------------------
// Useful commands:
//      void reWaitKeypress(20000);         = wait for the button to be pressed on the rotary encoder (timeout in 20 seconds if not)
//      chooseFromList(8, "TestList", q);   = choose from the list of 8 items in a string array 'q'
//      enterValue("Testval", 15, 0, 30);   = enter a value between 0 and 30 (with a starting value of 15)


/*      Usage Examples

display.drawPixel(10, 10, WHITE);

display.drawLine(0, 22, display.width(), 32, WHITE);

display.drawRect(10, 34, 108, 28, WHITE);    

// displaying a bitmap image
//    display with: display.drawBitmap((display.width() - LOGO_WIDTH ) / 2, (display.height() - LOGO_HEIGHT) / 2, logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
//    utility for creating the data: http://javl.github.io/image2cpp/ or http://en.radzio.dxp.pl/bitmap_converter/
  #define LOGO_HEIGHT   16
  #define LOGO_WIDTH    16
  static const unsigned char PROGMEM logo_bmp[] =
  { B00000000, B11000000,
    B00000001, B11000000,
    B00000001, B11000000,
    B00000011, B11100000,
    B11110011, B11100000,
    B11111110, B11111000,
    B01111110, B11111111,
    B00110011, B10011111,
    B00011111, B11111100,
    B00001101, B01110000,
    B00011011, B10100000,
    B00111111, B11100000,
    B00111111, B11110000,
    B01111100, B11110000,
    B01110000, B01110000,
    B00000000, B00110000 };
*/

  
// Available Menus

  // main menu
  void Main_Menu() {
      menuTitle = "Main Menu";                                     // set the menu title
      setMenu(0,"");                                               // clear all menu items
      setMenu(0,"list");                                           // choose from a list
      setMenu(1,"enter a value");                                  // enter a value
      setMenu(2,"message");                                        // display a message
      setMenu(3,"MENU 2");                                         // change menu
  }

  
  // menu 2
  void menu2() {
      menuTitle = "Menu 2";  
      setMenu(0,""); 
      setMenu(0,"Time");
      setMenu(1,"menu off");
      setMenu(2,"RETURN");
  }

   
    
//  -------------------------------------------------------------------------------------------
//  ------------------------------------- menu procedures -------------------------------------
//  -------------------------------------------------------------------------------------------


void menuItemActions() {
    
  if (menuItemClicked == 100) return;                                 // if no menu item has been clicked exit function


  //  --------------------- Main Menu Actions ------------------

    // choose from list
    if (menuTitle == "Main Menu" && menuItemClicked==0) {
      menuItemClicked=100;                                            // flag that the button press has been actioned (the menu stops and waits until this)             
      String q[] = {"Confirm","item1","item2","item3","item4","item5","item6","CANCEL"};
      int tres=chooseFromList(8, "TestList", q);
      if (tres==0 && confirmActionRequired()) {
        log_system_message("long press confirmed on item0");
      }
      if (tres==1) {
        log_system_message("item1 selected");
      }      
      log_system_message("Menu: item " + String(tres) + " chosen from list");
    }

    // enter a value
    if (menuTitle == "Main Menu" && menuItemClicked==1) {
      menuItemClicked=100;  
      int tres=enterValue("Testval", 15, 1, 0, 30);                      // enter a value (title, start value, step size, low limit, high limit)
      log_system_message("Menu: Value set = " + String(tres));
    }

    // display a message
    if (menuTitle == "Main Menu" && menuItemClicked==2) {
      menuItemClicked=100;     
      log_system_message("Menu: display message selected");
      // display a message
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(20, 20);
        display.print("Hello");
        display.display();
      reWaitKeypress(20000);                                          // wait for key press on rotary encoder
    }

    // switch to menu 2
    if (menuTitle == "Main Menu" && menuItemClicked==3) {
      menuItemClicked=100;                                            
      log_system_message("Menu: Menu 2 selected");
      menu2();                                                        // show a different menu
    }

    
  //  --------------------- Menu 2 Actions ---------------------

  
    // show time/date
    if (menuTitle == "Menu 2" && menuItemClicked==0) {
      menuItemClicked=100;                                            
      log_system_message("Menu: Time display selected");
      displayTimeOLED();
      reWaitKeypress(8000);                                           // wait for key press
    }
    
    // turn menus off
    if (menuTitle == "Menu 2" && menuItemClicked==1) {
      menuItemClicked=100;                                            
      log_system_message("Menu: menu off");
      menuTitle = "";                                                 // turn menu off
      display.clearDisplay();
      display.display(); 
    }

    // switch to main menu
    if (menuTitle == "Menu 2" && menuItemClicked==2) {
      menuItemClicked=100;                                            
      log_system_message("Menu: Main Menu selected");
      Main_Menu();                                                    // show main menu
    }

}


//  --------------------------------------


// confirm that a requested action is not a mistake
//   returns 1 if confirmed

bool confirmActionRequired() {
  display.clearDisplay();  
  display.setTextSize(2);
  display.setTextColor(WHITE,BLACK);
  display.setCursor(0, lineSpace2 * 0);
  display.print("HOLD");
  display.setCursor(0, lineSpace2 * 1);
  display.print("BUTTON TO");  
  display.setCursor(0, lineSpace2 * 2);
  display.print("CONFIRM!");
  display.display();
  
  delay(1500);
  display.clearDisplay();  
  display.display();

  if (digitalRead(encoder0Press) == LOW) return 1;        // if button still pressed
  return 0;
}


//  --------------------------------------


// display menu on oled

void staticMenu() {
  display.clearDisplay(); 
  // title
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(lineSpace1, 0);
    display.print(menuTitle);
    display.drawLine(0, lineSpace1, display.width(), lineSpace1, WHITE);

  // menu options
    int i=0;
    while (i < menuMax && menuOption[i] != "") {                           // if menu item is not blank display it
      if (i == menuItemClicked) display.setTextColor(BLACK,WHITE);         // if this item has been clicked
      else display.setTextColor(WHITE,BLACK);
      display.setCursor(10, topLine + (i*lineSpace1));
      display.print(menuOption[i]);
      i++;
    }

  // highlighted item if none yet clicked
    if (menuItemClicked == 100) {
      display.setCursor(2, topLine + (menuCount * lineSpace1));
      display.print(">");
    }
 
  display.display();          // update display
}


//  --------------------------------------


// rotary encoder button
//    returns 1 if the button status has changed since last time

bool menuCheck() {

  if (digitalRead(encoder0Press) == reButtonState) return 0;    // no change
  delay(20);
  if (digitalRead(encoder0Press) == reButtonState) return 0;    // debounce
  if (millis() - reButtonTimer  < reButtonMinTime) return 0;    // if too soon since last change

  // button status has changed
  reButtonState = !reButtonState;   
  reButtonTimer = millis();                                     // update timer
  noInterrupts();                                               // reset rotary encoder position counter back to zero
    encoder0Pos = 0;       
  interrupts();

  // oled menu action on button press 
  if (reButtonState==LOW) {                                     // if button is now pressed      
    lastREActivity = millis();                                  // log time last activity seen (don't count button release as activity)
    if (menuItemClicked != 100 || menuTitle == "") return 1;    // menu item already selected or there is no live menu
    if (serialDebug) Serial.println("menu '" + menuTitle + "' item " + String(menuCount) + " selected");
    menuItemClicked = menuCount;                                // set item selected flag
  }

  return 1;
}

//  --------------------------------------


// wait for key press or turn on rotary encoder
//    pass timeout in ms
void reWaitKeypress(int timeout) {          
        uint32_t tTimer = millis();   // log time
        // wait for button to be released
          while ( (digitalRead(encoder0Press) == LOW) && (millis() - tTimer < timeout) ) {        // wait for button release
            delay(20);
          }
        // clear rotary encoder position counter
          noInterrupts();
            encoder0Pos = 0;       
          interrupts();
        // wait for button to be pressed or encoder to be turned
          while ( (digitalRead(encoder0Press) == HIGH) && (encoder0Pos < itemTrigger && encoder0Pos > -itemTrigger) && (millis() - tTimer < timeout) ) {
            delay(10);
          }
}


//  --------------------------------------


// handle menu item highlighted via rotary encoder
//      note:       int instep = !(encoder0Pos%itemTrigger);           // to check the encoder counter is not out of step

void menuItemSelection() {
    if (encoder0Pos >= itemTrigger) {
      noInterrupts();
        encoder0Pos -= itemTrigger;
      interrupts();
      lastREActivity = millis();                            // log time last activity seen
      if (menuCount+1 < menuMax) menuCount++;               // if not past max menu items move
      if (menuOption[menuCount] == "") menuCount--;         // if menu item is blank move back
    }
    else if (encoder0Pos <= -itemTrigger) {
      noInterrupts();
        encoder0Pos += itemTrigger;
      interrupts();
      lastREActivity = millis();                            // log time last activity seen
      if (menuCount > 0) menuCount--;
    }
}   


//  ---------------------------------------


// enter a value using the rotary encoder
//   pass Value title, starting value, step size, low limit , high limit
//   returns the chosen value

int enterValue(String title, int start, int stepSize, int low, int high) {
  uint32_t tTimer = millis();                          // log time of start of function
  int storeStart = start;   
  // display title
    display.clearDisplay();  
    if (title.length() > 8) display.setTextSize(1);  // if title is longer than 8 chars make text smaller
    else display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(title);
    display.display();                                  // update display
  int tvalue = start;
  while ( (digitalRead(encoder0Press) == LOW) && (millis() - tTimer < (OLEDDisplayTimeout * 1000)) ) delay(5);    // wait for button release
  tTimer = millis();
  while ( (digitalRead(encoder0Press) == HIGH) && (millis() - tTimer < (OLEDDisplayTimeout * 1000)) ) {   // while button is not pressed and still within time limit
    if (encoder0Pos >= itemTrigger) {                    // encoder0Pos is updated via the interrupt procedure
      tvalue -= stepSize;
      noInterrupts();                                    // stop interrupt changing the value whilst it is changed here
        encoder0Pos -= itemTrigger;
      interrupts();
      tTimer = millis(); 
    } 
    else if (encoder0Pos <= -itemTrigger) {
      tvalue += stepSize;
      noInterrupts();
        encoder0Pos += itemTrigger;
      interrupts();
      tTimer = millis();
    }  
    // value limits
      if (tvalue > high) tvalue=high;              
      if (tvalue < low) tvalue=low;
    display.setTextSize(3);
    const int textPos = 27;                             // height of number on display
    display.fillRect(0, textPos, SCREEN_WIDTH, SCREEN_HEIGHT - textPos, BLACK);   // clear bottom half of display (128x64)
    display.setCursor(0, textPos);
    display.print(tvalue);
    // bar graph at bottom of display
      int tmag=map(tvalue, low, high, 0 ,SCREEN_WIDTH);
      display.fillRect(0, SCREEN_HEIGHT - 10, tmag, 10, WHITE);  
    display.display();                                  // update display
    yield();                                            // service any web page requests
  }
  
  // if it timed out set value back to what it was
    if (millis() - tTimer >= (OLEDDisplayTimeout * 1000)) tvalue=storeStart;  
   
  return tvalue;
}


//  --------------------------------------

        
// choose from list using rotary encoder
//  pass the number of items in list (max 8), list title, list of options in a string array

int chooseFromList(int noOfElements, String listTitle, String list[]) {

  const byte noList = 10;                                // max number of items to list
  uint32_t tTimer = millis();                            // log time of start of function
  int highlightedItem = 0;                               // which item in list is highlighted
  int xpos, ypos;
  
  // display title
    display.clearDisplay();  
    display.setTextSize(1);
    display.setTextColor(WHITE,BLACK);
    display.setCursor(10, 0);
    display.print(listTitle);
    display.drawLine(0, lineSpace1, display.width(), lineSpace1, WHITE);

  // scroll through list
    while ( (digitalRead(encoder0Press) == LOW) && (millis() - tTimer < (OLEDDisplayTimeout * 1000)) ) delay(5);    // wait for button release
    tTimer = millis();
    while ( (digitalRead(encoder0Press) == HIGH) && (millis() - tTimer < (OLEDDisplayTimeout * 1000)) ) {   // while button is not pressed and still within time limit
      if (encoder0Pos >= itemTrigger) {                    // encoder0Pos is updated via the interrupt procedure
        noInterrupts();
          encoder0Pos = 0;
        interrupts();
        highlightedItem++;
        tTimer = millis(); 
      }
      if (encoder0Pos <= -itemTrigger) {
        noInterrupts();
          encoder0Pos = 0;
        interrupts();
        highlightedItem--;
        tTimer = millis();
      }  
      // value limits
        if (highlightedItem > noOfElements - 1) highlightedItem = noOfElements - 1;        
        if (highlightedItem < 0) highlightedItem = 0;
      // display the list
        for (int i=0; i < noOfElements; i++) {
            if (i < (noList/2)) {
              xpos = 0; 
              ypos = (lineSpace1 * i+1) + topLine;
            } else {
              xpos = display.width() / 2;
              ypos = lineSpace1 * (i-(noList/2)) + topLine;
            }
            display.setCursor(xpos, ypos);
            if (i == highlightedItem) display.setTextColor(BLACK,WHITE);
            else display.setTextColor(WHITE,BLACK);
            display.print(list[i]);
        }
      display.display();                                    // update display
      yield();                                              // service any web page requests
    }
  
  // if it timed out set selection to cancel (i.e. item 0)
    if (millis() - tTimer >= (OLEDDisplayTimeout * 1000)) highlightedItem=0;      

//  // wait for button to be released (up to 1 second)
//    tTimer = millis();                         // log time
//    while ( (digitalRead(encoder0Press) == LOW) && (millis() - tTimer < 1000) ) {
//      yield();        // service any web page requests
//      delay(20);
//    }

    return highlightedItem;
}


//  --------------------------------------


// close the menus and return to sleep mode

void exitMenu() {    
    reButtonState = digitalRead(encoder0Press);     // reset current button status 
    lastREActivity = 0;                             // clear time of last rotary encoder usage
    noInterrupts();
      delay(50);
      encoder0Pos = 0;                              // clear rotary encoder position change counter
    interrupts();
    menuTitle = "";
    display.clearDisplay();                           // display off
    display.display();
}


//  --------------------------------------


// rotary encoder interrupt routine to update position counter when turned
//     interrupt info: https://www.gammon.com.au/forum/bbshowpost.php?id=11488

void ICACHE_RAM_ATTR doEncoder() {
     
  bool pinA = digitalRead(encoder0PinA);
  bool pinB = digitalRead(encoder0PinB);

  if ( (encoderPrevA == pinA && encoderPrevB == pinB) ) return;  // no change since last time (i.e. reject bounce)

  // same direction (alternating between 0,1 and 1,0 in one direction or 1,1 and 0,0 in the other direction)
         if (encoderPrevA == 1 && encoderPrevB == 0 && pinA == 0 && pinB == 1) encoder0Pos -= 1;
    else if (encoderPrevA == 0 && encoderPrevB == 1 && pinA == 1 && pinB == 0) encoder0Pos -= 1;
    else if (encoderPrevA == 0 && encoderPrevB == 0 && pinA == 1 && pinB == 1) encoder0Pos += 1;
    else if (encoderPrevA == 1 && encoderPrevB == 1 && pinA == 0 && pinB == 0) encoder0Pos += 1;
    
  // change of direction
    else if (encoderPrevA == 1 && encoderPrevB == 0 && pinA == 0 && pinB == 0) encoder0Pos += 1;   
    else if (encoderPrevA == 0 && encoderPrevB == 1 && pinA == 1 && pinB == 1) encoder0Pos += 1;
    else if (encoderPrevA == 0 && encoderPrevB == 0 && pinA == 1 && pinB == 0) encoder0Pos -= 1;
    else if (encoderPrevA == 1 && encoderPrevB == 1 && pinA == 0 && pinB == 1) encoder0Pos -= 1;

    else if (serialDebug) Serial.println("Error: invalid rotary encoder pin state - prev=" + String(encoderPrevA) + "," 
                                          + String(encoderPrevB) + " new=" + String(pinA) + "," + String(pinB));
    
  // update previous readings
    encoderPrevA = pinA;
    encoderPrevB = pinB;
}


//  --------------------------------------


// display time/date on oled

void displayTimeOLED() {

   // time
     time_t t=now();                                  // get current time 
     String ttime = String(hour(t)) + ":" ;           // hours
     if (minute(t) < 10) ttime += "0";                // minutes
     ttime += String(minute(t));
//     String tseconds = " ";                            // seconds
//     int tsec = second(t);
//     if (tsec < 10) tseconds += "0"; 
//     tseconds += String(tsec);

   // date
     const String tDoW[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
     String tday = tDoW[weekday(t)-1];                                                         // day of week
     String tdate = String(day(t)) + "/" + String(month(t)) + "/" + String(year(t)) + " ";    // date

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(ttime);
//    display.setTextSize(1);
//    display.print(tseconds);
//    display.setTextSize(2);
    display.setCursor(0, topLine);
    display.print(tday);
    display.setCursor(0, topLine + lineSpace2);
    display.print(tdate);
    display.setTextSize(1);
    display.setCursor(20, display.height() - lineSpace1);
    display.print(WiFi.localIP());
    display.display();
}

//  --------------------------------------


// set menu item
// pass: new menu items number, name         (blank iname clears all entries)

void setMenu(int inum, String iname) {
  if (inum >= menuMax) return;    // invalid number
  if (iname == "") {              // clear all menu items
    for (int i; i < menuMax; i++)  menuOption[i] = "";
    menuCount = 0;                // move highlight to top menu item
  } else {
    menuOption[inum] = iname;
    menuItemClicked = 100;        // set item selected flag as none
  }
}


// ---------------------------------------------- end ----------------------------------------------
