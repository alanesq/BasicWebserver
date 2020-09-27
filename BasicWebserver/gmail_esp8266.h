/**************************************************************************************************
 * 
 *      Send emails from ESP8266 via Gmail    
 * 
 *      include in main sketch if sending emails is required with command     #include "gmail.h"
 *    
 *      uses: 'EmailSender' library     More info: https://www.mischianti.org/2019/09/10/send-email-with-esp8266-and-arduino/
 * 
 * 
 *      This demo sketch will fail at the Gmail login unless your Google account has
 *      set the following option:     Allow less secure apps: ON       
 *                                see:  https://myaccount.google.com/lesssecureapps
 *
 *                                            Gmail - v2.0  - 27Sep20          
 *  
 **************************************************************************************************

 Usage:             // send email
                      String emessage = "<message here>";
                      byte q = sendEmail(emailReceiver,"<subject here>", emessage);    
                      if (q==0) log_system_message("email sent ok" );
                      else log_system_message("Error sending email code=" + String(q) ); 


 
 ************************************************************************************************** 
 */


// forward declarations
  byte sendEmail();



// ----------------------------------------------------------------
//                              -Startup
// ----------------------------------------------------------------


//               s e t t i n g s 

  char* emailReceiver = "<your email address here>";         // address to send emails
  
  char* _mailUser = "<email address to send from here>";
  
  char* _mailPassword = "<email password here>";
  
  

#include <EMailSender.h>        

EMailSender emailSend(_mailUser, _mailPassword);



// ----------------------------------------------------------------
//                     -Send an email via gmail
// ----------------------------------------------------------------

byte sendEmail(String emailTo, String emailSubject, String emailBody)
{
  Serial.println("\n----- sending an email -------");

    EMailSender::EMailMessage message;

    // convert inputs from Strings to char*
    char c[emailSubject.length()];
    emailSubject.toCharArray(c, emailSubject.length() + 1);
    char* _emailSubject = c;
    
    char d[emailBody.length()];
    emailBody.toCharArray(d, emailBody.length() + 1);
    char* _emailBody = d;
    
    char e[emailTo.length()];
    emailTo.toCharArray(e, emailTo.length() + 1);
    char* _emailTo = e;
    
    
    message.subject = _emailSubject;
    message.message = _emailBody;
    
    EMailSender::Response resp = emailSend.send(_emailTo, message);
    

    Serial.println("Sending status: ");
    Serial.println(resp.status);
    Serial.println(resp.code);       // responses:    0=message sent, 1=SMTP Response TIMEOUT, 2=Could not connect to mail server.
    Serial.println(resp.desc);

    // convert result code from string to byte
      byte tresult = 100;
      if (resp.code == "0") tresult=0;
      if (resp.code == "1") tresult=1;
      if (resp.code == "2") tresult=2;    
  
  Serial.println("------ end of email send -----");
  return tresult;

}


// --------------------------- E N D -----------------------------
