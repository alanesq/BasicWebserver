/*
 * esp32/esp8266 -  ntp demo - 11jan21
 * 
 */


//   ---------------------------------------------------------------------------------------------------------

//                                      Wifi Settings

#include <wifiSettings.h>       // delete this line, un-comment the below two lines and enter your wifi details

//const char *SSID = "your_wifi_ssid";

//const char *PWD = "your_wifi_pwd";


//   ---------------------------------------------------------------------------------------------------------


// NTP settings

  #define NTP_SRV "uk.pool.ntp.org"
  #define NTP_TIMEZONE "GMT+0BST-1,M3.5.0/01:00:00,M10.5.0/02:00:00"  // https://remotemonitoringsystems.ca/time-zone-abbreviations.php
  #define NTP_TIMEOUT 10  // seconds

  
//   ---------------------------------------------------------------------------------------------------------


// standard wifi includes (not all required for this code)
    #if defined ESP32
        // esp32
            #include <WiFi.h>
            #include <WebServer.h>
            #include <HTTPClient.h>     
            WebServer server(80);
    #elif defined ESP8266
        //Esp8266
            #include <ESP8266WiFi.h>  
            #include <ESP8266WebServer.h>  
            #include "ESP8266HTTPClient.h"    
            ESP8266WebServer server(80);  
    #else
        #error "This sketch only works with the ESP8266 or ESP32"
    #endif

        
 
// refresh time from NTP server and display it
void printLocalTime()
{
  char ttime[40];
  
  #if defined ESP32
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){         // refresh time from NTP server?
      Serial.println("Failed to obtain time");
      return;
    }
    strftime(ttime, 40, "%a_%b_%d_%Y-%H:%M:%S", &timeinfo);
  #else    // ESP8266
    time_t now;
    time(&now);                           // refresh time from NTP server?
    strftime(ttime, 40, "%a_%b_%d_%Y-%H:%M:%S", localtime(&now));
  #endif

  Serial.println(ttime);
}  // printLocalTime

 
 
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n\nStarting");

  // start wifi
    WiFi.begin(SSID, PWD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Unable to connect to WiFi");
      return;
    }

  // NTP
    configTime(0, 0, NTP_SRV); // configTime(10 * 3600, 3600, NTP_SRV); //UTC time  //dw AEST
    setenv("TZ", NTP_TIMEZONE, 1);
    // request time from NTP server?
      #if defined ESP32
        struct tm now;
        if (!getLocalTime(&now, NTP_TIMEOUT * 1000ULL)) {
          Serial.println("Unable to sync with NTP server");
          return;
        }
      #else    // ESP8266
        uint32_t start = millis();
        time_t now;
        tm timeinfo;
        do {
          time(&now);
          localtime_r(&now, &timeinfo);
          Serial.print(".");
          delay(300);
        } while (((millis() - start) <= (1000 * NTP_TIMEOUT)) && (timeinfo.tm_year < (2016 - 1900)));
        if (timeinfo.tm_year <= (2016 - 1900)) Serial.println("Unable to sync with NTP server");  // the NTP call was not successful
      #endif

    Serial.println("NTP time updated");
    // printLocalTime();
  
}    // setup



void loop() {  
  printLocalTime();        // update NTP time from server and display
  delay(5000);  
}   // loop


// end

