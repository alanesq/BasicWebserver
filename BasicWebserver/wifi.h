/**************************************************************************************************
 *
 *      Wifi / NTP Connections - 15Dec20
 *      
 *      part of the BasicWebserver sketch
 *             
 *      Set up wifi for either esp8266 or esp32 plus NTP (network time)
 *                    
 *      Libraries used: 
 *                      ESP_Wifimanager - https://github.com/khoih-prog/ESP_WiFiManager
 *                      TimeLib
 *                      ESPmDNS
 * 
 *      Notes:  Alternative NTP method: https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
 *                    
 *  
 **************************************************************************************************/



// **************************************** S e t t i n g s ****************************************

    
    // Configuration Portal (Wifimanager)
      const String portalName = "espserver";
      const String portalPassword = "12345678";
      // String portalName = "ESP_" + String(ESP_getChipId(), HEX);               // use chip id
      // String portalName = stitle;                                              // use sketch title

    // mDNS name
      const String mDNS_name = "espserver";
      // const String mDNS_name = stitle;                                         // use sketch title
      


// *************************************************************************************************


// forward declarations
  void startWifiManager();
  String currentTime();
  bool IsBST();
  void sendNTPpacket();
  time_t getNTPTime();
  void ClearWifimanagerSettings();
  String requestWebPage(String, String, int, int);
  

// ----------------------------------------------------------------
//                              -Startup
// ----------------------------------------------------------------

byte wifiok = 0;          // flag if wifi is connected ok (1 = ok)
  
// wifi for esp8266/esp32
  #if defined ESP32
    #include <esp_wifi.h>
    #include <WiFi.h>
    //#include "HTTPClient.h"             // used by requestwebpage()
    #include <WiFiClient.h>
    #include <WebServer.h>
    #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
    WebServer server(ServerPort);
    #include <ESPmDNS.h>                // see https://github.com/espressif/arduino-esp32/tree/master/libraries/ESPmDNS      
    const String ESPType = "ESP32";
  #elif defined ESP8266
    #include <ESP8266WiFi.h>            // https://github.com/esp8266/Arduino
    //#include "ESP8266HTTPClient.h"      // used by requestwebpage()
    #include <DNSServer.h>
    #include <ESP8266WebServer.h>  
    #define ESP_getChipId()   (ESP.getChipId())
    ESP8266WebServer server(ServerPort);
    #include <ESP8266mDNS.h>
    const String ESPType = "ESP8266";
  #else
      #error "This sketch only works with the ESP8266 or ESP32"
  #endif
 
  // SSID and Password for wifi 
    String Router_SSID;
    String Router_Pass;

  #define USE_AVAILABLE_PAGES false      // Use false if you don't like to display Available Pages in Information Page of Config Portal
  
  #include <ESP_WiFiManager.h>      


