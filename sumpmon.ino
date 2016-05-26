#include <SPI.h>
#include <Wire.h>
#include <Adafruit_WINC1500.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "dtostrf.h"

#ifdef DISPLAY_SSD1306
  #ifdef DISPLAY_I2C
    Adafruit_SSD1306 display = Adafruit_SSD1306();
  #elif DISPLAY_HWSPI
    Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
  #else
    Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  #endif
#endif


long last_update=0;

float interpolation_slope;
float interpolation_offset;


// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

int status = WL_IDLE_STATUS;

Adafruit_WINC1500Client client;

void printWifiData() {

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
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

#ifdef DISPLAY_SSD1306  
  display.print("SSID: ");
  display.println(WiFi.SSID());
  display.print("IP: ");
  display.println(ip);
  display.print("Signal: ");
  display.print(WiFi.RSSI());
  display.print("dB");
  display.display();
#endif
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

#ifdef DISPLAY_1306
  display.println("Connecting to WiFi");
#endif

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
  printWifiData();

}

void setup() {
  pinMode(SENSOR_PIN,INPUT);
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Got serial");
  
  interpolation_slope = (float)(HIGH_LEVEL-LOW_LEVEL) / (float)(ADC_HIGH - ADC_LOW);
  interpolation_offset = (float)LOW_LEVEL - ((float)ADC_LOW * interpolation_slope);
  Serial.print("interpolation slope: ");
  Serial.println(interpolation_slope);
  Serial.print("interpolation offset: ");
  Serial.println(interpolation_offset);

#ifdef DISPLAY_SSD1306
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
#endif

  setup_wifi();
  delay(1000);
}

void loop() {
  float reading=analogRead(SENSOR_PIN);
  float depth = reading * interpolation_slope + interpolation_offset;

#ifdef DISPLAY_SSD1306
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Reading: ");
  display.println((int)reading);
  display.print("Depth: ");
  display.print(depth);
  display.println(" in.");
  display.display();
#endif

  delay(250);

  if (last_update == 0 || (millis() > last_update+UPDATE_MS)) {
    char post_data[256];
    char content_length[256];
    Serial.println("sending reading");
    if (client.connected()) {
      client.stop();
    }
    if (client.connect(LOG_SERVER,LOG_PORT)) {
      Serial.println("connected to server");
      client.print("POST ");
      client.print(LOG_PATH);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(LOG_SERVER);
      client.println("Content-Type: application/x-www-form-urlencoded");
      char depth_str[20];
      dtostrf(depth,0,1,depth_str);
      sprintf(post_data,"%s value=%s",LOG_PARAM,depth_str);
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
    Serial.print("Analog reading: ");
    Serial.println(reading);
    Serial.print("Depth interpolation: ");
    Serial.println(depth);


/*    reading = (1023 / reading)  - 1;
    reading = SERIES_RESISTOR / reading;
    Serial.print("Sensor resistance "); 
    Serial.println(reading);
    Serial.println(); */

    
    last_update = millis();
  }
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

}
