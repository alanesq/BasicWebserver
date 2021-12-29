// ----------------------------------------------------------------
//
//
//       ESP32 / ESp8266  very basic web server demo - 29Dec21
//                for Arduino IDE or PlatformIO
//
//       shows use of AJAX to show updating info on the web page
//
//       https://github.com/alanesq/BasicWebserver/blob/master/misc/VeryBasicWebserver.ino
//
//
// ----------------------------------------------------------------


//   ---------------------------------------------------------------------------------------------------------

//         Wifi Settings - uncomment and set here if not stored in platformio.ini

//  #define SSID_NAME "Wifi SSID"
//  #define SSID_PASWORD "wifi password"


//   ---------------------------------------------------------------------------------------------------------


#include <Arduino.h>         // required by platformIO
bool serialDebug = 1;          // enable debugging info on serial port


// forward declarations (required by PlatformIO)
  //String requestWebPage(String ip, String page, int port, int maxChars, String cuttoffText = "");
  void handleRoot();
  void handleTest();
  void handleButton();
  void handleAJAX();
  void handleSendData();
  void handleLED();
  void handleNotFound();
  int requestWebPage(String*, String*, int);


// ----------------------------------------------------------------


//#include <arduino.h>         // required by platformio?

#if defined ESP32
    // esp32
        byte LEDpin = 2;
        #include <WiFi.h>
        #include <WebServer.h>
        #include <HTTPClient.h>
        WebServer server(80);
#elif defined ESP8266
    //Esp8266
        byte LEDpin = D4;
        #include <ESP8266WiFi.h>
        #include <ESP8266WebServer.h>
        #include "ESP8266HTTPClient.h"
        ESP8266WebServer server(80);
#else
      #error "This sketch only works with the ESP8266 or ESP32"
#endif


// ----------------------------------------------------------------


// this section runs once at startup


void setup() {

  Serial.begin(115200);       // start serial comms at speed 115200
  delay(200);
  Serial.println("\n\nWebserver demo sketch");
  //Serial.println("- Reset reason: " + ESP.getResetReason());

  // onboard LEDs
    pinMode(LEDpin, OUTPUT);
    digitalWrite(LEDpin, HIGH);

    // Connect to wifi
      if (serialDebug) {
        Serial.print("\nConnecting to ");
        Serial.print(SSID_NAME);
        Serial.print("\n   ");
      }
      WiFi.begin(SSID_NAME, SSID_PASWORD);         // can be set in platformio.ini
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          if (serialDebug) Serial.print(".");
      }
      if (serialDebug) {
        Serial.print("\nWiFi connected, ");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      }
      server.begin();                               // start web server
      digitalWrite(LEDpin, LOW);

  // set up web server pages to serve
    server.on("/", handleRoot);                   // root web page (i.e. when root page is requested run procedure 'handleroot')
    server.on("/test", handleTest);               // test web page
    server.on("/button", handleButton);           // demo simple use of buttons
    server.on("/ajax", handleAJAX);               // demo using AJAX to update information on the page (also javascript buttons)
      server.on("/senddata", handleSendData);     // requested by the AJAX web page to request current millis value
      server.on("/setLED", handleLED);            // action when a button is clicked on the AJAX page
    server.onNotFound(handleNotFound);            // if invalid url is requested

  // start web server
    server.begin();

    // stop the wifi being turned off if not used for a while
      #if defined ESP32
        WiFi.setSleep(false);
      #else
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
      #endif

    WiFi.mode(WIFI_STA);                          // turn off access point - options are WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF

}  // setup



// ----------------------------------------------------------------


// this section runs repeatedly in a loop


void loop() {

  server.handleClient();                         // service any web page requests

  //// flash onboard LED
  //digitalWrite(LEDpin, !digitalRead(LEDpin));    // invert onboard LED status
  //delay(200);                                    // wait 200 milliseconds

  #if defined(ESP8266)
      yield();                      // allow esp8266 to carry out wifi tasks (may restart randomly without this)
  #endif

}  // loop



// ----------------------------------------------------------------
//      -test web page requested     i.e. http://x.x.x.x/
// ----------------------------------------------------------------
// demonstrate sending a plain text reply

void handleRoot(){

  if (serialDebug) Serial.println("Root page requested");

  String message = "root web page.  Other pages are /test, /button, /ajax";

  server.send(404, "text/plain", message);   // send reply as plain text

}  // handleRoot



// ----------------------------------------------------------------
//      -test web page requested     i.e. http://x.x.x.x/test
// ----------------------------------------------------------------
// demonstrate sending html reply

void handleTest(){

  if (serialDebug) Serial.println("Test page requested");

  WiFiClient client = server.client();     // open link with client

  // html header
    client.print("<!DOCTYPE html> <html lang='en'> <head> <title>Web Demo</title> </head> <body>\n");         // basic html header

  // html body
    client.print("<h1>Test Page</h1>\n");

  // end html
    client.print("</body></html>\n");
    delay(3);
    client.stop();

}  // handleTest



// ----------------------------------------------------------------
//      -button web page requested     i.e. http://x.x.x.x/button
// ----------------------------------------------------------------
// demonstrate simple use of a html buttons without using Javascript

