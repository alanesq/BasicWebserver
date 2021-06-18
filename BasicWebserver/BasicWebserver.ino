/*******************************************************************************************************************
 *
 *                              Basic web server For ESP8266/ESP32 using Arduino IDE 
 * 
 *                                   https://github.com/alanesq/BasicWebserver
 * 
 *             
 *                           Included files: email.h, standard.h, ota.h, oled.h, gsm.h & wifi.h 
 *             
 *             
 *      I use this sketch as the starting point for most of my ESP based projects.   It is the simplest way
 *      I have found to provide a basic web page displaying updating information, control buttons etc..
 *      It also has the ability to retrieve a web page as text (see: requestWebPage() in wifi.h).
 *      For a more advanced method of updating info on a web page see: 
 *                              https://github.com/alanesq/BasicWebserver/blob/master/misc/VeryBasicWebserver.ino
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
 *      Lots of great info. - https://randomnerdtutorials.com  or  https://techtutorialsx.com or https://www.baldengineer.com/
 *      Handy way to try out your C++ code:   https://coliru.stacked-crooked.com/
 * 
 *      Much of the sketch is the code from other people's work which I have combined together, I believe I have links 
 *      to all the sources but let me know if I have missed anyone.
 *
 *                                                                                      Created by: www.alanesq.eu5.net
 *                                                                               
 *        BasicWebserver is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the 
 *        implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *      
 ********************************************************************************************************************/

#if (!defined ESP8266 && !defined ESP32)
  #error This sketch is for an esp8266 or esp32 only
#endif


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------


  const char* stitle = "BasicWebServer";                 // title of this sketch

  const char* sversion = "18Jun21";                      // version of this sketch

  const bool serialDebug = 1;                            // provide extended debug info on serial port

  #define ENABLE_OLED 0                                  // Enable OLED display  

  #define ENABLE_GSM 0                                   // Enable GSM board support

  #define ENABLE_EMAIL 0                                 // Enable E-mail  
  
  #define ENABLE_OTA 1                                   // Enable Over The Air updates (OTA)
  const String OTAPassword = "12345678";                 // Password to enable OTA service (supplied as - http://<ip address>?pwd=xxxx )

  const char HomeLink[] = "/";                           // Where home button on web pages links to (usually "/")

  const char datarefresh[] = "3000";                     // Refresh rate of the updating data on web page (1000 = 1 second)
  const char JavaRefreshTime[] = "500";                  // time delay when loading url in web pages (Javascript)
  
  const byte LogNumber = 40;                             // number of entries to store in the system log

  const uint16_t ServerPort = 80;                        // ip port to serve web pages on

  const byte led = 2;                                    // indicator LED pin - D0/D4 on esp8266 nodemcu, 3 on esp8266-01, 2 on ESP32

  const uint16_t ledBlinkRate = 1500;                    // Speed to blink the status LED (milliseconds)

  const boolean ledON = LOW;                             // Status LED control 
  const boolean ledOFF = HIGH;

  const int serialSpeed = 115200;                        // Serial data speed to use
  

// ---------------------------------------------------------------
  

  // Debugging tool
  // use with:    TRACE();    or    DUMP(someValue);
  
  // #include <ArduinoTrace.h>
  

// ---------------------------------------------------------------


bool OTAEnabled = 0;                    // flag if OTA has been enabled (via supply of password)
bool GSMconnected = 0;                  // flag if the gsm module is connected ok
bool wifiok = 0;                        // flag if wifi connection is ok

uint32_t LEDtimer = millis();           // used for flashing the LED
  
#include "wifi.h"                       // Load the Wifi / NTP stuff

#include "standard.h"                   // Some standard procedures

#if ENABLE_OTA
  #include "ota.h"                      // Over The Air updates (OTA)
#endif

#if ENABLE_GSM
  #include "gsm.h"                      // GSM board
#endif

#if ENABLE_OLED
  #include "oled.h"                     // OLED display - i2c version SSD1306
#endif

