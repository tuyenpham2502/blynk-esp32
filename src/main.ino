/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-async-web-server-espasyncwebserver-library/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
#define BLYNK_PRINT Serial
// You should get Auth Token in the Blynk App.
#define BLYNK_TEMPLATE_ID "TMPL6MpuZOMPs"
#define BLYNK_TEMPLATE_NAME "Trạm khí tượng"
char BLYNK_AUTH_TOKEN[33]="QLXXh7yTpJvKPF_t3LgL6iYXDbZ_M4cd";
// Import required libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SimpleKalmanFilter.h>
#include "index_html.h"
#include "data_config.h"
#include <EEPROM.h>
#include <Arduino_JSON.h>
#include "icon.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//----------------------- Khai báo 1 số biến Blynk -----------------------
bool blynkConnect = true;
BlynkTimer timer; 
// Một số Macro
#define ENABLE    1
#define DISABLE   0
// ---------------------- Khai báo cho OLED 1.3 --------------------------
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3C //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define OLED_SDA      21
#define OLED_SCL      22

typedef enum {
  SCREEN0,
  SCREEN1,
  SCREEN2,
  SCREEN3,
  SCREEN4,
  SCREEN5,
  SCREEN6,
  SCREEN7,
  SCREEN8,
  SCREEN9,
  SCREEN10,
  SCREEN11,
  SCREEN12,
  SCREEN13
}SCREEN;
int screenOLED = SCREEN0;

bool enableShow = DISABLE;

#define SAD    0
#define NORMAL 1
#define HAPPY  2
int warningTempState = SAD;
int warningHumiState = NORMAL;
int warningDustState = HAPPY;


bool autoWarning = DISABLE;
// --------------------- Cảm biến DHT11 ---------------------
#include "DHT.h"
#define DHT11_PIN         26
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHTTYPE);
float tempValue = 30;
int humiValue   = 60;
SimpleKalmanFilter tempfilter(2, 2, 0.001);
SimpleKalmanFilter humifilter(2, 2, 0.001);
bool dht11ReadOK = true;
// -------------------- Khai báo cảm biến bụi --------------
#include <GP2Y1010AU0F.h>
#define DUST_TRIG             23
#define DUST_ANALOG           36
GP2Y1010AU0F dustSensor(DUST_TRIG, DUST_ANALOG);
SimpleKalmanFilter dustfilter(2, 2, 0.001);
int dustValue = 10;
// Khai bao LED
#define LED           33
// Khai báo BUZZER
#define BUZZER        2
uint32_t timeCountBuzzerWarning = 0;
#define TIME_BUZZER_WARNING     300  //thời gian cảnh báo bằng còi (đơn vị giây)
//-------------------- Khai báo Button-----------------------
#include "mybutton.h"
#define BUTTON_DOWN_PIN   34
#define BUTTON_UP_PIN     35
#define BUTTON_SET_PIN    32

