/*******************************************************************************************************************
 *
 *             Basic web server For ESP8266/ESP32 using Arduino IDE 
 *             
 *             Included files: gmail.h, standard.h, ota.h and wifi.h (gmail.h is optional)
 *             
 *      I use this sketch as the starting point for most of my ESP based projects.   It is the simplest way
 *      I have found to provide a basic web page displaying updating information, control buttons etc..
 *      It also has the ability to retrieve a web page as text (requestpage).
 *      note: the more advanced method would be using web sockets - see https://www.youtube.com/watch?v=ROeT-gyYZfw
 *                                                     
 *                                                      
 *      Note:  To add ESP8266/32 ability to the Arduino IDE enter the below line in to FILE/PREFERENCES/BOARDS MANAGER
 *             http://arduino.esp8266.com/stable/package_esp8266com_index.json, https://dl.espressif.com/dl/package_esp32_index.json
 *             You can then add them in the Boards Manager.
 *          
 *      First time the ESP starts it will create an access point "ESPConfig" which you need to connect to in order to enter your wifi details.  
 *             default password = "12345678"   (note-it may not work if anything other than 8 characters long for some reason?)
 *             see: https://randomnerdtutorials.com/wifimanager-with-esp8266-autoconnect-custom-parameter-and-manage-your-ssid-and-password
 *
 * 
 *      Note: to send/receive data via wifi see UDP: https://www.arduino.cc/en/Tutorial/UDPSendReceiveString
 * 
 *      Lots of great info. - https://randomnerdtutorials.com  or  https://techtutorialsx.com
 *                            https://www.baldengineer.com/
 *      Handy way to try out your C++ code:   https://coliru.stacked-crooked.com/
 * 
 *      
 *      Much of the sketch is the code from other people's work which I have combined together, I believe I have links 
 *      to all the sources but let me know if I have missed anyone.
 *
 *                                                                               Created by: www.alanesq.eu5.net
 *      
 ********************************************************************************************************************/



// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------


  const String stitle = "BasicWebServer";                // title of this sketch

  const String sversion = "05Sep20";                     // version of this sketch

  const char* MDNStitle = "ESP1";                        // Mdns title (use http://<MDNStitle>.local )
  
  #define ENABLE_OTA 1                                   // Enable Over The Air updates (OTA)
  const String OTAPassword = "12345678";                 // Password to enable OTA service (supplied as - http://<ip address>?pwd=xxxx )

  const String HomeLink = "/";                           // Where home button on web pages links to (usually "/")

  const uint16_t datarefresh = 3000;                     // Refresh rate of the updating data on web page (1000 = 1 second)
  String JavaRefreshTime = "500";                        // time delay when loading url in web pages (Javascript)
  
  const byte LogNumber = 40;                             // number of entries to store in the system log

  const uint16_t ServerPort = 80;                        // ip port to serve web pages on

  const byte led = 2;                                    // indicator LED pin - D0/D4 on esp8266 nodemcu, 3 on esp8266-01, 2 on ESP32

  const uint16_t ledBlinkRate = 1500;                    // Speed to blink the status LED (milliseconds)
  
  // const onboardButton = D3;                              // onboard button (nodemcu boards)

  const boolean ledON = LOW;                             // Status LED control 
  const boolean ledOFF = HIGH;
  

// ---------------------------------------------------------------
  

  // Debugging tool
  // use with:    TRACE();    or    DUMP(someValue);
  
  // #include <ArduinoTrace.h>
  

// ---------------------------------------------------------------


bool OTAEnabled = 0;                    // flag if OTA has been enabled (via supply of password)

uint32_t LEDtimer = millis();           // used for flashing the LED
  
  
#include "wifi.h"                       // Load the Wifi / NTP stuff

#include "standard.h"                   // Standard BasicWebServer procedures

#if ENABLE_OTA
  #include "ota.h"                           // Over The Air updates (OTA)
#endif


//// if email required include this section
//#if defined(ESP8266) 
//    #include gmail_esp8266.h
//#elif defined(ESP32) 
//    #include gmail_esp32.h
//#endif

  
// ---------------------------------------------------------------
//    -SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
// ---------------------------------------------------------------
//
// setup section (runs once at startup)

