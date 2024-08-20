#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "Dhole_weather_icons32px.h"
//Thư viện Wifi - MQTT Client
#include <WiFi.h>
#include "WiFiManager.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
//Thư viện EEPROM
#include <EEPROM.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <AnimatedGIF.h>


#define EEPROM_SIZE 100

/*--------------------- DEBUG  -------------------------*/
#define Sprintln(a) (Serial.println(a))
#define SprintlnDEC(a, x) (Serial.println(a, x))
#define Sprint(a) (Serial.print(a))
#define SprintDEC(a, x) (Serial.print(a, x))
/*--------------------- Cấu hình bảng led-------------------------*/
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

void colorWaveEffect(float timeOffset) {
  for (int x = 0; x < dma_display->width(); x++) {
    for (int y = 0; y < dma_display->height(); y++) {
      uint8_t r = (sin(x * 0.1 + timeOffset) + 1) * 127; // Sóng màu đỏ
      uint8_t g = (sin(y * 0.1 + 2 + timeOffset) + 1) * 127; // Sóng màu xanh lá
      uint8_t b = (sin((x + y) * 0.1 + timeOffset) + 1) * 127; // Sóng màu xanh dương
      dma_display->drawPixel(x, y, dma_display->color565(r, g, b));
    }
  }
}

/* Bitmaps */
int current_icon = 0;
static int num_icons = 22;
/*Aurora*/
int lastPattern = 0;

static char icon_name[22][30] = {"cloud_moon_bits", "cloud_sun_bits", "clouds_bits", "cloud_wind_moon_bits", "cloud_wind_sun_bits", "cloud_wind_bits", "cloud_bits",
"lightning_bits", "moon_bits", "rain0_sun_bits", "rain0_bits", "rain1_moon_bits", "rain1_sun_bits", "rain1_bits", "rain2_bits", "rain_lightning_bits",
"rain_snow_bits", "snow_moon_bits", "snow_sun_bits", "snow_bits", "sun_bits", "wind_bits" };

static char *icon_bits[22] = { cloud_moon_bits, cloud_sun_bits, clouds_bits, cloud_wind_moon_bits, cloud_wind_sun_bits, cloud_wind_bits, cloud_bits,
lightning_bits, moon_bits, rain0_sun_bits, rain0_bits, rain1_moon_bits, rain1_sun_bits, rain1_bits, rain2_bits, rain_lightning_bits, rain_snow_bits,
snow_moon_bits, snow_sun_bits, snow_bits, sun_bits, wind_bits};

/*----------------------Cấu hình Wifi----------------------------*/
void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void wifi_manage();
void bitmap();

void setup() {

  delay(100);
  Serial.begin(115200); //Khởi tạo Serial

/*-----------------------------------Lấy giá trị eeproom-----------------------------------*/
  EEPROM.begin(EEPROM_SIZE); // Khởi tạo EEPROM với kích thước xác định
  char ssid_er[50];
  char pass_er[20];
  size_t bytes_ssid_read = EEPROM.readString(0, ssid_er, sizeof(ssid_er));
  size_t bytes_pass_read = EEPROM.readString(51, pass_er, sizeof(pass_er));
  Serial.println("Doc EEPROM:");
  Serial.println(ssid_er);
  Serial.println(pass_er);
  delay(100);
  
  /*-----------------------------------DISPLAY-----------------------------------*/
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin(); //Khởi tạo led matrix dma
  dma_display->setBrightness8(90); //0-255
  dma_display->clearScreen(); 
  //dma_display->fillScreen(dma_display->color444(0, 1, 0));  
  /*-----------------------------------Connect Wifi-----------------------------------*/\
  WiFi.begin(ssid_er, pass_er);
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
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);
    WiFiManager wifiManager;
    wifiManager.startConfigPortal("LedMatrix", "1234567890");
    wifiManager.setAPCallback(configModeCallback);
  }
  if(WiFi.status() == WL_CONNECTED){
    String ssid = WiFi.SSID();
    String password = WiFi.psk();
    Serial.println("Đã kết nối");
    EEPROM.writeString(0, ssid);
    EEPROM.writeString(51, password);
    EEPROM.commit();
  }
  else{
    Serial.print("Chưa kết nối");
  }
  delay(1000);
  dma_display->clearScreen();
  for(int g = 0; g<255; g++)
    {
      drawXbm565(0,0,64,32, wifi_image1bit, dma_display->color565(0,g,0));
      delay(5);
    }
  delay(2000);
  dma_display->clearScreen();
}


void loop() {
  // unsigned long currentMillis = millis();
  // bitmap();
  static float timeOffset = 0;
  colorWaveEffect(timeOffset);
  timeOffset += 0.1; // Điều chỉnh tốc độ của sóng
  delay(50); // Thời gian delay để làm chậm tốc độ chuyển động
  
}


void bitmap(){
  unsigned long Bitmap_currentMillis = millis();

  drawXbm565(0,0, 32, 32, icon_bits[current_icon]);
    if (Bitmap_currentMillis - Bitmap_previousMillis >= Bitmap_interval) {
        Bitmap_previousMillis = Bitmap_currentMillis;

        Serial.print("Showing icon ");
        Serial.println(icon_name[current_icon]);
        current_icon = (current_icon  +1 ) % num_icons;
        dma_display->clearScreen();
    }
  
}



