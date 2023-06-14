#include "esp_camera.h"
#include <WiFi.h>

#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include "camera_pins.h"

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

//extras
#include <Wire.h>
#include "Waveshare_LCD1602.h"
#include "HX711.h"
#include "Arduino.h"

const int LOADCELL_DOUT_PIN = 13;
const int LOADCELL_SCK_PIN = 12;
String result = "arduino";
 

Waveshare_LCD1602 lcd(16,2); // Un lcd con 2 lineas de 16 caracteres 
HX711 balanza;

//here
float peso;
float pesoPasado;
int state = 0;
int stateP = 0;
//till here

#define QR Serial1

TaskHandle_t Task1;



// ===========================
// Enter your WiFi credentials
// ===========================
/*const char* ssid = "INFINITUM0FC0_2.4";
const char* password = "gxRed527hx";*/

#define WIFI_SSID "INFINITUM0FC0_2.4"
#define WIFI_PASSWORD "gxRed527hx"
// BROKER
#define MQTT_HOST IPAddress(192, 168, 1, 137) // MQTT BROKER IP ADDRESS
/*for example:
#define MQTT_HOST IPAddress(192, 168, 1, 106)*/
#define MQTT_PORT 1883
#define BROKER_USER "hacky"
#define BROKER_PASS "mosquitto"
//MQTT Topics
#define MQTT_PUB_PESO "esp/peso"
#define MQTT_PUB_CODIGO "esp/codigo"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
unsigned long previousMillis = 0; // Stores last time a message was published
const long interval = 2000; // Interval at which to publish values

void startCameraServer();
void setupLedFlash(int pin);

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}
void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}
void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print(" packetId: ");
  Serial.println(packetId);
  Serial.print(" qos: ");
  Serial.println(qos);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print(" packetId: ");
  Serial.println(packetId);
}
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties
properties, size_t len, size_t index, size_t total) {
  // Save the message in a variable
  String receivedMessage;
  for (int i = 0; i < len; i++) {
    Serial.println((char)payload[i]);
    receivedMessage += (char)payload[i];
  }
  // Turn the LED on or off accordingly to the message content
  lcd.setCursor(0,0);
  lcd.send_string(String(receivedMessage).c_str());
  delay(3000);
  lcd.clear();
}
void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print(" packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  QR.begin(9600, SERIAL_8N1, 1, 3);
  xTaskCreatePinnedToCore(loop2_escucha_escaner_qr, "Task1", 10000, NULL, 3, &Task1, 0);


  //hola prueba
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE,
  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE,
  (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(BROKER_USER, BROKER_PASS);
  connectToWifi();

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();
  
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  
  lcd.init();
  Serial.println("Iniciando la balanza");
  balanza.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  balanza.set_scale(-105.71816); // Configuración de la balanza 
  balanza.tare(); // reseteamos la balanza a 0   
  mqttClient.subscribe("cajero/venta",2);
}

void loop() {
  
    // put your main code here, to run repeatedly:
  peso = balanza.get_units(10);
  
  if(peso > 50){
    state = 1;
  }else{
    state = 0;
  }

  if ((state ==1 && stateP == 0)) {
    delay(1000);
    peso = balanza.get_units(10);

    // Publish an MQTT message on topic esp/BME2800/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_PESO, 2, true,
    String(peso).c_str());
    Serial.printf("Publishing on topic %s at QoS 2, packetId: %i",
    MQTT_PUB_PESO, packetIdPub1);
    Serial.printf("Message: %.2f \n", peso);

  }

  lcd.setCursor(0,0);
  lcd.send_string(String(peso).c_str());

  balanza.power_down();
  delay(1000);
  balanza.power_up();
  stateP = state;
  
}