void setup(void) {
    
  Serial.begin(115200);
//  Serial.setTimeout(2000);
//  while(!Serial) { }        // Wait for serial to initialize.

  Serial.println(F("\n\n\n---------------------------------------"));
  Serial.println("Starting - " + stitle + " - " + sversion);
  Serial.println(F("---------------------------------------"));
  Serial.println( "ESP type: " + ESPType );
  // Serial.println(ESP.getFreeSketchSpace());
  #if defined(ESP8266)     
    Serial.println("Chip ID: " + ESP.getChipId());
    rst_info *rinfo = ESP.getResetInfoPtr();
    Serial.println(String("ResetInfo: ") + (*rinfo).reason + ": " + ESP.getResetReason());
    Serial.printf("Flash chip size: %d (bytes)\n", ESP.getFlashChipRealSize());
    Serial.printf("Flash chip frequency: %d (Hz)\n", ESP.getFlashChipSpeed());
  #endif
  
  // Serial.setDebugOutput(true);                                // enable extra diagnostic info  
   
  // configure the onboard LED
    pinMode(led, OUTPUT); 
    digitalWrite(led, ledON);                                    // led on until wifi has connected

  // configure the onboard input button (nodemcu onboard, low when pressed)
    // pinMode(onboardButton, INPUT); 

  startWifiManager();                                            // Connect to wifi (procedure is in wifi.h)

  // start MDNS - see https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
  if (MDNS.begin(MDNStitle)) {
    Serial.println("MDNS responder started");
  }
  
  WiFi.mode(WIFI_STA);     // turn off access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    //    // configure as wifi access point as well
    //    Serial.println(F("starting access point"));
    //    WiFi.softAP("ESP-AP", "password");               // access point settings (Note: password must be 8 characters for some reason)
    //    WiFi.mode(WIFI_AP_STA);                          // enable as both Station and access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    //    IPAddress myIP = WiFi.softAPIP();
    //    Serial.print("Access Point Started - IP address: ");
    //    Serial.println(myIP);
  
  // WiFi.setSleepMode(WIFI_NONE_SLEEP);     // Stops wifi turning off (if on causes wifi to drop out randomly on esp8266 boards)
    
  // set up web page request handling
    server.on(HomeLink, handleRoot);         // root page
    server.on("/data", handleData);          // This displays information which updates every few seconds (used by root web page)
    server.on("/ping", handlePing);          // ping requested
    server.on("/log", handleLogpage);        // system log
    server.on("/test", handleTest);          // testing page
    server.on("/reboot", handleReboot);      // reboot the esp
    server.onNotFound(handleNotFound);       // invalid page requested
  
  // start web server
    Serial.println(F("Starting web server"));
    server.begin();

  // Finished connecting to network
    digitalWrite(led, ledOFF);
    log_system_message(stitle + " Has Started");             

}



// ----------------------------------------------------------------
//   -LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// ----------------------------------------------------------------

void loop(void){

  #if defined(ESP8266)
    yield();                        // allow esp8266 to carry out wifi tasks (may restart randomly without this command)
  #endif
  
  server.handleClient();            // service any web page requests (may not be needed for esp32?)





           // YOUR CODE HERE !




  // every 1.5 seconds change the LED status, check Wifi is connected and update time
  //          explanation of timing here: https://www.baldengineer.com/arduino-millis-plus-addition-does-not-add-up.html
    if ((unsigned long)(millis() - LEDtimer) >= ledBlinkRate ) {   
      digitalWrite(led, !digitalRead(led));        // invert led status
      WIFIcheck();                                 // check if wifi connection is ok
      LEDtimer = millis();                         // reset timer
      // time_t t=now();                              // read current time to ensure NTP auto refresh keeps triggering (otherwise only triggers when time is required causing a delay in response)
    }

} 



// ----------------------------------------------------------------
//       -root web page requested    i.e. http://x.x.x.x/
// ----------------------------------------------------------------

