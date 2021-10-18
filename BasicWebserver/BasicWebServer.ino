/*******************************************************************************************************************
 *
 *                              Basic web server For ESP8266/ESP32 using Arduino IDE 
 * 
 *                                   https://github.com/alanesq/BasicWebserver
 * 
 *               Tested with ESP32 board manager version 1.0.6, ESP8266 board manager version 2.7.4, 
 *                           WifiManager v1.7.4
 * 
 *                           Included files: email.h, standard.h, ota.h, oled.h, gsm.h & wifi.h 
 *             
 *             
 *      I use this sketch as the starting point for most of my ESP based projects.   It is the simplest way
 *      I have found to provide a basic web page displaying updating information, control buttons etc..
 *      It also has the ability to retrieve a web page as text (see: requestWebPage in wifi.h).
 *      For a more advanced method examples see: 
 *                 https://github.com/alanesq/BasicWebserver/blob/master/misc/VeryBasicWebserver.ino
 *                                                     
 *                                                      
 *      Note:  To add ESP8266/32 ability to the Arduino IDE enter the below two lines in to FILE/PREFERENCES/BOARDS MANAGER
 *                 http://arduino.esp8266.com/stable/package_esp8266com_index.json
 *                 https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *             You can then add them in TOOLS/BOARD/BOARDS MANAGER (search for esp8266 or ESP32)
 *          
 *      First time the ESP starts it will create an access point "ESPPortal" which you need to connect to in order to enter your wifi details.  
 *             default password = "12345678"   (change this in wifi.h)
 *             see: https://randomnerdtutorials.com/wifimanager-with-esp8266-autoconnect-custom-parameter-and-manage-your-ssid-and-password
 * 
 *      Much of the sketch is the code from other people's work which I have combined together, I believe I have links 
 *      to all the sources but let me know if I have missed anyone.
 *
 *                                                                                      Created by: www.alanesq.eu5.net
 *                                                                               
 *        BasicWebserver is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the 
 *        implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *      
 ********************************************************************************************************************

 Lots of great info: https://randomnerdtutorials.com  or  https://techtutorialsx.com or https://www.baldengineer.com/
 
 Handy way to try out your C++ code: https://coliru.stacked-crooked.com/
 
 GPIO pin info:  esp32: https://electrorules.com/esp32-pinout-reference/
               esp8266: https://electrorules.com/esp8266-nodemcu-pinout-reference/

*/                  

#if (!defined ESP8266 && !defined ESP32)
  #error This sketch is for an esp8266 or esp32 only
#endif


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------


  const char* stitle = "BasicWebServer";                 // title of this sketch

  const char* sversion = "18Oct21";                      // version of this sketch

  const bool serialDebug = 1;                            // provide debug info on serial port

  #define ENABLE_OLED 0                                  // Enable OLED display support

  #define ENABLE_GSM 0                                   // Enable GSM board support (not yet fully working - oct21)

  #define ENABLE_EMAIL 1                                 // Enable E-mail support

  #define ENABLE_NEOPIXEL 0                              // Enable Neopixel support
  
  #define ENABLE_OTA 1                                   // Enable Over The Air updates (OTA)
  const String OTAPassword = "12345678";                 // Password to enable OTA service (supplied as - http://<ip address>?pwd=xxxx )

  const char HomeLink[] = "/";                           // Where home button on web pages links to (usually "/")

  const char datarefresh[] = "2000";                     // Refresh rate of the updating data on web page (ms)
  const char JavaRefreshTime[] = "400";                  // time delay when loading url in web pages via Javascript (ms)
  
  const byte LogNumber = 30;                             // number of entries in the system log

  const uint16_t ServerPort = 80;                        // ip port to serve web pages on

  const byte onboardLED = 2;                             // indicator LED pin - 2 or 16 on esp8266 nodemcu, 3 on esp8266-01, 2 on ESP32

  const byte onboardButton = 0;                          // onboard button (FLASH)

  const bool ledBlinkEnabled = 1;                        // enable blinking status LED
  const uint16_t ledBlinkRate = 1500;                    // Speed to blink the status LED (milliseconds) - also perform some system tasks

  const int serialSpeed = 115200;                        // Serial data speed to use
    

// ---------------------------------------------------------------


// forward declarations
  void log_system_message(String);  
     

// ---------------------------------------------------------------

int radioButton = 0;                    // Temporary variable used in demo radio buttons

//#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

bool OTAEnabled = 0;                    // flag if OTA has been enabled (via supply of password)
bool GSMconnected = 0;                  // flag if the gsm module is connected ok
bool wifiok = 0;                        // flag if wifi connection is ok
 