// Time from NTP server
//      from https://raw.githubusercontent.com/RalphBacon/No-Real-Time-Clock-RTC-required---use-an-NTP/master
  #include <TimeLib.h>
  #include <WiFiUdp.h>                          // UDP library which is how we communicate with Time Server
  const uint16_t localPort = 8888;              // Just an open port we can use for the UDP packets coming back in
  const char timeServer[] = "uk.pool.ntp.org"; 
  const uint16_t NTP_PACKET_SIZE = 48;          // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[NTP_PACKET_SIZE];           // buffer to hold incoming and outgoing packets
  WiFiUDP NTPUdp;                               // A UDP instance to let us send and receive packets over UDP
  const uint16_t timeZone = 0;                  // timezone (0=GMT)
  const String DoW[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  // How often to resync the time (under normal and error conditions)
    const uint16_t _resyncSeconds = 7200;       // 7200 = 2 hours
    const uint16_t _resyncErrorSeconds = 60;    // 60 = 1 min
  bool NTPok = 0;                               // Flag if NTP is curently connecting ok



// ----------------------------------------------------------------
//                 -wifi initialise  (called from 'setup')
// ----------------------------------------------------------------

void startWifiManager() {

  // erase stored wifi settings - Note - this may wipe the whole sketch - seems to be a bug/problem with wiping stored config?
  // ClearWifimanagerSettings();
 
  uint32_t startedAt = millis();

  ESP_WiFiManager ESP_wifiManager(stitle);     
  
  ESP_wifiManager.setMinimumSignalQuality(-1);

  ESP_wifiManager.setDebugOutput(true);

  ESP_wifiManager.setConfigPortalTimeout(600);       // Config portal timeout  

  // wifimanager config portal settings
    ESP_wifiManager.setSTAStaticIPConfig(IPAddress(192,168,2,114), IPAddress(192,168,2,1), IPAddress(255,255,255,0), 
                                         IPAddress(192,168,2,1), IPAddress(8,8,8,8));
  
  // We can't use WiFi.SSID() in ESP32as it's only valid after connected. 
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
  // Have to create a new function to store in EEPROM/SPIFFS for this purpose
    Router_SSID = ESP_wifiManager.WiFi_SSID();
    Router_Pass = ESP_wifiManager.WiFi_Pass();
  
  //  Serial.println("Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);    // show stored wifi password

  // if no stored wifi credentials found, open config portal
    if (Router_SSID == "") {  
      // open wifimanager config portal   
        Serial.println("\nOpening config portal");
        ESP_wifiManager.startConfigPortal((const char *) portalName.c_str(), portalPassword.c_str());
        delay(1000);
        ESP.restart();         // reboot 
        delay(5000);           // restart will fail without this delay
    }

    // connect to wifi
      #define WIFI_CONNECT_TIMEOUT        30000L
      #define WHILE_LOOP_DELAY            200L
      #define WHILE_LOOP_STEPS            (WIFI_CONNECT_TIMEOUT / ( 3 * WHILE_LOOP_DELAY ))
      WiFi.mode(WIFI_STA);
      WiFi.persistent (true);
      Serial.print("\nConnecting to ");
      Serial.println(Router_SSID);
      WiFi.begin(Router_SSID.c_str(), Router_Pass.c_str());
      startedAt = millis();
      int i = 0;
      while((!WiFi.status() || WiFi.status() >= WL_DISCONNECTED) && i++ < WHILE_LOOP_STEPS) {
        delay(WHILE_LOOP_DELAY);   
        Serial.print(".");
      }
      Serial.print("\nAfter waiting ");
      Serial.print((millis()- startedAt) / 1000);
      Serial.print(" secs. connection result is: ");
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("connected. Local IP: ");
        Serial.println(WiFi.localIP());
        wifiok = 1;     // flag wifi connected ok
      } else {
        Serial.println("Failed to connect to Wifi");
        Serial.println(ESP_wifiManager.getStatus(WiFi.status()));
        wifiok = 0;     // flag wifi failed
      }
  
  if (wifiok == 0) {           // if wifi is not connected 
    // open wifimanager config portal  
      // open wifimanager config portal   
        Serial.println("\nOpening config portal");
        ESP_wifiManager.startConfigPortal((const char *) portalName.c_str(), portalPassword.c_str());
        delay(1000);
        ESP.restart();         // reboot 
        delay(5000);           // restart will fail without this delay
      }
          
  // Set up mDNS responder:
    Serial.println( MDNS.begin(mDNS_name.c_str()) ? "mDNS responder started ok" : "Error setting up mDNS responder" );

  // start NTP
    NTPUdp.begin(localPort);                  // What port will the UDP/NTP packet respond on?
    setSyncProvider(getNTPTime);              // What is the function that gets the time (in ms since 01/01/1900)?
    setSyncInterval(_resyncErrorSeconds);     // How often should we synchronise the time on this machine (in seconds) 
           
}  // startwifimanager


// ----------------------------------------------------------------
//          -Return current time and date as a string
// ----------------------------------------------------------------

String currentTime(){

   time_t t=now();     // get current time 

   if (IsBST()) t+=3600;     // add one hour if it is Summer Time

   String ttime = String(hour(t)) + ":" ;                                             // hours
   if (minute(t) < 10) ttime += "0";                                                  // minutes
   ttime += String(minute(t)) + " ";
   ttime += DoW[weekday(t)-1] + " ";                                                  // day of week
   ttime += String(day(t)) + "/" + String(month(t)) + "/" + String(year(t)) + " ";    // date

   return ttime;

}



