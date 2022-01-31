<p align="center"><img src="/Images/BasicWebserver.jpg"/></p>

Works with both Arduino IDE and PlatformIO

I find that when creating pretty much any new project with an ESP8266 or ESP32 I use this sketch as a starting point, 
so I have published it here in case it is of any use/interest to anyone else (I also hope that people will offer 
improvements/suggestions as I am not a professional programmer by any stretch of the imagination).  
It is not all my own work, just something I have put together for my own use from many sources.

It is the easiest way I have found to serve web pages with control buttons, updating information, a log of activity
BTW - This sketch will automatically adjust to running on either a ESP8266 or a ESP32
For a more advanced option for showing updating info see: https://github.com/alanesq/misc/blob/main/VeryBasicWebserver.ino

It has built in support for sending emails, getting the real time from NTP, a simple Oled display menu system, GSM board support and OTA updating.

Note: It does not work with the latest esp32 Arduino addin (i.e. v2) as ESP_Wifimanager at least for me just gets an ip address of 255.255.255.255 for some reason

<table><tr>
  <td><img src="/Images/Screenshot1.png" /></td>
  <td><img src="/Images/Screenshot2.png" /></td>
</tr></table>   

The sketch as it is will first create a wifi access point named "espserver", you need to connect to this with the 
password "12345678" and you will then be able to enter your wifi network details so it is then able to connect to it 
(it uses the WifiManager library to do this which is included in the above file).  Once connected to your network you 
should be able to view a similar web page to the image above (you will need to find out what ip address it is using from 
your router or by viewing the terminal window in the Arduino IDE).

If you look in the procedure "handleRoot", the lower part is where it creates any control buttons and the top part is 
where it performs any actions when the buttons are pressed.  Any information you want to update (e.g. sensor status) 
are put in the procedure "data" as this is then automatically updated every few seconds.
It uses AJAX to update the info., this is not done in the conventional way where variable data is passed but instead passes 
complete lines of text, it may not be elegant but it makes changing what information is displayed much easier as all you 
have to do is modify what info handleData() sends.

You can then add your own code to the "loop" section to perform whatever tasks you wish...

The librarys used by the sketch are in "libraries used.zip" (copy them in to your libraries folder).
Note: I have recently updated this sketch to use newer versions of libraries so if you have an older version you will
      need to update your installed libraries.
ESP8266 addon package = v2.5.2     (I find problems if useing a newer version)
    from http://arduino.esp8266.com/stable/package_esp8266com_index.json

It now suports OTA updates (this can be disabled in the settings if preferred)
      Note - As at least some basic form of security you first need to enter a password to enable ota
             Then access with    http://x.x.x.x/ota
             
If you have several projects there is a HTML page here which can be used to give a menu of them all
https://github.com/alanesq/CameraWifiMotion/blob/master/misc/menu-of-projects.htm

