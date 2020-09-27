/**************************************************************************************************
 *  
 *      Over The Air updates (OTA) - 05Sep20
 * 
 *      part of the Webserver library
 *                                   
 *      In Arduino IDE Select "ESP32 dev module" not ESP32-cam, PSRAM enabled
 *                     
 * 
 **************************************************************************************************

    To enable/disable OTA see setting at top of main sketch (#define ENABLE_OTA 1)

    Activate OTA with   http://<esp ip address>?pwd=12345678
    Then access with    http://<esp ip address>/ota

 
 **************************************************************************************************/


#if defined ESP32
  #include <Update.h>
#endif

// forward declarations
  void otaSetup();
  void handleOTA();


// ----------------------------------------------------------------
//                         -OTA setup section
// ----------------------------------------------------------------

// Called from 'setup'

void otaSetup() {

    server.on("/ota", handleOTA);

    // esp32 version (using webserver.h)
    #if defined ESP32
        server.on("/update", HTTP_POST, []() {
          server.sendHeader("Connection", "close");
          server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          ESP.restart();
        }, []() {
          HTTPUpload& upload = server.upload();
          if (upload.status == UPLOAD_FILE_START) {
            Serial.setDebugOutput(true);
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin()) { //start with max available size
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
              Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
              Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
          } else {
            Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
          }
        });
    #endif
    
    // esp8266 version  (using ESP8266WebServer.h)
    #if defined ESP8266
        server.on("/update", HTTP_POST, []() {
          server.sendHeader("Connection", "close");
          server.send(200, "text/plain", (Update.hasError()) ? "Update Failed!, rebooting" : "Update complete, device is rebooting...");
          delay(500);
          ESP.restart();
          delay(2000);
        }, []() {
          HTTPUpload& upload = server.upload();
          if (upload.status == UPLOAD_FILE_START) {
            Serial.setDebugOutput(true);
            WiFiUDP::stopAll();
            Serial.printf("Update: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace)) { //start with max available size
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
              Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
              Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
          }
          yield();
        });
    #endif

}

void handleOTA(){

  WiFiClient client = server.client();          // open link with client
  String tstr;                                  // temp store for building lines of html;

  client.write(webheader().c_str());            // add the standard html header

  // log page request including clients IP address
      IPAddress cip = client.remoteIP();
      log_system_message("OTA web page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]));

  client.write("<br><H1>Update firmware</H1><br>\n");
  tstr = "Current version = " + stitle + ", " + sversion + "<br><br>";
  client.write(tstr.c_str());
  
  client.write("<form method='POST' action='/update' enctype='multipart/form-data'>\n");
  client.write("<input type='file' style='width: 300px' name='update'>\n");
  client.write("<br><br><input type='submit' value='Update'></form><br>\n");

  client.write("<br><br>Device will reboot when upload complete");
  tstr = red + "<br>To disable OTA restart device<br>" + endcolour;
  client.write(tstr.c_str());
                          
  // close html page
    client.write(webfooter().c_str());                      // add the standard web page footer
    delay(3);
    client.stop();
    
}

// ---------------------------------------------- end ----------------------------------------------