//-----------------------------------------------------------------------------
//                           -British Summer Time check
//-----------------------------------------------------------------------------
// returns true if it is British Summer time
//         code from https://my-small-projects.blogspot.com/2015/05/arduino-checking-for-british-summer-time.html

boolean IsBST()
{
    int imonth = month();
    int iday = day();
    int hr = hour();
    
    //January, february, and november are out.
    if (imonth < 3 || imonth > 10) { return false; }
    //April to September are in
    if (imonth > 3 && imonth < 10) { return true; }

    // find last sun in mar and oct - quickest way I've found to do it
    // last sunday of march
    int lastMarSunday =  (31 - (5* year() /4 + 4) % 7);
    //last sunday of october
    int lastOctSunday = (31 - (5 * year() /4 + 1) % 7);
        
    //In march, we are BST if is the last sunday in the month
    if (imonth == 3) { 
      
      if( iday > lastMarSunday)
        return true;
      if( iday < lastMarSunday)
        return false;
      
      if (hr < 1)
        return false;
              
      return true; 
  
    }
    //In October we must be before the last sunday to be bst.
    //That means the previous sunday must be before the 1st.
    if (imonth == 10) { 

      if( iday < lastOctSunday)
        return true;
      if( iday > lastOctSunday)
        return false;  
      
      if (hr >= 1)
        return false;
        
      return true;  
    }

}



//-----------------------------------------------------------------------------
//        send an NTP request to the time server at the given address
//-----------------------------------------------------------------------------

void sendNTPpacket(const char* address) {
  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now you can send a packet requesting a timestamp:
  // Note that Udp.begin will request automatic translation (via a DNS server) from a
  // name (eg pool.ntp.org) to an IP address. Never use a specific IP address yourself,
  // let the DNS give back a random server IP address
  NTPUdp.beginPacket(address, 123); //NTP requests are to port 123

  // Get the data back
  NTPUdp.write(packetBuffer, NTP_PACKET_SIZE);

  // All done, the underlying buffer is now updated
  NTPUdp.endPacket();
  
}



//-----------------------------------------------------------------------------
//                contact the NTP pool and retrieve the time
//-----------------------------------------------------------------------------
//
// code from https://github.com/RalphBacon/No-Real-Time-Clock-RTC-required---use-an-NTP

time_t getNTPTime() {

  // Send a UDP packet to the NTP pool address
  if (serialDebug) {
    Serial.print("\nSending NTP packet to ");
    Serial.println(timeServer);
  }
  sendNTPpacket(timeServer);

  // Wait to see if a reply is available - timeout after X seconds. At least
  // this way we exit the 'delay' as soon as we have a UDP packet to process
  #define UDPtimeoutSecs 3
  int timeOutCnt = 0;
  while (NTPUdp.parsePacket() == 0 && ++timeOutCnt < (UDPtimeoutSecs * 10)){
    delay(100);
    // yield();
  }

  // Is there UDP data present to be processed? Sneak a peek!
  if (NTPUdp.peek() != -1) {
    // We've received a packet, read the data from it
    NTPUdp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // The time-stamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900)
    unsigned long secsSince1900 = highWord << 16 | lowWord;     // shift highword 16 binary places to the left then combine with lowword
    if (serialDebug) {
      Serial.print("Seconds since Jan 1 1900 = ");
      Serial.println(secsSince1900);
    }

    // now convert NTP time into everyday time:
    //Serial.print("Unix time = ");

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     // UL denotes it is 'unsigned long' 

    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    // Reset the interval to get the time from NTP server in case we previously changed it
    setSyncInterval(_resyncSeconds);
    NTPok = 1;       // flag NTP is currently connecting ok

    return epoch;
  }

  // Failed to get an NTP/UDP response
    if (serialDebug) Serial.println("No NTP response");
    setSyncInterval(_resyncErrorSeconds);       // try more frequently until a response is received
    NTPok = 0;                                  // flag NTP not currently connecting

    return 0;
  
}


