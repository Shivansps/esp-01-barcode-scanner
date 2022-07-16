# **ESP-01 Barcode Reader**

This code creates a ESP-01 barcode reader, using a USB barcode reader that also supports native RS232 mode (it involves in changing the cable or the cable pin position in the reader.
Using a MAX3232 the RS232 signal is converted to TTL, then it diplays the information using a SSD1306 I2C display and plays "good/error" sounds using a passive buzzer

Buzzer will be connected to PIN 1, so the hardware serial cant be used. It also important to connect the buzzer other pin to 3.3v instead of GND otherwise the ESP-01S will not boot

The barcode can be displayed on screen and/or be uploaded via HTTP POST to a remote web/db server using the wifi connection.

## Parts
-To power the project im using a T6845-C 5V 1A Powerbank Charging module and a 18650 1500mah battery. I choosed the T6845-C because aside from charging the battery, it already comes with the 5V step-up needed for the reader and the female USB Type-A connector to plug in the reader. I soldered cables to get one of the unused data lines and the 5V. 
-A Max3232 serial conversor mini board
-I2C display, i used a 128x64 SSD1306
-A USB barcode reader that supports RS232 connection, my particular model is a handheld LC-300 that i had to open and change were the data cables pins were connected.
-A simple 3.3v regulator module to the 5V line to power the ESP-01, MAX3232, and the SSD1306 display. For this i used the Ams1117 800ma module.

## Wiring
![](https://i.imgur.com/th3fjP8.png)

## Working
![](https://i.imgur.com/rNPBNdv.jpg)
![](https://i.imgur.com/senVqkW.jpg)