void handleRoot() {  

  WiFiClient client = server.client();             // open link with client
  String tstr;                                     // temp store for building line of html
  client.write(webheader("").c_str());             // html page header  (with extra formatting)

  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    log_system_message("Root page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]));


  // action any button presses etc.

  #if ENABLE_OTA
  // enable OTA if password supplied in url parameters   (?pass=xxx)
    if (server.hasArg("pwd")) {
        String Tvalue = server.arg("pwd");   // read value
          if (Tvalue == OTAPassword) {
            otaSetup();    // Over The Air updates (OTA)
            log_system_message("OTA enabled");
            OTAEnabled = 1;
          }
    }
  #endif

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
  
    tstr = "<FORM action='" + HomeLink + "' method='post'>\n";     // used by the buttons (action = the page send it to)
    client.write(tstr.c_str());
    client.write("<P>");                                           // start of section

    tstr = "Welcome to the demo " + ESPType + " web page\n";
    client.write(tstr.c_str());

    // insert an iframe containing the changing data (updates every few seconds using javascript)
      client.write("<BR><iframe id='dataframe' height=150; width=600; frameborder='0';></iframe>\n");
      client.write("<script type='text/javascript'>\n");
      tstr = "setTimeout(function() {document.getElementById('dataframe').src='/data';}, " + JavaRefreshTime +");\n";
      client.write(tstr.c_str());
      tstr = "window.setInterval(function() {document.getElementById('dataframe').src='/data';}, " + String(datarefresh) + ");\n";
      client.write(tstr.c_str());
      client.write("</script>\n"); 

    // demo radio buttons - "RADIO1"
      client.write("<BR>Demo radio buttons\n");
      client.write("<BR>Radio1 button1\n");                                  // radio button 1
      client.write("<INPUT type='radio' name='RADIO1' value='1'>\n");
      client.write("<BR>Radio1 button2\n");                                  // radio button 2
      client.write("<INPUT type='radio' name='RADIO1' value='2'>\n");
      client.write("<BR><INPUT type='reset'>\n");                            // reset radio button 
      client.write("<INPUT type='submit' value='Action'>\n");                // action button

    // demo standard button 
    //    'name' is what is tested for above to detect when button is pressed, 'value' is the text displayed on the button
      client.write("<BR><BR><input style='"); 
      // if ( x == 1 ) client.write("background-color:red; ");      // to change button color depending on state
      client.write("height: 30px;' name='demobutton' value='Demonstration Button' type='submit'>\n");


    client.write("</span></P>\n");    // end of section    
  
    // close html page
      client.write(webfooter().c_str());                                                          // html page footer
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

  client.write("<!DOCTYPE HTML>\n");
  client.write("<html><body>\n"); 

  client.write("<BR>Auto refreshing information goes here\n");
  tstr = "<BR>" + currentTime() + "\n";   
  client.write(tstr.c_str());

  // OTA enabled status
    if (OTAEnabled) {
       tstr = red + "<BR>OTA ENABLED!" + endcolour;
       client.write(tstr.c_str());
    }



  
  
  
  // close html page
    client.write("</body></htlm>\n");
    delay(3);
    client.stop();
  
}



// ----------------------------------------------------------------
//           -testing page     i.e. http://x.x.x.x/test
// ----------------------------------------------------------------

void handleTest(){

  WiFiClient client = server.client();          // open link with client

  // log page request including clients IP address
      IPAddress cip = client.remoteIP();
      log_system_message("Test page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]));
  
  client.write(webheader().c_str());                // add the standard html header
  client.write("<BR>TEST PAGE<BR><BR>\n");

  // ---------------------------- test section here ------------------------------




       
  // -----------------------------------------------------------------------------

  // end html page
    client.write(webfooter().c_str());            // add the standard web page footer
    delay(1);
    client.stop();

  
}



// ----------------------------------------------------------------
//      -ping web page requested     i.e. http://x.x.x.x/ping
// ----------------------------------------------------------------

void handlePing(){

  log_system_message("ping web page requested");      
  String message = "ok";

  server.send(404, "text/plain", message);   // send reply as plain text
  message = "";      // clear variable
  
}



// --------------------------- E N D -----------------------------