#define BUTTON1_ID  1
#define BUTTON2_ID  2
#define BUTTON3_ID  3
Button buttonSET;
Button buttonDOWN;
Button buttonUP;
void button_press_short_callback(uint8_t button_id);
void button_press_long_callback(uint8_t button_id);
//------------------------------------------------------------
TaskHandle_t TaskButton_handle      = NULL;
TaskHandle_t TaskOLEDDisplay_handle = NULL;
TaskHandle_t TaskDHT11_handle = NULL;
TaskHandle_t TaskDustSensor_handle = NULL;
TaskHandle_t TaskAutoWarning_handle = NULL;
void setup(){
  Serial.begin(115200);
  // Đọc data setup từ eeprom
  EEPROM.begin(512);
  readEEPROM();
    // Khởi tạo LED
  pinMode(LED, OUTPUT);
  // Khởi tạo BUZZER
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, DISABLE);
  // Khởi tạo OLED
  oled.begin(i2c_Address, true);
  oled.setTextSize(2);
  oled.setTextColor(SH110X_WHITE);
  // Khởi tạo DHT11
  dht.begin();
  // Khai báo cảm biến bụi
  dustSensor.begin();
    // ---------- Đọc giá trị AutoWarning trong EEPROM ----------------
  autoWarning = EEPROM.read(210);

  // Khởi tạo nút nhấn
  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  button_init(&buttonSET, BUTTON_SET_PIN, BUTTON1_ID);
  button_init(&buttonUP, BUTTON_UP_PIN, BUTTON2_ID);
  button_init(&buttonDOWN,   BUTTON_DOWN_PIN,   BUTTON3_ID);
  button_pressshort_set_callback((void *)button_press_short_callback);
  button_presslong_set_callback((void *)button_press_long_callback);

  xTaskCreatePinnedToCore(TaskButton,          "TaskButton" ,          1024*10 ,  NULL,  20 ,  &TaskButton_handle       , 1);
  xTaskCreatePinnedToCore(TaskOLEDDisplay,     "TaskOLEDDisplay" ,     1024*16 ,  NULL,  20 ,  &TaskOLEDDisplay_handle  , 1);
  xTaskCreatePinnedToCore(TaskDHT11,           "TaskDHT11" ,           1024*10 ,  NULL,  10 ,  &TaskDHT11_handle  , 1);
  xTaskCreatePinnedToCore(TaskDustSensor,      "TaskDustSensor" ,      1024*10 ,  NULL,  10 ,  &TaskDustSensor_handle  , 1);
  xTaskCreatePinnedToCore(TaskAutoWarning,     "TaskAutoWarning" ,     1024*10 ,  NULL,  10  , &TaskAutoWarning_handle ,  1);

  // Kết nối wifi
  connectSTA();

  

}

void loop() {
  vTaskDelete(NULL);
}

//--------------------Task đo DHT11 ---------------
void TaskDHT11(void *pvParameters) { 
    //delay(10000);
    while(1) {
      int humi =  dht.readHumidity();
      float temp =  dht.readTemperature();
      if (isnan(humi) || isnan(temp) ) {
          Serial.println(F("Failed to read from DHT sensor!"));
          dht11ReadOK = false;
      }
      else if(humi <= 100 && temp < 100) {
          dht11ReadOK = true;
          // humiValue = humifilter.updateEstimate(humi);
          // tempValue = tempfilter.updateEstimate(temp);
          humiValue = humi;
          tempValue = temp;

          Serial.print(F("Humidity: "));
          Serial.print(humiValue);
          Serial.print(F("%  Temperature: "));
          Serial.print(tempValue);
          Serial.print(F("°C "));
          Serial.println();

          if(tempValue < EtempThreshold1 || tempValue > EtempThreshold2) 
            warningTempState = NORMAL;
          else
            warningTempState = HAPPY;
          if(humiValue < EhumiThreshold1 || humiValue > EhumiThreshold2) 
            warningHumiState = NORMAL;
          else
            warningHumiState = HAPPY;
      }
      delay(3000);
    }
}

//---------------- Task đo cảm biến bụi ----------
void TaskDustSensor(void *pvParameters) {
    while(1) {
      dustValue = dustSensor.read();
      dustValue = dustValue - 20;
      if(dustValue <= 0)  dustValue = 0;
      //dustValue = dustfilter.updateEstimate(dustValue);
      Serial.print("Dust Density = ");
      Serial.print(dustValue);
      Serial.println(" ug/m3");
      
      if(dustValue <= EdustThreshold1) 
        warningDustState = HAPPY;
      else if(dustValue > EdustThreshold1 && dustValue < EdustThreshold2) 
        warningDustState = NORMAL;
      else 
        warningDustState = SAD;
      delay(3000);
    }
}

// Xóa 1 ô hình chữ nhật từ tọa độ (x1,y1) đến (x2,y2)
void clearRectangle(int x1, int y1, int x2, int y2) {
   for(int i = y1; i < y2; i++) {
     oled.drawLine(x1, i, x2, i, 0);
   }
}

void clearOLED(){
  oled.clearDisplay();
  oled.display();
}