#if ENABLE_EMAIL
    #include "email.h"
    // stores for email messages
      //const int maxMessageLength = 600;                         // maximum length of email message
      //const int maxSubjectLength = 150;                         // maximum length of email subject
      //char _message[maxMessageLength];
      //char _subject[maxSubjectLength];
    // forward declaration
      void smtpCallback(SMTP_Status status);    // the procedure called when status info is available
      bool sendEmail(char*, char* , char*);
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

  #if ENABLE_OLED
    oledSetup();         // initialise the oled display
  #endif

  #if ENABLE_GSM
    setupGSM();          // initialise GSM board
  #endif
 
  // if (serialDebug) Serial.setDebugOutput(true);                                // enable extra diagnostic info  
   
  // configure the onboard LED
    pinMode(led, OUTPUT); 
    digitalWrite(led, ledON);                                    // led on until wifi has connected

  // configure the onboard input button (nodemcu onboard, low when pressed)
    // pinMode(onboardButton, INPUT); 

  startWifiManager();                                            // Connect to wifi (procedure is in wifi.h)
  
  WiFi.mode(WIFI_STA);     // turn off access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    //    // configure as wifi access point as well
    //    Serial.println("starting access point");
    //    WiFi.softAP("ESP-AP", "password");               // access point settings (Note: password must be 8 characters for some reason - this may no longer be true?)
    //    WiFi.mode(WIFI_AP_STA);                          // enable as both Station and access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    //    IPAddress myIP = WiFi.softAPIP();
    //    if (serialDebug) Serial.print("Access Point Started - IP address: ");
    //    Serial.println(myIP);
  
  // Stop wifi going to sleep (if enabled it causes wifi to drop out randomly especially on esp8266 boards)
    #if defined ESP8266
      WiFi.setSleepMode(WIFI_NONE_SLEEP);     
    #elif defined ESP32
      WiFi.setSleep(false);   
    #endif
    
  // set up web page request handling
    server.on(HomeLink, handleRoot);         // root page
    server.on("/data", handleData);          // This displays information which updates every few seconds (used by root web page)
    server.on("/ping", handlePing);          // ping requested
    server.on("/log", handleLogpage);        // system log
    server.on("/test", handleTest);          // testing page
    server.on("/reboot", handleReboot);      // reboot the esp
    server.on("/ota", handleOTA);
    server.onNotFound(handleNotFound);       // invalid page requested
  
  // start web server
    if (serialDebug) Serial.println("Starting web server");
    server.begin();

  // Finished connecting to network
    digitalWrite(led, ledOFF);
    log_system_message("Started");

}

// ----------------------------------------------------------------
//   -LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// ----------------------------------------------------------------

void loop(void){

    #if defined(ESP8266)
        yield();                      // allow esp8266 to carry out wifi tasks (may restart randomly without this command)
    #endif
    
    server.handleClient();            // service any web page requests (may not be needed for esp32?)

    #if ENABLE_OLED
        oledLoop();                   // handle oled menu system
    #endif

    #if ENABLE_GSM
        GSMloop();                    // handle GSM board
    #endif




           // YOUR CODE HERE !




    // every 1.5 seconds change the LED status, check Wifi is connected and update time
    //          explanation of timing here: https://www.baldengineer.com/arduino-millis-plus-addition-does-not-add-up.html
    if ((unsigned long)(millis() - LEDtimer) >= ledBlinkRate ) {   
        digitalWrite(led, !digitalRead(led));        // invert led status
        WIFIcheck();                                 // check if wifi connection is ok
        LEDtimer = millis();                         // reset timer
        time_t t=now();                              // read current time to ensure NTP auto refresh keeps triggering (otherwise only triggers when time is required causing a delay in response)
    }

} 



// ----------------------------------------------------------------
//       -root web page requested    i.e. http://x.x.x.x/
// ----------------------------------------------------------------

void handleRoot() {  

  WiFiClient client = server.client();             // open link with client
  String tstr;                                     // temp store for building line of html
  webheader(client);             // html page header  (with extra formatting)

  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    log_system_message("Root page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]));


  // action any button presses etc.

    // if demo radio button "RADIO1" was selected 
      if (server.hasArg("RADIO1")) {
        String RADIOvalue = server.arg("RADIO1");   // read value of the "RADIO" argument 
        //if radio button 1 selected
        if (RADIOvalue == "1") {
          log_system_message("radio button 1 selected");       
          // <code here for radio buttons action>
        }
        //if radio button 2 selected
        if (RADIOvalue == "2") {
          log_system_message("radio button 2 selected");       
          // <code here for radio buttons action>
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
      client.write("<br><INPUT type='reset'>\n");                            // reset radio button 
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
  tstr = "<br>" + currentTime() + "\n";   
  client.print(tstr);         

  // OTA enabled status
    if (OTAEnabled) client.printf("%s <br>OTA ENABLED! %s", colRed, colEnd);
    
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
      log_system_message("Test page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]));
  
  webheader(client);                 // add the standard html header
  client.write("<br>TEST PAGE<br><br>\n");


  // ---------------------------- test section here ------------------------------



//// demo of how to send a sms message via GSM board
//  sendSMS(phoneNumber, "this is a test message from the gsm demo sketch");



//  // demo of how to send a email
//      _message[0]=0; _subject[0]=0;          // clear any existing text
//      strcat(_subject,"test message");
//      strcat(_message,"this is a test email from the esp");
//      sendEmail(_emailReceiver, _subject, _message);  
  


//  // demo of how to request a web page
//      String webpage = requestWebPage("192.168.1.166","/log",80,800,"<html>");
//      if (serialDebug) Serial.println(webpage);
    
     
  
  // -----------------------------------------------------------------------------


  // end html page
    webfooter(client);            // add the standard web page footer
    delay(1);
    client.stop();
}


// --------------------------- E N D -----------------------------
