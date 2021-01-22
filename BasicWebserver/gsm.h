/************************************************************************************************** 
 *
 *                             GSM module - 22jan21
 * 
 *
 *  See:  https://lastminuteengineers.com/a6-gsm-gprs-module-arduino-tutorial/
 *        https://randomnerdtutorials.com/sim900-gsm-gprs-shield-arduino/
 *        AT commands: https://radekp.github.io/qtmoko/api/atcommands.html
 *
 *
 *  Note: 
 *        takes a few seconds for GSM module to connect to network before it will send any replys
 *        needs good 5v supply
 *        RST low to turn off
 *       
 *        If the GSM module keeps turning off it is probably the power supply can't supply enough current
 *       
 *        If you send it a text message containing "send sms" it will send a reply message
 *
 *        A standard received sms will look like this:
 *                                                      +CIEV: "MESSAGE",1
 *                                                     
 *                                                      +CMT: "+441111111111",,"2021/01/20,10:46:09+00"
 *                                                      This is a test text message   
 *       
 *        A message from GiffGaff will look like this (i.e. no date or number info):
 *                                                      +CIEV: "MESSAGE",1
 *                                                     
 *                                                      +CMT: "Hi, that text was charged from your giffgaff credit balance. Your credit balance is now ⸮2.40.
 *                                                      
 *
 **************************************************************************************************


 Notes: 

    Only tested with a AI Thinker A6 board so far

    You can issue AT commands to the GSM module via Serial Monitor 

    To set custom actions on incoming data from the GSM module see dataReceivedFromGSM()
        e.g. respond to incoming sms message or incoming data requested via requestWebPageGSM()

    For a Sim to use in your GSM module I recommend GiffGaff, they are good value and do not seem to get disconnected if you do not top
        them up very often.  If you top it up every 3 months (£10 min.) then text messages between Giffgaff phones are not charged for.

*/

 //            --------------------------- settings -------------------------------


const String phoneNumber = "+441111111111";        // standard phone number to send sms messages to 

const String GSM_APN = "giffgaff.com";             // APN for mobile data

const int GSMresetPin = -1;                        // GSM reset pin (-1 = not used)
bool GSMresetPinActive = 1;                        // reset active state (i.e. 1 = high to reset device)

const int TxPin = D5;                              // Tx pin
const int RxPin = D6;                              // Rx pin

int checkGSMmodulePeriod = 30000;                  // how often to check GSM module is still responding ok (ms)

int checkGSMdataPeriod = 500;                      // how often to check for incoming data from GSM module (ms)

const int GSMbuffer = 512;                         // buffer size for incoming data from GSM module


// --------------------------------------------------------------------------


// forward declarations
  String contactGSMmodule(String);
  void sendSMS(String , String);
  bool checkGSMmodule(int);
  bool resetGSM(int);
  void setupGSM();
  void requestWebPageGSM(String);
  void dataReceivedFromGSM();


#include <SoftwareSerial.h>    // Note: the esp32 has a second hardware serial port you can use instead of SoftwareSerial

uint32_t checkGSMmoduleTimer = millis();    // timer for periodic gsm module check
uint32_t checkGSMdataTimer = millis();      // timer for periodic check for incoming data from GSM module

//Create software serial object to communicate with GSM module
  SoftwareSerial GSMserial(TxPin, RxPin);     
  


// ----------------------------------------------------------------
//                     -act on any incoming data
// ----------------------------------------------------------------
// periodic check for any incoming data from gms module

void dataReceivedFromGSM() {

    String reply = contactGSMmodule("");  // check gsm module for any incoming data on serial
    if (reply == "") return;   // no incoming data from GSM module

        
    // check for incoming SMS message 
      if (reply.indexOf("+CIEV: \"MESSAGE\"") >= 0) {       // search for:     +CIEV: "MESSAGE"
        int pos = reply.indexOf("+CMT: ");                  // search for:     +CMT:
            if (pos >= 0) {
              // a sms has been received
                String smsMessage = reply.substring(pos + 6);
                if (serialDebug) {
                  Serial.println("SMS message received:");
                  Serial.println(smsMessage);
                }
            } else if (serialDebug) Serial.println("Error in received SMS message");
      }


//    // send a test sms message if "send sms" in a received sms message
//      if (reply.indexOf("send sms") >= 0) sendSMS(phoneNumber, "this is a test message from the gsm demo sketch");
//
//
//    // request a web page via GSM data if "get web" received in a sms message
//      if (reply.indexOf("get web") >= 0) requestWebPageGSM("http://alanesq.eu5.net/temp/q.txt");  
            
}


