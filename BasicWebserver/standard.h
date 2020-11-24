/**************************************************************************************************
 *
 *      Standard procedures - 07Oct20
 *      
 *      part of the BasicWebserver sketch
 *      
 *             
 **************************************************************************************************/


// forward declarations
  void log_system_message(String);
  void webheader();
  void webfooter();
  void handleLogpage();
  void handleNotFound();
  String requestpage(const char*, String, uint16_t);
  void handleReboot();
  void WIFIcheck();



// ----------------------------------------------------------------
//                              -Startup
// ----------------------------------------------------------------
  
// html text colour codes (obsolete html now and should use CSS instead)
  const char colRed[] = "<font color='#FF0000'>";           // red text
  const char colGreen[] = "<font color='#006F00'>";         // green text
  const char colblue[] = "<font color='#0000FF'>";          // blue text
  const char colEnd[] = "</font>";                          // end coloured text

String system_message[LogNumber + 1];                       // system log message store




// ----------------------------------------------------------------
//                      -log a system message  
// ----------------------------------------------------------------

void log_system_message(String smes) {

  //scroll old log entries up 
    for (int i=0; i < LogNumber; i++){
      system_message[i]=system_message[i+1];
    }

  // add the new message to the end
    system_message[LogNumber] = currentTime() + " - " + smes;
  
  // also send message to serial port
    if (serialDebug) Serial.println(system_message[LogNumber]);
}



// ----------------------------------------------------------------
//                         -header (html) 
// ----------------------------------------------------------------
// HTML at the top of each web page
//    additional style settings can be included


void webheader(WiFiClient &client, char style[] = " ") {

      client.write("<!DOCTYPE html>\n");
      client.write(  "<html lang='en'>\n");
      client.write(     "<head>\n");
      client.write(       "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
      client.write(       "<link rel=\'icon\' href=\'data:,\'>\n");
      client.printf(      "<title> %s </title>\n", stitle);                            // insert sketch title
      client.write(       "<style>\n");                                                // Settings here for the top of screen menu appearance 
      client.write(         "ul {list-style-type: none; margin: 0; padding: 0; overflow: hidden; background-color: rgb(128, 64, 0);}\n");
      client.write(         "li {float: left;}\n");
      client.write(         "li a {display: inline-block; color: white; text-align: center; padding: 30px 20px; text-decoration: none;}\n");
      client.write(         "li a:hover { background-color: rgb(100, 0, 0);}\n" );
      client.printf(      "%s\n", style);                                              // insert aditional style if supplied 
      client.write(       "</style>\n");
      client.write(     "</head>\n");
      client.write(     "<body style='color: rgb(0, 0, 0); background-color: yellow; text-align: center;'>\n");
      client.write(       "<ul>\n");
      client.printf(        "<li><a href='%s'>Home</a></li>\n", HomeLink);             // home page url
      client.write(         "<li><a href='/log'>Log</a></li>\n");                      // log menu button 
      client.printf(        "<h1> <font color='#FF0000'>%s</h1></font>\n", stitle);    // sketch title
      client.write(       "</ul>\n");

}




// ----------------------------------------------------------------
//                             -footer (html)
// ----------------------------------------------------------------
// HTML at the end of each web page


void webfooter(WiFiClient &client) {

     // get mac address
       byte mac[6];
       WiFi.macAddress(mac);
     
     client.write("<br>\n"); 
     
     /* Status display at bottom of screen */
     client.write("<div style='text-align: center;background-color:rgb(128, 64, 0)'>\n");
     client.printf("<small> %s", colRed); 
     client.printf("%s %s", stitle, sversion); 
     client.printf(" | Memory: %dK", ESP.getFreeHeap() /1000); 
     client.printf(" | Wifi: %ddBm", WiFi.RSSI()); 
     // NTP server link status
        if (NTPok == 1) client.write(" | NTP Link OK");
        else client.write(" | NTP Link DOWN");
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
      log_system_message("Root page requested from: " + String(cip[0]) +"." + String(cip[1]) + "." + String(cip[2]) + "." + String(cip[3]));   
      // To send to serial port use: Serial.println(cip.toString());
      

    // build the html for /log page

      webheader(client);                    // send html page header  

      client.write("<P>\n");                // start of section
  
      client.write("<br>SYSTEM LOG<br><br>\n");
  
      // list all system messages
      for (int i=LogNumber; i != 0; i--){
        client.write(system_message[i].c_str());
        if (i == LogNumber) {
          client.printf("%s  {Most Recent Entry} %s", colRed, colEnd);          // build line of html
        }
        client.write("<br>\n");    // new line
      }
    
      client.write("<br>");
    
      // close html page
        webfooter(client);                          // send html page footer
        delay(3);
        client.stop();

}



// ----------------------------------------------------------------
//                      -invalid web page requested
// ----------------------------------------------------------------

void handleNotFound() {
  
  log_system_message("invalid web page requested");      
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
//        -request a web page and return reply as a string
// ----------------------------------------------------------------
//
//     parameters = ip address, page to request, port to use (usually 80)     e.g.   "alanesq.com","/index.htm",80


String requestpage(const char* ip, String page, uint16_t port){

  if (serialDebug) {
    Serial.print("requesting web page: ");
    Serial.println(ip + page);
  }
  //log_system_message("requesting web page");      

  // Connect to the site 
    WiFiClient client;
    if (!client.connect(ip, port)) {
      if (serialDebug) Serial.println("Connection failed :-(");
      log_system_message("web connection failed");      
      return "connection failed";
    }  
    if (serialDebug) Serial.println("Connected to host - sending request...");


    // request the page
    client.print(String("GET " + page + " HTTP/1.1\r\n") +
                 "Host: " + ip + "\r\n" + 
                 "Connection: close\r\n\r\n");
  
    if (serialDebug) Serial.println("Request sent - waiting for reply...");
  
    //Wait up to 5 seconds for server to respond then read response
    uint16_t i = 0;
    while ((!client.available()) && (i < 500)) {
      delay(10);
      i++;
    }
    
    String wpage="";    // reply stored here
    
    // Read the entire response up to 200 characters
    while( (client.available()) && (wpage.length() <= 200) ) {
      wpage += client.readStringUntil('\r');     
    }
    if (serialDebug) {
      Serial.println("-----received web page--------");
      Serial.println(wpage);
      Serial.println("------------------------------");
    }

    client.stop();    // close connection
    if (serialDebug) Serial.println("Connection closed.");

  return wpage;

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


void WIFIcheck() {
  
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

}


// --------------------------- E N D -----------------------------
