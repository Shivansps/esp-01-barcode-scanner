/*
  This code creates a ESP-01 barcode reader, using a USB barcode reader that also supports native RS232 mode (it involves in changing the cable or the cable pin position in the reader)
  Using a MAX3232 the RS232 signal is converted to TTL, then it diplays the information using a SSD1306 I2C display and plays "good/error" sounds using a passive buzzer
  Buzzer will be connected to PIN 1, so the hardware serial cant be used. It also important to connect the buzzer other pin to 3.3v instead of GND otherwise the ESP-01S will not boot
  The barcode can be displayed on screen and/or be uploaded via HTTP POST to a remote web/db server using the wifi connection.
*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

/*Configuration area*/
#define RX_PIN 3
#define TX_PIN 12 //Unused
#define BUZZER_PIN 1
#define SCL_PIN 2
#define SCA_PIN 0
#define SERIAL_TIMEOUT 100 //Time in ms for the readString() function, if is not enoght to read an entire code, increase the time
#define SERIAL_BAUD 9600
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D //or 0x3C depends on your display
#define STASSID "wifi-name"
#define STAPSK  "wifi-password"
#define MAX_BARCODE_LENGHT 200 //This also sets the maximum text lengh you can write on the lower part of the display
const unsigned long wifi_interval = 3000; //Time to check for wifi rssi

/* Working Modes Codes*/
enum Mode                 { ESPINSERT,  ESPSCAN }; //Avalible working modes "ESPINSERT" inputs barcodes in DB, "ESPSCAN" only displays them on screen
const char* mode_desc[] = { "INS.EAN",  "SCAN" };  //Working modes description to show in screen
Mode wmode = ESPINSERT; //Starting working mode, this could be saved into rom so it always boots with the last working mode, not implemented

SoftwareSerial max3232;
int wifi_bars = 0;
char ui_line1[50] = "";
char ui_line2[MAX_BARCODE_LENGHT] = "";
unsigned long previous_time = 0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int determine_wifi_bars(){
  int bars = 0;
  int rssi = -100;
  if ((WiFi.status() == WL_CONNECTED)){
      rssi = WiFi.RSSI();
  }
  if (rssi > -55) { 
    bars = 5;
  } else if (rssi < -55 & rssi > -65) {
    bars = 4;
  } else if (rssi < -65 & rssi > -70) {
    bars = 3;
  } else if (rssi < -70 & rssi > -78) {
    bars = 2;
  } else if (rssi < -78 & rssi > -82) {
    bars = 1;
  }
  return bars;
}

void ui_wifi_str(){
  wifi_bars=determine_wifi_bars();
  for (int b=0; b <= wifi_bars; b++) {
    display.fillRect(110 + (b*3),15 - (b*3),1,b*3,WHITE); 
  }
}

void ui_write_text(const char *text, int x, int y, int size=1){
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.println(text);
}

/*
  This function is called to create sound using the passive buzzer
*/
void sound (int freq, int duration){
  tone(BUZZER_PIN, freq);
  delay(duration);
  noTone(BUZZER_PIN);  
}

/*
 * This is called to refresh display information
*/
void ui_draw(){
   display.clearDisplay();
   ui_write_text(mode_desc[wmode],0,0,2);
   ui_write_text(ui_line1,0,20,2);
   ui_write_text(ui_line2,0,40,1);
   ui_wifi_str();
   display.display();  
}

/*
 * Here we check for special barcodes, i used code 128
 * Barcode "ESPSHOWIP" display current ip asigned by router
 * Barcode "ESPSCAN" and "ESPINSERT" changes the working mode
*/
bool check_special_codes(){
    if(strcmp(ui_line2,"ESPSHOWIP") == 0){
      strcpy(ui_line1,"WIFI IP:");
      sound(1500,200);
      if ((WiFi.status() == WL_CONNECTED))
        strcpy(ui_line2,WiFi.localIP().toString().c_str()); 
      else 
        strcpy(ui_line2,"WIFI DISCONNECTED!"); 
      sound(1900,100);
      sound(1900,100);
      return true;
    }
    if(strcmp(ui_line2,"ESPSCAN") == 0){
      wmode = ESPSCAN;
      strcpy(ui_line1,"CHG. MODE");
      strcpy(ui_line2,"Working mode changed to only scanner");
      sound(1900,100);
      sound(1900,100);
      return true;
    }
    if(strcmp(ui_line2,"ESPINSERT") == 0){
      wmode = ESPINSERT;
      strcpy(ui_line1,"CHG. MODE");
      strcpy(ui_line2,"Mode changed to inset barcodes in database");
      sound(1900,100);
      sound(1900,100);
      return true;
    }
    return false;
}

/*
  If the barcode is not a special code, so something based on current working mode
*/
void action(){
  switch(wmode){
    case ESPINSERT: { 
                        WiFiClient client;
                        HTTPClient http;
                        http.setTimeout(1000);
                        String url = "http://SERVERIP/insert_barcode.php";
                        http.begin(client, url);
                        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
                        String httpRequestData = "apikey=XXXXXX&barcode="+String(ui_line2);
                        int httpResponseCode = http.POST(httpRequestData);
                        if (httpResponseCode==200) {
                          /*If everything is OK display answer on display: inserted, duplicated, etc*/
                          String payload = http.getString();
                          payload.toCharArray(ui_line1, payload.length());
                          sound(1500,200);    
                        } else {
                          /* 404, 500, -1 for http timeout*/
                          String error = "ERROR:" + String(httpResponseCode);
                          strcpy(ui_line1,error.c_str());
                          sound(500,100); 
                          sound(500,100); 
                        }
                        http.end();
                        break;
                    }

    case ESPSCAN:  {
                       strcpy(ui_line1,"SCANNED");
                       break;
                   }                  
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  WiFi.begin(STASSID, STAPSK);
  delay(500);
  Wire.pins(SCA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {for(;;);}
  max3232.begin(SERIAL_BAUD, SWSERIAL_8N1, RX_PIN, TX_PIN, true); //This last "true" inverts data coming from serial, i need to use it for my reader
  max3232.setTimeout(SERIAL_TIMEOUT);
  while (!max3232.isListening()) {delay(1000);}
  ui_draw();
  sound(800,300);
}

void loop() {
  unsigned long current_time = millis();
  if (current_time - previous_time >= wifi_interval) {
    previous_time = current_time;
    if(wifi_bars!=determine_wifi_bars()){
      ui_draw();
    }
  }
  
  if (max3232.available()) {
    String buffer = max3232.readString();
    if(buffer.length()<MAX_BARCODE_LENGHT){
      buffer.toCharArray(ui_line2, buffer.length());      
      if(!check_special_codes()){
        action();
      }
    }
    ui_draw();
  }
}
