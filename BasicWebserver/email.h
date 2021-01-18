/**************************************************************************************************
 * 
 *    Send emails from ESP8266/ESP32 via Gmail v2.0 
 *    
 *    using ESP_Mail_Client library  -  https://github.com/mobizt/ESP-Mail-Client
 *    
 * 
 *    include in main sketch if sending emails is required with command     #include "gmail.h"
 *    
 * 
 * 
 * This demo sketch will fail at the Gmail login unless your Google account has
 * set the following option:     Allow less secure apps: ON       
 *                               see:  https://myaccount.google.com/lesssecureapps
 *
 *                                         email - v2.0  - 18Jan21         
 *  
 **************************************************************************************************

 Usage:

 Main code to include:       
                              #include "email.h"
                              // forward declaration
                                void smtpCallback(SMTP_Status status);    // the procedure to call when status info is available
                                bool sendEmail(char*, char* , char*);        
                                        

  Using char arrays:  https://www.tutorialspoint.com/arduino/arduino_strings.htm

 
  // send a test email
      _message[0]=0; _subject[0]=0;          // clear any existing text
      strcat(_subject,"test message");
      strcat(_message,"this is a test email from the esp");
      sendEmail(_emailReceiver, _subject, _message);  


 
 ************************************************************************************************** 
 */

//               s e t t i n g s 


  #define _emailReceiver "<email to send to>"               // address to send emails
  
  #define _mailUser "<email to send from>"                  // address to send from
  
  #define _mailPassword "<email password>"                  // email password

  #define _SMTP "smtp.gmail.com"                            // smtp server address

  #define _SMTP_Port 587                                    // port to use

  #define _SenderName "BasicWebServer"                      // name of sender (no spaces)

  const int maxMessageLength = 600;                         // maximum length of email message
  const int maxSubjectLength = 150;                         // maximum length of email subject


//  ----------------------------------------------------------------------------------------


// stores for email messages
  char _message[maxMessageLength];
  char _subject[maxSubjectLength];


#include <ESP_Mail_Client.h>    

  
/* The SMTP Session object used for Email sending */
  SMTPSession smtp;


// ----------------------------------------------------------------------------------------


// Function send an email 


bool sendEmail(char* emailTo, char* emailSubject, char* emailBody) {
 
  if (serialDebug) Serial.println("----- sending an email -------");

  // Define the session config data which used to store the TCP session configuration
    ESP_Mail_Session session;

  // enable debug info on serial port
    if (serialDebug) smtp.debug(1);
  
  // Set the session config
    session.server.host_name =  _SMTP; //for outlook.com
    session.server.port = _SMTP_Port;
    session.login.email = _mailUser; //set to empty for no SMTP Authentication
    session.login.password = _mailPassword; //set to empty for no SMTP Authentication
    session.login.user_domain = _SenderName;
  
  // Define the SMTP_Message class variable to handle to message being transport
    SMTP_Message message;
  
  // Set the message headers
    message.sender.name = _SenderName;
    message.sender.email = _mailUser;
    message.subject = emailSubject;
    message.addRecipient("receiver", emailTo);
    // message.addRecipient("name2", "email2");
    // message.addCc("email3");
    // message.addBcc("email4");
  
  // Set the message content
    message.text.content = emailBody;
  
//  // Add attachment to the message
//    message.addAttachment(att);
  
  // Connect to server with the session config
    smtp.connect(&session);
  
  // Start sending Email and close the session
    if (!MailClient.sendMail(&smtp, &message)) {
      log_system_message("Sending email '" + String(_subject) +"' failed, reason=" + String(smtp.errorReason()));
      return 0;
    } else {
      log_system_message("Email '" + String(_subject) +"' sent ok  ");
      return 1;
    }
    
}


// ----------------------------------------------------------------------------------------


/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {

  if (!serialDebug) return;       // exit if serial debug info is disabled
  
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
      localtime_r(&result.timesstamp, &dt);

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
