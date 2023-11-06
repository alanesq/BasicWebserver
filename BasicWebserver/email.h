/**************************************************************************************************
 *
 *                                 Send emails from ESP8266/ESP32 v2.0 
 *
 *    using ESP Mail Client@2.7.2 library  -  https://github.com/mobizt/ESP-Mail-Client
 *
 *    part of the BasicWebserver sketch - https://github.com/alanesq/BasicWebserver
 *
 *
 *   This will fail if using Gmail unless your Google account has the following option set:
 *       Allow less secure apps: ON       see:  https://myaccount.google.com/lesssecureapps
 *       Also requires POP access to be enabled.
 *   GMX.COM emails work very well with no additional setup other than enable POP access.
 *
 *                                             06Nov23
 *
 **************************************************************************************************

 Usage:

    In main code include:      #include "email.h"


    Using char arrays: // https://www.tutorialspoint.com/arduino/arduino_strings.htm


    Send a test email:
        _message[0]=0; _subject[0]=0;          // clear any existing text
        strcat(_subject,"test message");
        strcat(_message,"this is a test email from the esp");
        sendEmail(_emailReceiver, _subject, _message);

        Note: To also send an sms along with the email use:     sendSMSflag = 1;
        
    
        
  Notes:
  
    Sending emails can take up to 1min if the system time is not set


 **************************************************************************************************/


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------

  const int EmailAttemptTime = 30;                          // how often to re-attempt failed email sends (seconds)

  const int MaxEmailAttempts = 3;                           // maximum email send attempts

  //#define _SenderName "BWS"                                 // name of sender (no spaces)

  #define _UserDomain "127.0.0.1"                           // user domain to report in email

  // your email settings here
    #define _emailReceiver "<email address>"                  // address to send emails to
    #define _smsReceiver "<email address>"                    // address to send text messages to
    #define _UserDomain "<domain to report from>"             // user domain to report in email
    #define _mailUser "<email address>"                       // address to send from
    #define _mailPassword "<email password>"                  // email password
    #define _SMTP "<smtp server>"                             // smtp server address
    #define _SMTP_Port 587                                    // port to use (gmail: Port for SSL: 465, Port for TLS/STARTTLS: 587)


const int maxMessageLength = 500;                             // maximum length of email message
const int maxSubjectLength = 150;                             // maximum length of email subject


// ---------------------------------------------------------------


bool sendSMSflag = 0;                                       // if set then also send sms when any email is sent
bool emailToSend = 0;                                       // flag if there is an email waiting to be sent
uint32_t lastEmailAttempt = 0;                              // last time sending of an email was attempted
int emailAttemptCounter = 0;                                // counter for failed email attempts

#include <ESP_Mail_Client.h>

// stores for email messages
  char _message[maxMessageLength];
  char _subject[maxSubjectLength];
  char _recepient[80];

// forward declarations
    void smtpCallback(SMTP_Status status);
    bool sendEmail(char*, char* , char*);
    void EMAILloop();

/* The SMTP Session object used for Email sending */
  SMTPSession smtp;


// ---------------------------------------------------------------
//                          -Loop
// ---------------------------------------------------------------
// Called from LOOP to handle emails

void EMAILloop() {

  if (!emailToSend || emailAttemptCounter >= MaxEmailAttempts) return;     // no email to send

  // if time to attempt sending an email
    if ( lastEmailAttempt==0 || (unsigned long)(millis() - lastEmailAttempt) > (EmailAttemptTime * 1000) ) {
    
      sendEmail(_recepient, _subject, _message);      // try to send the email
      lastEmailAttempt = millis();                    // set last time attempted timer to now
      
      // if limit of tries reached
        if (emailAttemptCounter >= MaxEmailAttempts) {
          log_system_message("Error: Max email attempts exceded, email send has failed");
          emailToSend = 0;                                // clear flag that there is an email waiting to be sent
          sendSMSflag = 0;                                // clear sms send flag
          lastEmailAttempt = 0;                           // reset timer
        }    
    }

}  // EMAILloop