#include "wifi.h"                       // Load the Wifi / NTP stuff

#include "standard.h"                   // Some standard procedures

Led statusLed1(onboardLED, LOW);        // set up onboard LED as led1 (LOW = on) - standard.h

Button button1(onboardButton, HIGH);    // set up the onboard button (Flash button) - standard.h

#if ENABLE_OTA
  #include "ota.h"                      // Over The Air updates (OTA)
#endif

#if ENABLE_GSM
  #include "gsm.h"                      // GSM board
#endif

#if ENABLE_NEOPIXEL
  #include "neopixel.h"                 // Neopixels used
#endif

#if ENABLE_OLED
  #include "oled.h"                     // OLED display - i2c version SSD1306
#endif

#if ENABLE_EMAIL
    #define _SenderName "ESP"           // name of email sender (no spaces)
    #include "email.h"
#endif

  
// ---------------------------------------------------------------
//    -SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
// ---------------------------------------------------------------
//
// setup section (runs once at startup)

void setup() {
    
  if (serialDebug) {
    Serial.begin(serialSpeed); while (!Serial); delay(200);       // start serial comms                                   // serial port
    delay(200);

    Serial.println("\n\n\n");                                    // line feeds
    Serial.println("-----------------------------------");
    Serial.printf("Starting - %s - %s \n", stitle, sversion);
    Serial.println("-----------------------------------");
    Serial.println( "Device type: " + String(ARDUINO_BOARD) + ", chip id: " + String(ESP_getChipId(), HEX));
    // Serial.println(ESP.getFreeSketchSpace());
    #if defined(ESP8266)     
        Serial.println("Chip ID: " + ESP.getChipId());
        rst_info *rinfo = ESP.getResetInfoPtr();
        Serial.println("ResetInfo: " + String((*rinfo).reason) + ": " + ESP.getResetReason());
        Serial.printf("Flash chip size: %d (bytes)\n", ESP.getFlashChipRealSize());
        Serial.printf("Flash chip frequency: %d (Hz)\n", ESP.getFlashChipSpeed());
        Serial.printf("\n");
    #endif
  }

  #if ENABLE_NEOPIXEL    // initialise Neopixels
    neopixelSetup(); 
  #endif  

  #if ENABLE_OLED
    oledSetup();         // initialise the oled display
  #endif

  #if ENABLE_GSM
    setupGSM();          // initialise GSM board
  #endif
 
  // if (serialDebug) Serial.setDebugOutput(true);         // enable extra diagnostic info  
   
  statusLed1.on();                                         // turn status led on until wifi has connected

  // configure the onboard input button (nodemcu onboard, low when pressed)
    // pinMode(onboardButton, INPUT); 

  startWifiManager();                                      // Connect to wifi (procedure is in wifi.h)
  
  WiFi.mode(WIFI_STA);     // turn off access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    //    // configure as wifi access point as well
    //    Serial.println("starting access point");
    //    WiFi.softAP("ESP-AP", "password");               // access point settings (Note: password must be 8 characters for some reason - this may no longer be true?)
    //    WiFi.mode(WIFI_AP_STA);                          // enable as both Station and access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    //    IPAddress myIP = WiFi.softAPIP();
    //    if (serialDebug) Serial.print("Access Point Started - IP address: ");
    //    Serial.println(myIP);
      
  // set up web page request handling
    server.on(HomeLink, handleRoot);         // root page
    server.on("/data", handleData);          // This displays information which updates every few seconds (used by root web page)
    server.on("/ping", handlePing);          // ping requested
    server.on("/log", handleLogpage);        // system log
    server.on("/test", handleTest);          // testing page
    server.on("/reboot", handleReboot);      // reboot the esp
    server.onNotFound(handleNotFound);       // invalid page requested
    #if ENABLE_OTA
      server.on("/ota", handleOTA);          // ota updates web page
    #endif    
  
  // start web server
    if (serialDebug) Serial.println("Starting web server");
    server.begin();
    
  // Stop wifi going to sleep (if enabled it causes wifi to drop out randomly especially on esp8266 boards)
    #if defined ESP8266
      WiFi.setSleepMode(WIFI_NONE_SLEEP);     
    #elif defined ESP32
      WiFi.setSleep(false);   
    #endif    

  // Finished connecting to network
    statusLed1.off();                        // turn status led off
    log_system_message("Started");

}

// ----------------------------------------------------------------
//   -LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// ----------------------------------------------------------------

