/**************************************************************************************************
 *
 *      Standard procedures - 18Oct21
 *      
 *      part of the BasicWebserver sketch - https://github.com/alanesq/BasicWebserver
 *      
 *             
 **************************************************************************************************/


// forward declarations
  void log_system_message(String);
  void webheader();
  void webfooter();
  void handleLogpage();
  void handleNotFound();
  void handleReboot();
  bool WIFIcheck();
  String decodeIP(String);


// ----------------------------------------------------------------
//                              -Startup
// ----------------------------------------------------------------  

// html text colour codes (obsolete html now and should use CSS)
  const char colRed[] = "<font color='#FF0000'>";           // red text
  const char colGreen[] = "<font color='#006F00'>";         // green text
  const char colBlue[] = "<font color='#0000FF'>";          // blue text
  const char colEnd[] = "</font>";                          // end coloured text

// misc variables
  String lastClient = "n/a";                  // IP address of most recent client connected
  int system_message_pointer = 0;             // pointer for current system message position
  String system_message[LogNumber + 1];       // system log message store  (suspect serial port issues caused if not +1 ???)
  

// ----------------------------------------------------------------
//                            -repeatTimer
// ---------------------------------------------------------------- 
// repeat an operation periodically 
// example to repeat every 2 seconds:   static repeatTimer timer1;             // set up a timer     
//                                      if (timer1.check(2000)) {do stuff};    // repeat every 2 seconds

class repeatTimer {

  private:
    uint32_t  rLastTime;                                              // store last time event triggered

  public:
    repeatTimer() {
      reset();
    }

    bool check(uint32_t r_interval, bool r_reset=1) {                 // check if provided time has passed 
      if ((unsigned long)(millis() - rLastTime) >= r_interval ) {
        if (r_reset) reset();
        return 1;
      } else {
        return 0;
      }
    }

    void reset() {                                                    // reset the timer
      rLastTime = millis();
    }

};


// ----------------------------------------------------------------
//                    -decode IP addresses
// ----------------------------------------------------------------
// Check for known IP addresses 

String decodeIP(String IPadrs) {
  
    if (IPadrs == "192.168.1.176") IPadrs = "HA server";
    else if (IPadrs == "192.168.1.103") IPadrs = "Parlour laptop";
    else if (IPadrs == "192.168.1.101") IPadrs = "Bedroom laptop";
    else if (IPadrs == "192.168.1.169") IPadrs = "Linda's laptop";
    else if (IPadrs == "192.168.1.170") IPadrs = "Shed 1 laptop";
    else if (IPadrs == "192.168.1.143") IPadrs = "Shed 2 laptop";

    // log last IP client connected
      if (IPadrs != lastClient) {
        lastClient = IPadrs;
        log_system_message("New IP client connected: " + IPadrs);
      }
    
    return IPadrs;
}


// ----------------------------------------------------------------
//                      -log a system message  
// ----------------------------------------------------------------

void log_system_message(String smes) {

  // increment position pointer
    system_message_pointer++;
    if (system_message_pointer >= LogNumber) system_message_pointer = 0;

  // add the new message to log
    system_message[system_message_pointer] = currentTime() + " - " + smes;
  
  // also send message to serial port
    if (serialDebug) Serial.println("Log:" + system_message[system_message_pointer]);
}


// ----------------------------------------------------------------
//                         -header (html) 
// ----------------------------------------------------------------
// HTML at the top of each web page
//    additional style settings can be included and auto page refresh rate