// ----------------------------------------------------------------
//                        request a web page
// ----------------------------------------------------------------

//     parameters = ip address, page to request, port to use (usually 80), maximum chars to receive, ignore all in reply before this text 
//          e.g. requestWebPage("192.168.1.166","/log",80,600,"");

String requestWebPage(String ip, String page, int port, int maxChars, String cuttoffText = ""){

  int maxWaitTime = 3000;                 // max time to wait for reply (ms)

  char received[maxChars + 1];            // temp store for incoming character data
  int received_counter = 0;               // number of characters which have been received

  if (!page.startsWith("/")) page = "/" + page;     // make sure page begins with "/" 

  if (serialDebug) {
    Serial.print("requesting web page: ");
    Serial.print(ip);
    Serial.println(page);
  }
     
    WiFiClient client;

    // Connect to the site 
      if (!client.connect(ip.c_str() , port)) {                                      
        if (serialDebug) Serial.println("Web client connection failed");   
        return "web client connection failed";
      } 
      if (serialDebug) Serial.println("Connected to host - sending request...");
    
    // send request - A basic request looks something like: "GET /index.html HTTP/1.1\r\nHost: 192.168.0.4:8085\r\n\r\n"
      client.print("GET " + page + " HTTP/1.1\r\n" +
                   "Host: " + ip + "\r\n" + 
                   "Connection: close\r\n\r\n");
  
      if (serialDebug) Serial.println("Request sent - waiting for reply...");
  
    // Wait for a response
      uint32_t ttimer = millis();
      while ( !client.available() && (uint32_t)(millis() - ttimer) < maxWaitTime ) {
        delay(10);
      }

    // read the response
      while ( client.available() && received_counter < maxChars ) {
        #if defined ESP8266
          delay(2);                          // it just reads 255s on esp8266 if this delay is not included
        #endif        
        received[received_counter] = char(client.read());     // read one character
        received_counter+=1;
      }
      received[received_counter] = '\0';     // end of string marker
            
    if (serialDebug) {
      Serial.println("--------received web page-----------");
      Serial.println(received);
      Serial.println("------------------------------------");
      Serial.flush();     // wait for serial data to finish sending
    }
    
    client.stop();    // close connection
    if (serialDebug) Serial.println("Connection closed");

    // if cuttoffText was supplied then only return the text following this 
      if (cuttoffText != "") {
        char* locus = strstr(received,cuttoffText.c_str());    // locus = pointer to the found text
        if (locus) {                                           // if text was found
          if (serialDebug) Serial.println("The text '" + cuttoffText + "' was found in reply");
          return locus;                                        // return the reply text following 'cuttoffText'
        } else if (serialDebug) Serial.println("The text '" + cuttoffText + "' WAS NOT found in reply");
      }
    
  return received;        // return the full reply text
}


//-----------------------------------------------------------------------------
//                     Clear stored wifi settings (Wifimanager)
//-----------------------------------------------------------------------------
// Note - this may wipe the whole sketch - seems to be a bug/problem with wiping stored config?

void ClearWifimanagerSettings() {

  #if defined ESP32
    // clear stored wifimanager settings
          Serial.println("Clearing stored wifimanager settings");
          wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //load the flash-saved configs
          esp_wifi_init(&cfg); //initiate and allocate wifi resources (does not matter if connection fails)
          delay(2000); //wait a bit
          if(esp_wifi_restore()!=ESP_OK)  Serial.println("\nWiFi is not initialized by esp_wifi_init");
  #endif

  #ifdef ESP8266  
    WiFi.disconnect(true);
  #else
    WiFi.disconnect(true, true);
  #endif

  ESP_WiFiManager ESP_wifiManager(stitle);    
  ESP_wifiManager.resetSettings();
  
  Serial.println("\nRestarting!");
  delay(1000);
  ESP.restart();         // reboot 
  delay(5000);           // restart will fail without this delay

}


// --------------------------- E N D -----------------------------