void loop(void){

    #if defined(ESP8266)
        yield();                      // allow esp8266 to carry out wifi tasks (may restart randomly without this)
    #endif
    
    server.handleClient();            // service any web page requests 

    #if ENABLE_NEOPIXEL 
      neoTest();                      // run the neopixel testing code (in neopixel.h)
    #endif  
  
    #if ENABLE_OLED
        oledLoop();                   // handle oled menu system
    #endif

    #if ENABLE_GSM
        GSMloop();                    // handle the GSM board
    #endif

    #if ENABLE_EMAIL
        EMAILloop();                  // handle emails
    #endif




           // YOUR CODE HERE !




    // Periodically change the LED status to indicate all well
    //    explanation of timing here: https://www.baldengineer.com/arduino-millis-plus-addition-does-not-add-up.html
      static repeatTimer ledTimer;                          // set up a repeat timer
      if (ledTimer.check(ledBlinkRate)) {                   //  repeat at set interval (ms)
          bool allOK = 1;                                   // if all checks leave this as 1 then the all ok LED is flashed
          if (!WIFIcheck()) allOK = 0;                      // if wifi connection is ok
          if (timeStatus() != timeSet) allOK = 0;           // if NTP time is updating ok
          #if ENABLE_GSM
            if (!GSMconnected) allOK = 0;                   // if GSM board is responding ok
          #endif
          time_t t=now();                                   // read current time to ensure NTP auto refresh keeps triggering (otherwise only triggers when time is required causing a delay in response)
          // blink status led
            if (ledBlinkEnabled && allOK) statusLed1.flip(); // invert the LED status if all OK
            else statusLed1.off();      
      }

} 



// ----------------------------------------------------------------
//       -root web page requested    i.e. http://x.x.x.x/
// ----------------------------------------------------------------

void handleRoot() {  

  WiFiClient client = server.client();             // open link with client
  String tstr;                                     // temp store for building line of html
  webheader(client);             // html page header  (with extra formatting)

//  // set document title
//      client.print("\n<script>\n");
//      if (enableEmails) client.printf("  document.title = \"%s\";\n", stitle); 
//      else client.printf("  document.title = \"%s\";\n", "Warning!"); 
//      client.print("</script>\n");

  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    String clientIP = String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]);
    clientIP = decodeIP(clientIP);               // check for known IP addresses
    //log_system_message("Root page requested from: " + clientIP);  

  // action any button presses etc.

//  #if ENABLE_OTA
//  // enable OTA if password supplied in url parameters   (?pass=xxx)
//    if (server.hasArg("pwd")) {
//        String Tvalue = server.arg("pwd");   // read value
//          if (Tvalue == OTAPassword) {
//            otaSetup();    // Over The Air updates (OTA)
//            log_system_message("OTA enabled");
//            OTAEnabled = 1;
//          }
//    }
//  #endif

    // if demo radio button "RADIO1" was selected 
      if (server.hasArg("RADIO1")) {
        String RADIOvalue = server.arg("RADIO1");   // read value of the "RADIO" argument 
        //if radio button 1 selected
        if (RADIOvalue == "1") {
          log_system_message("radio button 1 selected");       
          // <code here for radio buttons action>
          radioButton = 1;
        }
        //if radio button 2 selected
        if (RADIOvalue == "2") {
          log_system_message("radio button 2 selected");       
          // <code here for radio buttons action>
          radioButton = 2;
        }  
        //if radio button 3 selected
        if (RADIOvalue == "3") {
          log_system_message("radio button 2 selected");       
          // <code here for radio buttons action>
          radioButton = 3;
        }              
      }
  
    // if button "demobutton" was pressed  
      if (server.hasArg("demobutton")) {
        // demo button was pressed 
          log_system_message("demo button was pressed");     
      }


  // build the HTML code 

    client.printf("<FORM action='%s' method='post'>\n", HomeLink);     // used by the buttons (action = the page send it to)
    client.write("<P>");                                               // start of section

    client.print("Welcome to the BasicWebServer, running on a " + String(ARDUINO_BOARD) + "\n");

    // insert an iframe containing the changing data (updates every few seconds using javascript)
      client.write("<br><iframe id='dataframe' height=150 width=600 frameborder='0'></iframe>\n");
      // javascript to refresh data display
        client.write("<script>\n");
        client.printf("setTimeout(function() {document.getElementById('dataframe').src='/data';}, %s );\n", JavaRefreshTime);
        client.printf("window.setInterval(function() {document.getElementById('dataframe').src='/data';}, %s );\n", datarefresh);
        client.write("</script>\n"); 

    // demo radio buttons - "RADIO1"
      client.write("<br>Demo radio buttons\n");
      client.write("<br>Radio1 button1\n");                                  // radio button 1
      client.write("<INPUT type='radio' name='RADIO1' value='1'>\n");
      client.write("<br>Radio1 button2\n");                                  // radio button 2
      client.write("<INPUT type='radio' name='RADIO1' value='2'>\n");
      client.write("<br>Radio1 button3\n");                                  // radio button 2
      client.write("<INPUT type='radio' name='RADIO1' value='3'>\n");      
      client.write("<br><INPUT type='reset'>\n");                            // reset selection
      client.write("<INPUT type='submit' value='Action'>\n");                // action button

    // demo standard button 
    //    'name' is what is tested for above to detect when button is pressed, 'value' is the text displayed on the button
      client.write("<br><br><input style='"); 
      // if ( x == 1 ) client.write("background-color:red; ");      // to change button color depending on state
      client.write("height: 30px;' name='demobutton' value='Demonstration Button' type='submit'>\n");

  
    // close page
      client.write("</P>");    // end of section    
      client.write("</form>\n");                                             // end form section (used by buttons etc.)
      webfooter(client);                                                     // html page footer
      delay(3);        
      client.stop();

}

  
// ----------------------------------------------------------------
//     -data web page requested     i.e. http://x.x.x.x/data
// ----------------------------------------------------------------
//
//   This shows information on the root web page which refreshes every few seconds

