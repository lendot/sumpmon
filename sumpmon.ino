#include <SPI.h>
#include <Wire.h>
#include <Adafruit_WINC1500.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

long last_update=0;

/* Uncomment this block to use hardware SPI
#define OLED_DC     6
#define OLED_CS     7
#define OLED_RESET  8
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
*/

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

int status = WL_IDLE_STATUS;

Adafruit_WINC1500Client client;

void printWifiData() {

  display.print("SSID: ");
  display.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);
  display.print("IP: ");
  display.println(ip);

  display.print("Signal: ");
  display.print(WiFi.RSSI());
  display.print("dB");

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
  display.display();
}

void setup_wifi()
{
#ifdef WINC_EN
  pinMode(WINC_EN, OUTPUT);
  digitalWrite(WINC_EN, HIGH);
#endif

  Serial.println("Setting up WiFi");
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASS);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  //printCurrentNet();
  printWifiData();
  
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Got serial");
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  
  // Clear the buffer.
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, World!");
  display.display();

  setup_wifi();
}

void loop() {
  if (last_update == 0 || (millis() > last_update+UPDATE_MS)) {
    char post_data[256];
    char content_length[256];
    Serial.println("sending reading");
    if (client.connect(LOG_SERVER,LOG_PORT)) {
      Serial.println("connected to server");
      client.print("POST ");
      client.print(LOG_PATH);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(LOG_SERVER);
      client.println("Content-Type: application/x-www-form-urlencoded");
      sprintf(post_data,"%s value=%d",LOG_PARAM,12);
      sprintf(content_length,"Content-Length: %d",strlen(post_data));
      client.println(content_length);
      Serial.println(content_length);
      client.println();
      client.print(post_data);
      Serial.println(post_data);
      //client.println();
    }
    else {
      Serial.println("Unable to connect");
    }
    last_update = millis();
  }
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

}