int countSCREEN9 = 0;
// Task hiển thị OLED
void TaskOLEDDisplay(void *pvParameters) {
  while (1) {
      switch(screenOLED) {
        case SCREEN0: // Hiệu ứng khởi động
          for(int j = 0; j < 3; j++) {
            for(int i = 0; i < FRAME_COUNT_loadingOLED; i++) {
              oled.clearDisplay();
              oled.drawBitmap(32, 0, loadingOLED[i], FRAME_WIDTH_64, FRAME_HEIGHT_64, 1);
              oled.display();
              delay(FRAME_DELAY/4);
            }
          }
          screenOLED = SCREEN4;
          break;
        case SCREEN1:   // Hiển thị nhiệt độ 
          for(int j = 0; j < 2 && enableShow == ENABLE; j++) {

            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(0, 20);
            oled.print("Nhiet do : ");
            oled.setTextSize(2);
            oled.setCursor(0, 32);
            //if(dht11ReadOK == true)
              oled.print(tempValue,1); 
            //else
            //  oled.print("NaN"); 
            oled.drawCircle(54, 32, 3,SH110X_WHITE); 
            oled.print(" C"); 
          
            for(int i = 0; i < FRAME_COUNT_face1OLED && enableShow == ENABLE; i++) {
                  clearRectangle(96, 0, 128, 64);
                  if(warningTempState == SAD)
                    oled.drawBitmap(96, 20, face1OLED[i], 32, 32, 1);
                  else if(warningTempState == NORMAL)
                    oled.drawBitmap(96, 20, face2OLED[i], 32, 32, 1);
                  else if(warningTempState == HAPPY)
                    oled.drawBitmap(96, 20, face3OLED[i], 32, 32, 1);
                  oled.display();
                  delay(FRAME_DELAY);
            }
            oled.display();
            delay(100);
          }
          if( enableShow == ENABLE)
            screenOLED = SCREEN2;
          break;
        case SCREEN2:   // Hiển thị độ ẩm
          for(int j = 0; j < 2 && enableShow == ENABLE; j++) {
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(0, 20);
            oled.print("Do am khong khi: ");
            oled.setTextSize(2);
            oled.setCursor(0, 32);
            //if(dht11ReadOK == true)
              oled.print(humiValue); 
            //else
            //  oled.print("NaN");
            oled.print(" %"); 
 
            for(int i = 0; i < FRAME_COUNT_face1OLED && enableShow == ENABLE; i++) {
                  clearRectangle(96, 0, 128, 64);
                  if(warningHumiState == SAD)
                    oled.drawBitmap(96, 20, face1OLED[i], 32, 32, 1);
                  else if(warningHumiState == NORMAL)
                    oled.drawBitmap(96, 20, face2OLED[i], 32, 32, 1);
                  else if(warningHumiState == HAPPY)
                    oled.drawBitmap(96, 20, face3OLED[i], 32, 32, 1);
                  oled.display();
                  delay(FRAME_DELAY);
            }
            oled.display();
            delay(100);
          }
          if( enableShow == ENABLE)
            screenOLED = SCREEN3;
          break;
        case SCREEN3:  // Hiển thị Bụi
          for(int j = 0; j < 2 && enableShow == ENABLE; j++) {
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(0, 20);
            oled.print("Bui min PM2.5: ");
            oled.setTextSize(2);
            oled.setCursor(0, 32);
            oled.print(dustValue); 
            oled.setTextSize(1);
            oled.print(" ug/m3");  

            for(int i = 0; i < FRAME_COUNT_face1OLED && enableShow == ENABLE; i++) {
              clearRectangle(96, 0, 128, 64);
              if(warningDustState == SAD)
                oled.drawBitmap(96, 20, face1OLED[i], 32, 32, 1);
              else if(warningDustState == NORMAL)
                oled.drawBitmap(96, 20, face2OLED[i], 32, 32, 1);
              else if(warningDustState == HAPPY)
                oled.drawBitmap(96, 20, face3OLED[i], 32, 32, 1);
              oled.display();
              delay(FRAME_DELAY);  
            }
            oled.display();
            delay(100);
          }
          if( enableShow == ENABLE)
            screenOLED = SCREEN1;
          break; 
        case SCREEN4:    // Đang kết nối Wifi
          oled.clearDisplay();
          oled.setTextSize(1);
          oled.setCursor(40, 5);
          oled.print("WIFI");
          oled.setTextSize(1.5);
          oled.setCursor(40, 17);
          oled.print("Dang ket noi..");
      
          for(int i = 0; i < FRAME_COUNT_wifiOLED; i++) {
            clearRectangle(0, 0, 32, 32);
            oled.drawBitmap(0, 0, wifiOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.display();
            delay(FRAME_DELAY);
          }
          break;
        case SCREEN5:    // Kết nối wifi thất bại
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Mat ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.drawLine(31, 0 , 0, 31 , 1);
            oled.drawLine(32, 0 , 0, 32 , 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN9;
          break;
        case SCREEN6:   // Đã kết nối Wifi, đang kết nối Blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);

            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Dang ket noi..");
                        

            for(int i = 0; i < FRAME_COUNT_blynkOLED; i++) {
              clearRectangle(0, 32, 32, 64);
              oled.drawBitmap(0, 32, blynkOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }

          break;
        case SCREEN7:   // Đã kết nối Wifi, Đã kết nối Blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);

            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 32, blynkOLED[FRAME_COUNT_wifiOLED/2], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN3;
            enableShow = ENABLE;
          break;
        case SCREEN8:   // Đã kết nối Wifi, Mat kết nối Blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);

            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Mat ket noi.");
            oled.drawBitmap(0, 32, blynkOLED[FRAME_COUNT_wifiOLED/2], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.drawLine(31, 32 , 0, 63 , 1);
            oled.drawLine(32, 32 , 0, 64 , 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN9;
          break;
        case SCREEN9:   // Cai đặt 192.168.4.1
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.setTextSize(1);
            oled.print("Ket noi Wifi:");
            oled.setCursor(40, 17);
            oled.setTextSize(1);
            oled.print("ESP32_IOT");

            oled.setCursor(40, 38);
            oled.print("Dia chi IP:");
    
            oled.setCursor(40, 50);
            oled.print("192.168.4.1");

            for(int i = 0; i < FRAME_COUNT_settingOLED; i++) {
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, settingOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY*2);
            }
            countSCREEN9++;
            if(countSCREEN9 > 10) {
              countSCREEN9 = 0;
              screenOLED = SCREEN1;
              enableShow = ENABLE;
            }

            break;
          case SCREEN10:    // auto : on
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print("Canh bao:");
            oled.setTextSize(2);
            oled.setCursor(40, 32);
            oled.print("DISABLE"); 
            for(int i = 0; i < FRAME_COUNT_autoOnOLED; i++) {
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, autoOnOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            clearRectangle(40, 32, 128, 64);
            oled.setCursor(40, 32);
            oled.print("ENABLE"); 
            oled.display();   
            delay(2000);
            screenOLED = SCREEN1;
            enableShow = ENABLE;
            break;
          case SCREEN11:     // auto : off
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print("Canh bao:");
            oled.setTextSize(2);
            oled.setCursor(40, 32);
            oled.print("ENABLE");
            for(int i = 0; i < FRAME_COUNT_autoOffOLED; i++) {
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, autoOffOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            clearRectangle(40, 32, 128, 64);
            oled.setCursor(40, 32);
            oled.print("DISABLE"); 
            oled.display();    
            delay(2000);
            screenOLED = SCREEN1;  
            enableShow = ENABLE;
            break;
          case SCREEN12:  // gui du lieu len blynk
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print("Gui du lieu");
            oled.setCursor(40, 32);
            oled.print("den BLYNK"); 
            for(int i = 0; i < FRAME_COUNT_sendDataOLED; i++) {
                clearRectangle(0, 0, 32, 64);
                oled.drawBitmap(0, 16, sendDataOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
                oled.display();
                delay(FRAME_DELAY);
            } 
            delay(1000);
            screenOLED = SCREEN1; 
            enableShow = ENABLE;
            break;
          case SCREEN13:   // khoi dong lai
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(0, 20);
            oled.print("Khoi dong lai");
            oled.setCursor(0, 32);
            oled.print("Vui long doi ..."); 
            oled.display();
            break;
          default : 
            delay(500);
            break;
      } 
      delay(10);
  }
}



//-----------------Kết nối STA wifi, chuyển sang wifi AP nếu kết nối thất bại ----------------------- 
void connectSTA() {
      delay(5000);
      enableShow = DISABLE;
      if ( Essid.length() > 1 ) {  
      Serial.println(Essid);        //Print SSID
      Serial.println(Epass);        //Print Password
      Serial.println(Etoken);        //Print token
      Etoken = Etoken.c_str();
      WiFi.begin(Essid.c_str(), Epass.c_str());   //c_str()
      int countConnect = 0;
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);   
          if(countConnect++  == 15) {
            Serial.println("Ket noi Wifi that bai");
            Serial.println("Kiem tra SSID & PASS");
            Serial.println("Ket noi Wifi: ESP32 de cau hinh");
            Serial.println("IP: 192.168.4.1");
            screenOLED = SCREEN5;
            digitalWrite(BUZZER, ENABLE);
            delay(2000);
            digitalWrite(BUZZER, DISABLE);
            delay(3000);
            break;
          }
          // MODE đang kết nối wifi
          screenOLED = SCREEN4;
          delay(2000);
      }
      Serial.println("");
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("Da ket noi Wifi: ");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP()); 
        Serial.println((char*)Essid.c_str());

       // MODE wifi đã kết nối, đang kết nối blynk
       screenOLED = SCREEN6;
       delay(2000);
        strcpy(BLYNK_AUTH_TOKEN,Etoken.c_str());
        
        Blynk.config(BLYNK_AUTH_TOKEN);
        blynkConnect = Blynk.connect();
        if(blynkConnect == false) {
            screenOLED = SCREEN8;
            delay(2000);
            connectAPMode(); 
        }
        else {
            Serial.println("Da ket noi BLYNK");
            enableShow = ENABLE;
            // MODE đã kết nối wifi, đã kết nối blynk
            screenOLED = SCREEN7;
            delay(2000);
            xTaskCreatePinnedToCore(TaskBlynk,            "TaskBlynk" ,           1024*16 ,  NULL,  20  ,  NULL ,  1);
            timer.setInterval(1000L, myTimer);  
            buzzerBeep(5);  
            return; 
        }
      }
      else {
        digitalWrite(BUZZER, ENABLE);
        delay(2000);
        digitalWrite(BUZZER, DISABLE);
        // MODE truy cập vào 192.168.4.1
        screenOLED = SCREEN9;
        connectAPMode(); 
      }
        
    }
}


//--------------------------- switch AP Mode --------------------------- 
void connectAPMode() {

  // Khởi tạo Wifi AP Mode, vui lòng kết nối wifi ESP32, truy cập 192.168.4.1
  WiFi.softAP(ssidAP, passwordAP);  

  // Gửi trang HTML khi client truy cập 192.168.4.1
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Gửi data ban đầu đến clientgetDataFromClient
  server.on("/data_before", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getJsonData();
    request->send(200, "application/json", json);
  });

  // Get data từ client
  server.on("/post_data", HTTP_POST, [](AsyncWebServerRequest *request){
    String response = "SUCCESS";
    if (request->hasParam("data", true)) {
      enableShow = DISABLE;
      screenOLED = SCREEN13;
      delay(2000); // Shorter delay to avoid blocking
      
      // Ensure EEPROM writes are complete
      EEPROM.commit();
      
      // Schedule restart after response is sent
      request->onDisconnect([]() {
        delay(100);
        ESP.restart();
      });
    } else {
      response = "ERROR: No data received";
    }
    request->send(200, "text/plain", response);
  }, NULL, getDataFromClient);

  // Start server
  server.begin();
}

//------------------- Hàm đọc data từ client gửi từ HTTP_POST "/post_data" -------------------
void getDataFromClient(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.print("get data : ");
  Serial.println((char *)data);
  JSONVar myObject = JSON.parse((char *)data);
  if(myObject.hasOwnProperty("ssid"))
    Essid = (const char*) myObject["ssid"];
  if(myObject.hasOwnProperty("pass"))
    Epass = (const char*)myObject["pass"] ;
  if(myObject.hasOwnProperty("token"))
    Etoken = (const char*)myObject["token"];
  if(myObject.hasOwnProperty("tempThreshold1"))
    EtempThreshold1 = (int) myObject["tempThreshold1"];
  if(myObject.hasOwnProperty("tempThreshold2")) 
    EtempThreshold2 = (int) myObject["tempThreshold2"];
  if(myObject.hasOwnProperty("humiThreshold1")) 
    EhumiThreshold1 = (int) myObject["humiThreshold1"];
  if(myObject.hasOwnProperty("humiThreshold2")) 
    EhumiThreshold2 = (int) myObject["humiThreshold2"];
  if(myObject.hasOwnProperty("dustThreshold1")) 
    EdustThreshold1 = (int) myObject["dustThreshold1"];
  if(myObject.hasOwnProperty("dustThreshold2")) 
    EdustThreshold2 = (int) myObject["dustThreshold2"];
  writeEEPROM();
  

}

// ------------ Hàm in các giá trị cài đặt ------------
void printValueSetup() {
    Serial.print("ssid = ");
    Serial.println(Essid);
    Serial.print("pass = ");
    Serial.println(Epass);
    Serial.print("token = ");
    Serial.println(Etoken);
    Serial.print("tempThreshold1 = ");
    Serial.println(EtempThreshold1);
    Serial.print("tempThreshold2 = ");
    Serial.println(EtempThreshold2);
    Serial.print("humiThreshold1 = ");
    Serial.println(EhumiThreshold1);
    Serial.print("humiThreshold2 = ");
    Serial.println(EhumiThreshold2);
    Serial.print("dustThreshold1 = ");
    Serial.println(EdustThreshold1);
    Serial.print("dustThreshold2 = ");
    Serial.println(EdustThreshold2);
    Serial.print("autoWarning = ");
    Serial.println(autoWarning);
}

//-------- Hàm tạo biến JSON để gửi đi khi có request HTTP_GET "/" --------
String getJsonData() {
  JSONVar myObject;
  myObject["ssid"]  = Essid;
  myObject["pass"]  = Epass;
  myObject["token"] = Etoken;
  myObject["tempThreshold1"] = EtempThreshold1;
  myObject["tempThreshold2"] = EtempThreshold2;
  myObject["humiThreshold1"] = EhumiThreshold1;
  myObject["humiThreshold2"] = EhumiThreshold2;
  myObject["dustThreshold1"] = EdustThreshold1;
  myObject["dustThreshold2"] = EdustThreshold2;

  String jsonData = JSON.stringify(myObject);
  return jsonData;
}

//-------------------------------------------------------------------------------
//--------------------------------Task Blynk-------------------------------------

//----------------------------- Task auto Warning--------------------------------
void TaskAutoWarning(void *pvParameters)  {
    delay(20000);
    while(1) {
      if(autoWarning == 1) {
          check_air_quality_and_send_to_blynk(ENABLE, tempValue, humiValue, dustValue);
      }
      delay(10000);
    }
}

//----------------------- Send send Data value to Blynk every 2 seconds--------
void myTimer() {
    Blynk.virtualWrite(V0, tempValue);  
    Blynk.virtualWrite(V1, humiValue);
    Blynk.virtualWrite(V2, dustValue);
    Blynk.virtualWrite(V4, autoWarning); 
}
//--------------Read button from BLYNK and send notification back to Blynk-----------------------
int checkAirQuality = 0;
BLYNK_WRITE(V3) {
    enableShow = DISABLE;
    checkAirQuality = param.asInt();
    if(checkAirQuality == 1) {
      buzzerBeep(1);
      check_air_quality_and_send_to_blynk(DISABLE, tempValue, humiValue, dustValue);
      screenOLED = SCREEN12;
    } 
}

//------------------------- check autoWarning from BLYNK  -----------------------
BLYNK_WRITE(V4) {
    enableShow = DISABLE;
    autoWarning = param.asInt();
    buzzerBeep(1);
    EEPROM.write(210, autoWarning);  EEPROM.commit();
    if(autoWarning == 0) screenOLED = SCREEN11;
    else screenOLED = SCREEN10;
}

//---------------------------Task TaskSwitchAPtoSTA---------------------------
void TaskBlynk(void *pvParameters) {
    while(1) {
      Blynk.run();
      timer.run(); 
      delay(10);
    }
}

/*
 * Các hàm liên quan đến lưu dữ liệu cài đặt vào EEPROM
*/
//--------------------------- Read Eeprom  --------------------------------
void readEEPROM() {
    for (int i = 0; i < 32; ++i)       //Reading SSID
        Essid += char(EEPROM.read(i)); 
    for (int i = 32; i < 64; ++i)      //Reading Password
        Epass += char(EEPROM.read(i)); 
    for (int i = 64; i < 96; ++i)      //Reading Password
        Etoken += char(EEPROM.read(i)); 
    if(Essid.length() == 0) Essid = "BLK";

    EtempThreshold1 = EEPROM.read(200);
    EtempThreshold2 = EEPROM.read(201);

    EhumiThreshold1 = EEPROM.read(202);
    EhumiThreshold2 = EEPROM.read(203);

    EdustThreshold1 = EEPROM.read(204) * 100 + EEPROM.read(205);
    EdustThreshold2 = EEPROM.read(206) * 100 + EEPROM.read(207);  

    autoWarning     = EEPROM.read(210);

    printValueSetup();
}

// ------------------------ Clear Eeprom ------------------------

void clearEeprom() {
    Serial.println("Clearing Eeprom");
    for (int i = 0; i < 250; ++i) 
      EEPROM.write(i, 0);
}

// -------------------- Hàm ghi data vào EEPROM ------------------
void writeEEPROM() {
    clearEeprom();
    for (int i = 0; i < Essid.length(); ++i)
          EEPROM.write(i, Essid[i]);  
    for (int i = 0; i < Epass.length(); ++i)
          EEPROM.write(32+i, Epass[i]);
    for (int i = 0; i < Etoken.length(); ++i)
          EEPROM.write(64+i, Etoken[i]);

    EEPROM.write(200, EtempThreshold1);          // lưu ngưỡng nhiệt độ 1
    EEPROM.write(201, EtempThreshold2);          // lưu ngưỡng nhiệt độ 2

    EEPROM.write(202, EhumiThreshold1);          // lưu ngưỡng độ ẩm 1
    EEPROM.write(203, EhumiThreshold2);          // lưu ngưỡng độ ẩm 2

    EEPROM.write(204, EdustThreshold1 / 100);      // lưu hàng nghìn + trăm bụi 1
    EEPROM.write(205, EdustThreshold1 % 100);      // lưu hàng chục + đơn vị bụi 1

    EEPROM.write(206, EdustThreshold2 / 100);      // lưu hàng nghìn + trăm bụi 2
    EEPROM.write(207, EdustThreshold2 % 100);      // lưu hàng chục + đơn vị bụi 2
    
    EEPROM.commit();

    Serial.println("write eeprom");
    delay(500);
}


//-----------------------Task Task Button ----------
void TaskButton(void *pvParameters) {
    while(1) {
      handle_button(&buttonSET);
      handle_button(&buttonUP);
      handle_button(&buttonDOWN);
      delay(10);
    }
}
//-----------------Hàm xử lí nút nhấn nhả ----------------------
void button_press_short_callback(uint8_t button_id) {
    switch(button_id) {
      case BUTTON1_ID :  
        buzzerBeep(1);
        Serial.println("btSET press short");
        break;
      case BUTTON2_ID :
        buzzerBeep(1);
        Serial.println("btUP press short");
        break;
      case BUTTON3_ID :
        buzzerBeep(1);
        Serial.println("btDOWN press short");
        enableShow = DISABLE;
        check_air_quality_and_send_to_blynk(DISABLE, tempValue, humiValue, dustValue);
        screenOLED = SCREEN12;
        break;  
    } 
} 
//-----------------Hàm xử lí nút nhấn giữ ----------------------
void button_press_long_callback(uint8_t button_id) {
  switch(button_id) {
    case BUTTON1_ID :
      buzzerBeep(2);  
      enableShow = DISABLE;
      Serial.println("btSET press long");
      screenOLED = SCREEN9;
      clearOLED();
      connectAPMode(); 
      break;
    case BUTTON2_ID :
      buzzerBeep(2);
      Serial.println("btUP press short");
      break;
    case BUTTON3_ID :
      buzzerBeep(2);
      Serial.println("btDOWN press short");
      enableShow = DISABLE;
      autoWarning = 1 - autoWarning;
      EEPROM.write(210, autoWarning);  EEPROM.commit();
      Blynk.virtualWrite(V4, autoWarning); 
      if(autoWarning == 0) screenOLED = SCREEN11;
      else screenOLED = SCREEN10;
      break;  
  } 
} 
// ---------------------- Hàm điều khiển còi -----------------------------
void buzzerBeep(int numberBeep) {
  for(int i = 0; i < numberBeep; ++i) {
    digitalWrite(BUZZER, ENABLE);
    delay(100);
    digitalWrite(BUZZER, DISABLE);
    delay(100);
  }  
}
// ---------------------- Hàm điều khiển LED -----------------------------
void blinkLED(int numberBlink) {
  for(int i = 0; i < numberBlink; ++i) {
    digitalWrite(LED, DISABLE);
    delay(300);
    digitalWrite(LED, ENABLE);
    delay(300);
  }  
}

/**
 * @brief Kiểm tra chất lượng không khí và gửi lên BLYNK
 *
 * @param autoWarning auto Warning
 * @param temp Nhiệt độ hiện tại    *C
 * @param humi Độ ẩm hiện tại        %
 * @param dust bụi PM2.5 hiện tại    ug/m3
 */
void check_air_quality_and_send_to_blynk(bool autoWarning, int temp, int humi, int dust) {
  String notifications = "";
  int tempIndex = 0;
  int dustIndex = 0;
  int humiIndex = 0;
  if(dht11ReadOK ==  true) {
  if(autoWarning == 0) {
    if(temp < EtempThreshold1 )tempIndex = 1;
    else if(temp >= EtempThreshold1 && temp <=  EtempThreshold2)  tempIndex = 2;
    else tempIndex = 3;
    

    if(humi < EhumiThreshold1 ) humiIndex = 1;
    else if(humi >= EhumiThreshold1 && humi <= EhumiThreshold2)   humiIndex = 2;
    else humiIndex = 3;

    if(dust < EdustThreshold1 ) dustIndex = 1;
    else if(dust >= EdustThreshold1 && EdustThreshold1 <= EtempThreshold2)   dustIndex = 2;
    else dustIndex = 3;
    
    notifications = snTemp[tempIndex] + String(temp) + "*C . " + snHumi[humiIndex] + String(humi) + "% . " + snDust[dustIndex] + String(dust) + "ug/m3 . " ;  
    
    Blynk.logEvent("check_data",notifications);
  } else {
    if(temp < EtempThreshold1 )tempIndex = 1;
    else if(temp >= EtempThreshold1 && temp <=  EtempThreshold2)  tempIndex = 0;
    else tempIndex = 3;
    

    if(humi < EhumiThreshold1 ) humiIndex = 1;
    else if(humi >= EhumiThreshold1 && humi <= EhumiThreshold2)   humiIndex = 0;
    else humiIndex = 3;

    if(dust < EdustThreshold1 ) dustIndex = 0;
    else if(dust >= EdustThreshold1 && EdustThreshold1 <= EdustThreshold2)   dustIndex = 2;
    else dustIndex = 3;

    if(tempIndex == 0 && humiIndex == 0 && dustIndex == 0)
      notifications = "";
    else {
      if(tempIndex != 0) notifications = notifications + snTemp[tempIndex] + String(temp) + "*C . ";
      if(humiIndex != 0) notifications = notifications + snHumi[humiIndex] + String(humi) + "% . " ;
      if(dustIndex != 0) notifications = notifications + snDust[dustIndex] + String(dust) + "ug/m3 . " ;
      Blynk.logEvent("auto_warning",notifications);
    }
  }

  Serial.println(notifications);
  }
}