void handleData(){

  WiFiClient client = server.client();          // open link with client
  String tstr;                                  // temp store for building lines of html;

  // send standard html header
    client.write("HTTP/1.1 200 OK\r\n");
    client.write("Content-Type: text/html\r\n");
    client.write("Connection: close\r\n");
    client.write("\r\n");
    client.write("<!DOCTYPE HTML>\n");

  client.write("<html lang='en'><head><title>data</title></head><body>\n"); 

  client.write("<br>Auto refreshing information goes here\n");
  
  // current time
    tstr = "<br>" + currentTime() + "\n";   
    client.print(tstr);     

  // last ip client connection
    client.print("<br>Last IP client connected: ");
    client.print(lastClient);   

  // onboard button status
    client.print("<br>The onboard FLASH button is ");
    if (button1.isPressed()) client.print("PRESSED");
    else client.print("not pressed");

  // OTA enabled status
    if (OTAEnabled) client.printf("%s <br>OTA ENABLED! %s", colRed, colEnd);

  #if ENABLE_GSM
    if (GSMconnected) client.print("<br>GSM board connected ok");
    else client.print("<br>GSM board is not responding");
  #endif
    
  // close html page
    client.write("</body></html>\n");
    delay(3);
    client.stop();
}


// ----------------------------------------------------------------
//      -ping web page requested     i.e. http://x.x.x.x/ping
// ----------------------------------------------------------------

void handlePing(){

  log_system_message("ping web page requested");      
  String message = "ok";
  server.send(404, "text/plain", message);   // send reply as plain text
  
}


// ----------------------------------------------------------------
//           -testing page     i.e. http://x.x.x.x/test
// ----------------------------------------------------------------

void handleTest(){

  WiFiClient client = server.client();          // open link with client

  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    String clientIP = String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]);
    clientIP = decodeIP(clientIP);               // check for known IP addresses
    log_system_message("Test page requested from: " + clientIP);   
  
  webheader(client);                 // add the standard html header
  client.write("<br>TEST PAGE<br><br>\n");


  // ---------------------------- test section here ------------------------------



//// demo of how to send a sms message via GSM board (phone number is a String in the format "+447871862749")
//  sendSMS(phoneNumber, "this is a test message from the gsm demo sketch");



//  // demo of how to send an email over Wifi
//  #if ENABLE_EMAIL
//    _recepient[0]=0; _message[0]=0; _subject[0]=0;                  // clear any existing text
//    strcat(_recepient, _emailReceiver);                             // email address to send it to
//    strcat(_subject,stitle);
//    strcat(_subject,": test");
//    strcat(_message,"test email from esp project");
//    emailToSend=1; lastEmailAttempt=0; emailAttemptCounter=0; sendSMSflag=0;   // set flags that there is an email to be sent
//  #endif



//// demo of how to request a web page over Wifi
//  String webpage = requestWebPage("86.3.1.8","/ping",80,800,"");



//  // demo of how to request a web page over GSM
//    requestWebPageGSM("alanesq.eu5.net", "/temp/q.txt", 80);
    



  
  // -----------------------------------------------------------------------------


  // end html page
    webfooter(client);            // add the standard web page footer
    delay(1);
    client.stop();
}


// --------------------------- E N D -----------------------------