// ----------------------------------------------------------------
//                       -Setup GSM board
// ----------------------------------------------------------------
//  setup for GSM board - called from setup

void setupGSM() {

  // configure reset pin
    if (GSMresetPin != -1) {
        digitalWrite(GSMresetPin, !GSMresetPinActive);     // set to not resetting
        pinMode(GSMresetPin, OUTPUT);
    }  

  //Begin serial communication with GSM module 
    GSMserial.begin(9600, SWSERIAL_8N1, D5, D6, false, GSMbuffer); while(!GSMserial) delay(200);

  // check GSM module is responding (up to 40 attempts, this will set the flag 'GSMconnected')
    checkGSMmodule(40);

  if (GSMconnected) {
    contactGSMmodule("ATI");             // Get the module name and revision
    if (serialDebug) Serial.println("GSM setup completed OK");
  } else {
    if (serialDebug) Serial.println("ERROR: GSM module is not responding");
  }
    
}   // setupGSM


// ----------------------------------------------------------------
//              -loop, routine activites for GSM module
// ----------------------------------------------------------------
// called from 'Loop'

void GSMloop() {

  if (GSMconnected) {

    // periodic check that GSM module is still responding
      if ((unsigned long)(millis() - checkGSMmoduleTimer) >= checkGSMmodulePeriod ) {
          checkGSMmoduleTimer = millis();
          if (!checkGSMmodule(2)) {
            if (serialDebug) Serial.println("ERROR: GSM module has stopped responding");
          }
      }    

    // periodic check for any incoming data on serial or from gms module
      if ((unsigned long)(millis() - checkGSMdataTimer) >= checkGSMdataPeriod ) {
        checkGSMdataTimer = millis();
        dataReceivedFromGSM();
      } 
      
  }  // GSMconnected

}  // GSMloop


// ----------------------------------------------------------------
//                        -reset GSM module
// ----------------------------------------------------------------

bool resetGSM(int GSMcheckRetries) {
    
  if (GSMresetPin == -1) {
    if (serialDebug) Serial.println("Unable to reset GSM device as no reset pin defined");
    return 0;
  }
  
  if (serialDebug) Serial.println("Resetting GSM device");

  digitalWrite(GSMresetPin, GSMresetPinActive);    // reset active
  delay(1000);
  digitalWrite(GSMresetPin, !GSMresetPinActive);    // restart device
  delay(2000);

  checkGSMmodule(GSMcheckRetries);
  return GSMconnected; 
}
 
 
// ----------------------------------------------------------------
//                -check gsm module is responding
// ----------------------------------------------------------------
// sets the flag 'GSMconnected'

bool checkGSMmodule(int maxTries) {

  if (serialDebug) Serial.println("Checking GSM module is responding");   
  bool GSMconnectedCurrent = GSMconnected;    // store current GSM status

  String reply;
  GSMconnected = 0;
  while (GSMconnected == 0  && maxTries > 0) {
    reply = contactGSMmodule("AT");
    if (reply.indexOf("OK") >= 0) GSMconnected = 1;
    maxTries--;
  }
  
  if (serialDebug){
    if (!GSMconnected) Serial.println("No response received from GSM device");
    else Serial.println("GSM device responding ok");
  }

  // if GSM wasn't connected but now is send some configuration
  //   see:  https://oldlight.wordpress.com/2009/06/16/tutorial-using-at-commands-to-send-and-receive-sms/
  if (GSMconnected && !GSMconnectedCurrent) {
    if (serialDebug) Serial.println("Configuring incoming SMS");
    contactGSMmodule("AT+CNMI=1,2,0,0,0");           // Set module to send SMS data to serial upon receipt 
    contactGSMmodule("AT+CMGF=1");                   // format sms as text
  }

  return GSMconnected;
      
}   // GSMmodule