void webheader(WiFiClient &client, char style[] = " ", int refresh = 0) {

  // html header
    client.write("HTTP/1.1 200 OK\r\n");
    client.write("Content-Type: text/html\r\n");
    client.write("Connection: close\r\n");
    client.write("\r\n");
    client.write("<!DOCTYPE HTML>\n");  
    client.write("<html lang='en'>\n");
    client.write("<head>\n");
    client.write("<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");

  // page refresh
    if (refresh > 0) client.print( "<meta http-equiv='refresh' content='" + String(refresh) + "'>\n");   

  // page title
    client.printf("<title> %s </title>\n", stitle); 
    
  // CSS 
    client.write("<style>\n"); 
    client.write("ul {list-style-type: none; margin: 0; padding: 0; overflow: hidden; background-color: rgb(128, 64, 0);}\n");
    client.write("li {float: left;}\n");
    client.write("li a {display: inline-block; color: white; text-align: center; padding: 30px 20px; text-decoration: none;}\n");
    client.write("li a:hover { background-color: rgb(100, 0, 0);}\n");
    client.print(style);                                // insert aditional style if supplied 
    client.write("</style>\n");
    
  client.write("</head>\n");
  client.write("<body style='color: rgb(0, 0, 0); background-color: yellow; text-align: center;'>\n");

  // menu
    client.write("<ul>\n");
    client.printf("<li><a href='%s'>Home</a></li>\n", HomeLink);                                          // home page url
    client.print("<li><a href='/log'>Log</a></li>\n");                                                    // log menu button 
    client.printf("<h1> <font color='#FF0000'>%s</h1></font>\n", stitle);                                 // sketch title
    client.print("</ul>\n");
}


// ----------------------------------------------------------------
//                             -footer (html)
// ----------------------------------------------------------------
// HTML at the end of each web page

void webfooter(WiFiClient &client) {

   // get mac address
     byte mac[6];
     WiFi.macAddress(mac);
   
   client.print("<br>\n"); 
   
   /* Status display at bottom of screen */
     client.write("<div style='text-align: center;background-color:rgb(128, 64, 0)'>\n");
     client.printf("<small> %s", colRed); 
     client.printf("%s %s", stitle, sversion); 
     
     client.printf(" | Memory: %dK", ESP.getFreeHeap() /1000); 
     
     client.printf(" | Wifi: %ddBm", WiFi.RSSI()); 
     
     // NTP server link status
      int tstat = timeStatus();   // ntp status
      if (tstat == timeSet) client.print(" | NTP OK");
      else if (tstat == timeNeedsSync) client.print(" | NTP Sync failed");
      else if (tstat == timeNotSet) client.print(" | NTP Failed");
      
     // client.printf(" | Spiffs: %dK", ( SPIFFS.totalBytes() - SPIFFS.usedBytes() / 1000 ) );             // if using spiffs 
     
     // client.printf(" | MAC: %2x%2x%2x%2x%2x%2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);       // mac address
     
     client.printf("%s </small>\n", colEnd);
     client.write("</div>\n"); 
     
  /* end of HTML */  
   client.write("</body>\n");
   client.write("</html>\n");

}


// ----------------------------------------------------------------
//   -log web page requested    i.e. http://x.x.x.x/log
// ----------------------------------------------------------------

void handleLogpage() {

  WiFiClient client = server.client();                     // open link with client

  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    String clientIP = String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]);
    clientIP = decodeIP(clientIP);               // check for known IP addresses
    //log_system_message("Log page requested from: " + clientIP);  

      
  // build the html for /log page

    webheader(client);                         // send html page header  

    client.println("<P>");                     // start of section

    client.println("<br>SYSTEM LOG<br><br>");

    // list all system messages
    int lpos = system_message_pointer;         // most recent entry
    for (int i=0; i < LogNumber; i++){         // count through number of entries
      client.print(system_message[lpos]);
      client.println("<br>");
      if (lpos == 0) lpos = LogNumber;
      lpos--;
    }
    
    // close html page
      webfooter(client);                       // send html page footer
      delay(3);
      client.stop();

}


// ----------------------------------------------------------------
//                      -invalid web page requested
// ----------------------------------------------------------------

void handleNotFound() {

  WiFiClient client = server.client();          // open link with client
  
  // log page request including clients IP address
    IPAddress cip = client.remoteIP();
    String clientIP = String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]);
    clientIP = decodeIP(clientIP);               // check for known IP addresses
    log_system_message("Invalid URL requested from " + clientIP);  
       
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  message = "";      // clear variable
  
}


