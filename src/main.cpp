#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "Dhole_weather_icons32px.h"
#include <WiFi.h>
#include "WiFiManager.h"  

/*--------------------- DEBUG  -------------------------*/
#define Sprintln(a) (Serial.println(a))
#define SprintlnDEC(a, x) (Serial.println(a, x))
#define Sprint(a) (Serial.print(a))
#define SprintDEC(a, x) (Serial.print(a, x))
/*--------------------- Cấu hình bảng led-------------------------*/
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN -1
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1
 
MatrixPanel_I2S_DMA *dma_display = nullptr;
HUB75_I2S_CFG mxconfig(
	PANEL_RES_X,
	PANEL_RES_Y,
	PANEL_CHAIN
);

/*----------------------Biến timer-------------------*/
unsigned long previousMillis = 0;
const long interval = 1000;

unsigned long Bitmap_previousMillis = 0;
const long Bitmap_interval = 2000;
/*----------------------Bitmap Wifi------------------*/
const char wifi_image1bit[] PROGMEM   =  {
 0x00,0x00,0x00,0xf8,0x1f,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0xff,0x01,0x00,0x00,0x00,0x00,0xf0,0xff,0xff,0x07,0x00,0x00,0x00,0x00,0xfc,0xff,0xff,0x1f,
 0x00,0x00,0x00,0x00,0xfe,0x07,0xe0,0x7f,0x00,0x00,0x00,0x80,0xff,0x00,0x00,0xff,0x01,0x00,0x00,0xc0,0x1f,0x00,0x00,0xf8,0x03,0x00,0x00,0xe0,0x0f,0x00,
 0x00,0xf0,0x07,0x00,0x00,0xf0,0x03,0xf0,0x0f,0xc0,0x0f,0x00,0x00,0xe0,0x01,0xff,0xff,0x80,0x07,0x00,0x00,0xc0,0xc0,0xff,0xff,0x03,0x03,0x00,0x00,0x00,
 0xe0,0xff,0xff,0x07,0x00,0x00,0x00,0x00,0xf8,0x0f,0xf0,0x1f,0x00,0x00,0x00,0x00,0xfc,0x01,0x80,0x3f,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,0x3e,0x00,0x00,
 0x00,0x00,0x38,0x00,0x00,0x1c,0x00,0x00,0x00,0x00,0x10,0xe0,0x07,0x08,0x00,0x00,0x00,0x00,0x00,0xfc,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0x7f,0x00,
 0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0xf8,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x00,0x00,0x00,
 0x00,0xc0,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0x03,0x00,0x00,0x00,
 0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void drawXbm565(int x, int y, int width, int height, const char *xbm, uint16_t color = 0xffff) 
{
  if (width % 8 != 0) {
      width =  ((width / 8) + 1) * 8;
  }
    for (int i = 0; i < width * height / 8; i++ ) {
      unsigned char charColumn = pgm_read_byte(xbm + i);
      for (int j = 0; j < 8; j++) {
        int targetX = (i * 8 + j) % width + x;
        int targetY = (8 * i / (width)) + y;
        if (bitRead(charColumn, j)) {
          dma_display->drawPixel(targetX, targetY, color);
        }
      }
    }
}

/* Bitmaps */
int current_icon = 0;
static int num_icons = 22;

static char icon_name[22][30] = {"cloud_moon_bits", "cloud_sun_bits", "clouds_bits", "cloud_wind_moon_bits", "cloud_wind_sun_bits", "cloud_wind_bits", "cloud_bits",
"lightning_bits", "moon_bits", "rain0_sun_bits", "rain0_bits", "rain1_moon_bits", "rain1_sun_bits", "rain1_bits", "rain2_bits", "rain_lightning_bits",
"rain_snow_bits", "snow_moon_bits", "snow_sun_bits", "snow_bits", "sun_bits", "wind_bits" };

static char *icon_bits[22] = { cloud_moon_bits, cloud_sun_bits, clouds_bits, cloud_wind_moon_bits, cloud_wind_sun_bits, cloud_wind_bits, cloud_bits,
lightning_bits, moon_bits, rain0_sun_bits, rain0_bits, rain1_moon_bits, rain1_sun_bits, rain1_bits, rain2_bits, rain_lightning_bits, rain_snow_bits,
snow_moon_bits, snow_sun_bits, snow_bits, sun_bits, wind_bits};

/*----------------------Cấu hình Wifi----------------------------*/
const char* ssid     = "204P";
const char* password = "1234567889";
//Tên wifi + mật khẩu cho access point
const char* AP_ssid = "ESP32-Access-Point";
const char* AP_password = "12345678";

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void wifi_manage(){
  WiFi.begin(ssid, password);
  int r = 0;
  int time_out_wifi_connect = 0;
  while ((WiFi.status() != WL_CONNECTED) && time_out_wifi_connect < 600 ) {  
    time_out_wifi_connect++;
    if(r < 255)
    {
      drawXbm565(0,0,64,32, wifi_image1bit, dma_display->color565(r,0,0));
      r++;
      delay(10);
    }
    Serial.print("Test 1");
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);
    WiFiManager wifiManager;
    wifiManager.startConfigPortal("LedMatrix", "1234567890");
    wifiManager.setAPCallback(configModeCallback);
    String ssid = WiFi.SSID();
    String password = WiFi.psk();
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
  }
}


void bitmap();

void setup() {

  delay(100); Serial.begin(115200); delay(100);
  /************** DISPLAY **************/
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(90); //0-255
  dma_display->clearScreen(); 
  //dma_display->fillScreen(dma_display->color444(0, 1, 0));  

  /*-----------------Connect Wifi-----------------*/\
  wifi_manage();
  if(WiFi.status() == WL_CONNECTED){
    Serial.print("Đã kết nối");
    drawXbm565(0,0,64,32, wifi_image1bit, dma_display->color565(255,0,0));
  }
  else{
    Serial.print("Ngắt kết nối");
  }
  delay(2000);
  dma_display->clearScreen();
}


void loop() {
  unsigned long currentMillis = millis();

    bitmap();
}


void bitmap(){
  unsigned long Bitmap_currentMillis = millis();

  drawXbm565(0,0, 32, 32, icon_bits[current_icon]);
    if (Bitmap_currentMillis - Bitmap_previousMillis >= Bitmap_interval) {
        Bitmap_previousMillis = Bitmap_currentMillis; // Cập nhật thời gian trước đó

        Serial.print("Showing icon ");
        Serial.println(icon_name[current_icon]);
        current_icon = (current_icon  +1 ) % num_icons;
        dma_display->clearScreen();
    }
  
}