// ---------------------------------------------------------------
//                          -Send email
// ---------------------------------------------------------------

// Function send an email
//   see full example: https://github.com/mobizt/ESP-Mail-Client/blob/master/examples/Send_Text/Send_Text.ino

bool sendEmail(char* emailTo, char* emailSubject, char* emailBody) {

  log_system_message("Sending email: " + String(emailSubject));
  emailAttemptCounter++;         // increment attempt counter

  if (serialDebug) Serial.println("----- sending an email -------");

  // enable debug info on serial port
    if (serialDebug) smtp.debug(1);                       // turn debug reporting on
  
  smtp.callback(smtpCallback);         // Set the callback function to get the sending results
  
  // Define the session config data which used to store the TCP session configuration
    Session_Config session;

  // Set the session config
    session.server.host_name =  _SMTP;
    session.server.port = _SMTP_Port;
    session.login.email = _mailUser;
    session.login.password = _mailPassword;
    session.login.user_domain = _UserDomain;

  // Define the SMTP_Message class variable to handle to message being transported
    SMTP_Message message;
    // message.clear();

  // Set the message headers
    message.sender.name = _SenderName;
    message.sender.email = _mailUser;
    message.subject = emailSubject;
    message.addRecipient("name1", emailTo);
    if (sendSMSflag) message.addCc(_smsReceiver);

  // Set the message content
    message.text.content = emailBody;

  // set system time
    time_t t=now();     // get current time
    smtp.setSystemTime(t, 0);     // linux time, offset

  // Misc settings
    message.text.charSet = "utf-8";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_8bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_high;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
    // message.addHeader("Message-ID: <abcde.fghij@gmail.com>");    // custom message header

  // Add attachment to the message
  //    message.addAttachment(att);

  // reset watchdog timer as next step can take around 20 seconds
    #if (defined ESP32)  
      esp_task_wdt_reset();
    #endif

  // Connect to server with the session config
    if (!smtp.connect(&session)) {
      log_system_message("Sending email: failed, SMTP: '" + smtp.errorReason() + "'");
      return 0;
    }

  // Connect to server with the session config
    if (!smtp.isLoggedIn())
    {
      log_system_message("Sending email: Not logged in to smtp");
    }
    else
    {
      if (smtp.isAuthenticated())
        log_system_message("Sending email: Connected to smtp");
      else
        log_system_message("Sending email: Connected to smtp with no Auth.");
    }

  // reset watchdog timer as next step can take around 20 seconds
    #if (defined ESP32)  
      esp_task_wdt_reset();          
    #endif

  // Start sending Email 
    if (!MailClient.sendMail(&smtp, &message)) {
      log_system_message("Sending email: send failed");
      return 0;
    }

  return 1;
}


// ---------------------------------------------------------------
//                        -email callback
// ---------------------------------------------------------------

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  
  // if email has sent ok
    if (status.success()) {
      emailToSend = 0;                                  // clear flag that there is an email waiting to be sent
      sendSMSflag = 0;                                  // clear sms flag
      lastEmailAttempt = 0;                             // reset timer
      _recepient[0]=0; _message[0]=0; _subject[0]=0;    // clear all stored email text
      emailAttemptCounter = 0;                          // reset attempt counter
      smtp.sendingResult.clear();                       // clear sending results log
      log_system_message("Sending email: Email has been sent");
    } else {
      //log_system_message("Sending email: status: '" + String(status.info()) + "'");
    }

  if (!serialDebug) return;

  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    Serial.println("----------------");
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      Serial.printf("Message No: %d\n", i + 1);
      Serial.printf("Status: %s\n", result.completed ? "success" : "failed");
      Serial.printf("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      Serial.printf("Recipient: %s\n", result.recipients);
      Serial.printf("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }

}

// --------------------------- E N D -----------------------------