// ----------------------------------------------------------------
//   -reboot web page requested        i.e. http://x.x.x.x/reboot  
// ----------------------------------------------------------------
//
//     note: this fails if the esp has just been reflashed and not restarted


void handleReboot(){

      String message = "Rebooting....";
      server.send(404, "text/plain", message);   // send reply as plain text

      // rebooting
        delay(500);          // give time to send the above html
        ESP.restart();   
        delay(5000);         // restart fails without this line

}


// --------------------------------------------------------------------------------------
//                                -wifi connection check
// --------------------------------------------------------------------------------------

bool WIFIcheck() {
  
    if (WiFi.status() != WL_CONNECTED) {
      if ( wifiok == 1) {
        log_system_message("Wifi connection lost");          // log system message if wifi was ok but now down
        wifiok = 0;                                          // flag problem with wifi
      }
    } else { 
      // wifi is ok
      if ( wifiok == 0) {
        log_system_message("Wifi connection is back");       // log system message if wifi was down but now back
        wifiok = 1;                                          // flag wifi is now ok
      }
    }

    return wifiok;

}


// --------------------------------------------------------------------------------------
//                                    -LED control code
// --------------------------------------------------------------------------------------
// Supply gpio pin, if LED ON when pin is high or low
// useage:      Led led1(LED_1_GPIO, HIGH);       led1.on();

class Led {
  
  private:
    byte pin;           // gpio pin
    bool onState;       // if on when pin is high or low
    
  public:
    Led(byte l_pin, bool l_onState = HIGH) {
      this->pin = l_pin;
      this->onState = l_onState;
      init();
    }

    void init() {
      pinMode(pin, OUTPUT);
      off();
    }

    void on() {
      digitalWrite(pin, onState);
    }

    void off() {
      digitalWrite(pin, !onState);
    }

    void flip() {
      digitalWrite(pin, !digitalRead(pin));   // flip the leds status
    }

    bool status() {                           // returns HIGH if LED is on
      bool s_state = digitalRead(pin);
      if (onState == HIGH) return s_state;
      else return !s_state;
    }

    void flash(int f_reps=1, int f_ledDelay=350) {
      bool f_tempStat = digitalRead(pin);
      if (f_tempStat == onState) {            // if led is already on
        off();
        delay(f_ledDelay);
      }
      for (int i=0; i<f_reps; i++) {
        on();
        delay(f_ledDelay);
        off();
        delay(f_ledDelay);
      }
      if (f_tempStat == onState) on();          // return led status
    }
}; 


// --------------------------------------------------------------------------------------
//                                   -button control code
// --------------------------------------------------------------------------------------
// Supply gpio pin, normal state (i.e. high or low when not pressed)
// useage:      Button button1(GPIO_PIN, HIGH);     if (button1.isPressed()) {};
// Note: Because of the debouncing used it needs to be polled regularly as a single check will be ignored

class Button {
  
  private:
    byte pin;                                 // gpio pin
    bool normalState;                         // pin state when button not pressed
    bool state;                               // current status of button
    unsigned long debounceDelay = 40;         // delay time for debouncing (ms)
    
  public:
    Button(byte b_pin, bool b_normalState = LOW) {
      this->pin = b_pin;
      this->normalState = b_normalState;
      init();
    }

    void init() {
      pinMode(pin, INPUT);
      state = digitalRead(pin);               // store current state of button
    }

    void update() {
      bool u_newReading = digitalRead(pin);
      if (u_newReading != state) {            // if the button status has changed
        delay(debounceDelay);
        u_newReading = digitalRead(pin);
      }
      state = u_newReading;
    }

    bool getState() {                         // returns the logic state of the gpio pin
      update();
      return state;
    }

    bool isPressed() {                        // returns TRUE if button is curently pressed
      if (normalState == LOW) return getState();
      else return (!getState());
    }
}; 


// --------------------------- E N D -----------------------------