void handleButton(){

  if (serialDebug) Serial.println("Button page requested");

  // check if button1 has been pressed
    if (server.hasArg("button1")) {
        Serial.println("Button 1 was pressed");
    }

  // check if button2 has been pressed
    if (server.hasArg("button2")) {
        Serial.println("Button 2 was pressed");
    }


  // send reply to client

  WiFiClient client = server.client();     // open link with client

  // html header
    client.print("<!DOCTYPE html> <html lang='en'> <head> <title>Web Demo</title> </head> <body>\n");         // basic html header
    client.print("<FORM action='/button' method='post'>\n");       // used by the buttons in the html (action = the web page to send it to

  // html body
    client.print("<h1>Button demo page</h1>\n");
    if (server.hasArg("button1")) client.print("Button 1 has been pressed!");
    if (server.hasArg("button2")) client.print("Button 2 has been pressed!");
    client.print("<br><br><input style='height: 35px;' name='button1' value='Demo button 1' type='submit'> \n");
    client.print("<br><br><input style='height: 35px;' name='button2' value='Demo button 2' type='submit'> \n");

  // end html
    client.print("</body></html>\n");
    delay(3);
    client.stop();

}  // handleButton


// ----------------------------------------------------------------
//      -ajax web page requested     i.e. http://x.x.x.x/ajax
// ----------------------------------------------------------------
// demonstrate use of AJAX to refresh info. on web page
// see:  https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/
// to store html in program memory to save ram use command:     const char index_html[] PROGMEM = R"rawliteral( <your html here> )rawliteral";

void handleAJAX() {

  WiFiClient client = server.client();     // open link with client

  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    Serial.println("Ajax page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]) );


  //     ---------------------- html ----------------------

  client.print (R"=====(

    <!DOCTYPE html>
    <html lang='en'>
    <head>
        <title>AJAX Demo</title>
    </head>

    <body>
    <div id='demo'>
        <h1>Update web page using AJAX</h1>
        <button type='button' onclick='sendData(1)'>LED ON</button>
        <button type='button' onclick='sendData(0)'>LED OFF</button><BR>
    </div>
    <div>
        Current Millis : <span id='MillisValue'>0</span><br>
        Received text : <span id='ReceivedText'>NA</span><br>
        LED State is : <span id='LEDState'>NA</span><br>
    </div>

  )=====");

  //     ------------------- JavaScript -------------------

  client.print (R"=====(<script>

      function sendData(led) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            document.getElementById('LEDState').innerHTML = this.responseText;
          }
       };
       xhttp.open('GET', 'setLED?LEDstate='+led, true);
       xhttp.send();}

       function getData() {
          var xhttp = new XMLHttpRequest();
          xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            var receivedArr = this.responseText.split(',');
            document.getElementById('MillisValue').innerHTML = receivedArr[0];
            document.getElementById('ReceivedText').innerHTML = receivedArr[1];
          }
       };
       xhttp.open('GET', 'senddata', true);
       xhttp.send();}

       setInterval(function() {
          getData();
        }, 2000);

  </script>)=====");
  //     --------------------------------------------------

  // close html page
    client.print("</body></html>\n");        // close HTML
    delay(3);
    client.stop();

}   // handleAJAX


// send data to AJAX web page
//   it replies with two items comma separated: the value in millis and some text
void handleSendData() {
   String reply = String(millis());                     // item 1
   reply += ",";
   reply += "This text sent by handleSendtime()";       // item 2
   server.send(200, "text/plane", reply); //Send millis value only to client ajax request
}


// handle button clicks on AJAX web page
//    it sets the onboard LED status and replies with it's status as plain text
void handleLED() {
    String ledState = "OFF";
    String t_state = server.arg("LEDstate");     //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
    Serial.println(t_state);
    if(t_state == "1") {
      digitalWrite(LEDpin,LOW);                   //LED ON
      ledState = "ON";                            //Feedback parameter
    } else {
      digitalWrite(LEDpin,HIGH);                  //LED OFF
      ledState = "OFF";                           //Feedback parameter
    }
    server.send(200, "text/plane", ledState);     //Send web page
}


// ----------------------------------------------------------------
//                      -invalid web page requested
// ----------------------------------------------------------------
// send this reply to any invalid url requested

void handleNotFound() {

  if (serialDebug) Serial.println("Invalid page requested");

  String tReply;

  tReply = "File Not Found\n\n";
  tReply += "URI: ";
  tReply += server.uri();
  tReply += "\nMethod: ";
  tReply += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  tReply += "\nArguments: ";
  tReply += server.args();
  tReply += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    tReply += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", tReply );
  tReply = "";      // clear variable

}  // handleNotFound



// ----------------------------------------------------------------------------------------------------------------


// Not used in this sketch but provided as an example of how to request an external web page and receive the reply as a string


// ----------------------------------------------------------------
//                        request a web page
// ----------------------------------------------------------------
//   @param    page         web page to request
//   @param    received     String to store response in
//   @param    maxWaitTime  maximum time to wait for reply (ms)
//   @returns  http code
// see:  https://randomnerdtutorials.com/esp32-http-get-post-arduino/#http-get-1
// to do:  limit size of reply

int requestWebPage(String* page, String* received, int maxWaitTime=5000){

  if (serialDebug) Serial.println("requesting web page: " + *page);

  WiFiClient client;
  HTTPClient http;     // see:  https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient
  http.setTimeout(maxWaitTime);
  http.begin(client, *page);      // for https requires (client, *page, thumbprint)  e.g.String thumbprint="08:3B:71:72:02:43:6E:CA:ED:42:86:93:BA:7E:DF:81:C4:BC:62:30";
  int httpCode = http.GET();      // http codes: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
  if (serialDebug) Serial.println("http code: " + String(httpCode));

  if (httpCode > 0) {
    *received = http.getString();
  } else {
    *received = "error:" + String(httpCode);
  }
  if (serialDebug) Serial.println(*received);

  http.end();   //Close connection
  if (serialDebug) Serial.println("Web connection closed");

  return httpCode;

}  // requestWebPage


// ----------------------------------------------------------------
// end