// ----------------------------------------------------------------
//                       send a sms message
// ----------------------------------------------------------------

void sendSMS(String SMSnumber, String SMSmessage) {

  if (serialDebug) Serial.println("Sending SMS to '" + SMSnumber + "', message = '" + SMSmessage + "'");

  contactGSMmodule("AT+CMGF=1");                         // put in to sms mode
  
  contactGSMmodule("AT+CMGS=\"" + SMSnumber + "\"");     // e.g. "AT+CMGS=\"+ZZxxxxxxxxxx\"" - change ZZ with country code and xxxxxxxxxxx with phone number to sms
  
  contactGSMmodule(SMSmessage);                          // the message to send
  
  GSMserial.write(26);                                   // Ctrl Z
  
}
/*
 
Response when text was sent ok  (the 4 changes):
                              +CMGS: 4
                              
                              OK
*/



// ----------------------------------------------------------------
//                   Request web page via GSM
// ----------------------------------------------------------------

void requestWebPageGSM(String URL) {

  if (serialDebug) Serial.println("Requesting web page via GSM '" + URL + "', message = '");

  contactGSMmodule("AT+CSTT=\"" + GSM_APN + "\",\"\",\"\"");        // set APN
    
  contactGSMmodule("AT+CIICR");                                     // connect

  delay(5000);
  
  contactGSMmodule("AT+CIFSR");                                     // display IP address

  contactGSMmodule("AT+HTTPGET=\"" + URL + "\"");                   // request URL
  
}
/*

 reply example:
                  +HTTPRECV:HTTP/1.1 200 OK
                  Date: Thu, 21 Jan 2021 16:49:46 GMT
                  Server: Apache
                  Last-Modified: Thu, 21 Jan 2021 07:37:32 GMT
                  ETag: "9-5b96424ff7617"
                  Accept-Ranges: bytes
                  Content-Length: 9
                  Keep-Alive: timeout=4, max=90
                  Connection: Keep-Alive
                  Content-Type: text/plain
                  
                  it works
*/


// ----------------------------------------------------------------
//                 exchange data with GSM module
// ----------------------------------------------------------------
// GSMcommand is sent to GSM module
// If data being lost try increasing 'GSMbuffer' size

String contactGSMmodule(String GSMcommand) {

  int delayTime = 500;                        // length of delays
  char replyStore[GSMbuffer + 1];             // store for reply from GSM module
  int received_counter = 0;                   // number of characters which have been received

  // if a command was supplied send it to GSM module
    if (GSMcommand != "") {
      if (serialDebug) Serial.println("Sending command to GSM module: '" + GSMcommand +"'");
      GSMserial.println(GSMcommand);
      delay(delayTime);
    }
  
  // forward any incoming data an serial to the gsm module
    if (GSMserial.available()) delay(delayTime);      // make sure full message has time to come in first
    while (Serial.available()) {
        GSMserial.write(Serial.read());   
    }
  
  // if any data coming in from GSM module copy it to a string
    if (GSMserial.available()) delay(delayTime);    // make sure full message has time to come in first
    while(GSMserial.available() && received_counter < GSMbuffer) {
        char tstore = char(GSMserial.read());
        replyStore[received_counter] = tstore;      // Forward what Software Serial received to a String
        received_counter+=1;
    }
    replyStore[received_counter] = 0;               // end of string marker

  if (received_counter > 0 && serialDebug) {
    Serial.println("--------------- Data received from gsm module --------------");
    Serial.print(replyStore);
    Serial.println("------------------------------------------------------------");
  }

//  if (!strstr(replyStore, "OK")) {
//    // No valid reply received
//      return "ERROR";
//  }
  
  return replyStore;
}


// --------------------------------------------------------------------------

// end
