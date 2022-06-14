//Bismillahir Rahomanir Rahim


/*===== Including Library =====*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


/*===== Wifi Setting =====*/

const char* ssid = "SSID";
const char* password = "Password";
WiFiClient client;

/*===== Server Setting =====*/

String serverName = "192.168.86.5";//You can use domain name too
String serverPath = "/Home/UploadFile";//I've created a simple upload system by ASP.NET and this string is the controller and action name.
const int serverPort = 8787;//I've host my upload application in this port.


/*===== Camera Parameter Setting =====*/

//Camera Model AI_Thinker


#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22




void setup()
{
//Disable Brownout Detector

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);

  //Wifi Setup

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("IP : ");
  Serial.println(WiFi.localIP());


  //Camera Setting

  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

//Pre Allocate large buffer

  if(psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 3; // 0 - 63, Lower is better.
    config.fb_count = 1;//frame buffer
  }
  else
  {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 10; // 0 - 63, Lower is better.
    config.fb_count = 1;//frame buffer   
  }

//camera init

  esp_err_t err = esp_camera_init(&config);

  if(err != ESP_OK)
  {
    Serial.printf("Error : 0x%x",err);
    delay(1000);
    ESP.restart();
  }

  //captureAndSendPhoto();

}




void loop()
{
  unsigned long currentMillis = millis();
  unsigned long previousMillis = 0;
  int timerInterval = 10000;
  

  if((currentMillis - previousMillis) >= timerInterval )
  {  
    captureAndSendPhoto();
    previousMillis = currentMillis;
  }

  
  
}




void captureAndSendPhoto()
{
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();

  if(!fb)
  {
    Serial.println("Camera failed to capture image");
    delay(1000);
    ESP.restart();    
  }

//Connecting to server

  Serial.println("Connecting to server : " + serverName);

  if(client.connect(serverName.c_str(), serverPort))
  {
    String head = "--AE\r\nContent-Disposition: form-data; name=\"file\"; filename=\"ImageCaptured.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--AE--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: "+ serverName);
    client.println("Content-Length: "+ String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=AE");
    client.println();
    client.print(head);

    uint8_t* fbBuf = fb->buf;
    size_t fbLen = fb->len;

    for(size_t n = 0; n< fbLen; n=n+1024)
    {
      if(n+1024 < fbLen)
      {
        client.write(fbBuf,1024);
        fbBuf += 1024;
      }  
      else if(fbLen%1024 > 0)
      {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }
  

  client.print(tail);

  esp_camera_fb_return(fb);



  int timeOutTimer = 10000;
  long startTimer = millis();
  boolean state = false;
  int restart_attemp = 0;

  while((startTimer + timeOutTimer) > millis())
  {
    Serial.print(".");
    restart_attemp++;

    if(restart_attemp >= 10)
    {
      ESP.restart();
    }

    delay(100);
    
    while(client.available())
    {
      char c = client.read();

      if(c=='\n')
      {
        if(getAll.length() == 0)
        {
          state = true;
        }
        getAll= "";
      }
      else if(c!= '\r')
      {
        getAll += String(c);        
      }

      if(state == true)
      {
        getBody += String(c); 
      }
      startTimer = millis();      
    }

    if(getBody.length() > 0)
    {
      break;
    }
  }
  
  client.stop();
  Serial.println(getBody);
    
  }
  else
  {
    getBody = "Failed to connect";
    Serial.println(getBody);
  }


  delay(1000);

  
}
