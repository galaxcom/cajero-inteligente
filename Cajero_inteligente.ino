// Importar las bibliotecas
#include "esp_camera.h"
#include <WiFi.h>

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}

#include <AsyncMqttClient.h>

#include <Wire.h>
#include "Waveshare_LCD1602.h"
#include "HX711.h"
#include "Arduino.h"

// Definir el modelo de cámara
#define CAMERA_MODEL_AI_THINKER 
#include "camera_pins.h"

// Definir los pines de la celda de carga
const int LOADCELL_DOUT_PIN = 13;
const int LOADCELL_SCK_PIN = 12;
 
// LCD con 2 filas de 16 caracteres
Waveshare_LCD1602 lcd(16,2); 
// Definir una celda de carga
HX711 loadcell;

// Una variable para guardar el peso
float peso;

// Una variable para el estado de la celda de carga
// 1 = tiene algo encima
// 0 = no tiene nada encima
int estado = 0;
int estadoPasado = 0;


#define QR Serial1

TaskHandle_t Task1;

// Ingrese sus credenciales de WiFi
#define WIFI_SSID "**********"
#define WIFI_PASSWORD "**********"

// Configuración del broker
#define MQTT_HOST IPAddress(192, 168, 1, 137) // Dirección IP del broker
#define MQTT_PORT 1883 // Puerto del broker
#define BROKER_USER "hacky" // Usuario para el broker
#define BROKER_PASS "mosquitto" // Contraseña del broker 
//MQTT Temas
#define MQTT_PUB_PESO "esp/peso"
#define MQTT_PUB_CODIGO "esp/codigo"
#define MQTT_SUB_PRODUCTO "esp/produto"

// Configuraciones MQTT
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer; // Almacena la última vez que se publicó un mensaje
unsigned long previousMillis = 0; 
const long interval = 2000; // Intervalo en el que se publican valores

// Camera Server
void startCameraServer();
void setupLedFlash(int pin);

// Función para la conexión a wifi
void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// Función para la conexión a MQTT
void connectToMqtt() {
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      xTimerStop(mqttReconnectTimer, 0); // Asegurarse de que no nos volvamos a conectar a MQTT mientras nos volvemos a conectar a Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  // Escribe aquí lo que quieres hacer al conectarse a mqtt
}

// Esta función se ejecuta cuando te desconectas de MQTT
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  //Escribe aquí lo que quieres hacer cuando te suscribas a un tema
}

void onMqttUnsubscribe(uint16_t packetId) {
  //Escribe aquí lo que quieres hacer cuando te desuscribas de un tema
}

// Esta función se ejecuta cuando recibes un mensaje MQTT
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties
properties, size_t len, size_t index, size_t total) {

  // Guardar el mensaje en una variable
  String receivedMessage;
  for (int i = 0; i < len; i++) {
    receivedMessage += (char)payload[i];
  }

  // Muestra el mensaje en el LCD
  lcd.setCursor(0,0);
  lcd.send_string(String(receivedMessage).c_str());
  delay(300);
  lcd.clear();
}

// Esta función se ejecuta cuando publicas un mensaje MQTT
void onMqttPublish(uint16_t packetId) {
  
}


void setup() {
  QR.begin(9600, SERIAL_8N1, 1, 3);
  xTaskCreatePinnedToCore(loop2_escucha_escaner_qr, "Task1", 10000, NULL, 3, &Task1, 0);

  // Configuraciónes MQTT
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

  // Conexión a wifi
  connectToWifi();
  
  // Configuración de la cámara
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
  config.pixel_format = PIXFORMAT_JPEG; 
  //config.pixel_format = PIXFORMAT_RGB565; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  

  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_240X240;
    #if CONFIG_IDF_TARGET_ESP32S3
      config.fb_count = 2;
    #endif
  }

  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif

  // Inicio de cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); 
    s->set_brightness(s, 1); 
    s->set_saturation(s, -2); 
  }

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

  #if defined(LED_GPIO_NUM)
    setupLedFlash(LED_GPIO_NUM);
  #endif

  // Conexión a wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Iniciar el servidor de cámara
  startCameraServer();
  
  // Iniciar el LCD
  lcd.init();

  // Iniciar la celda de carga
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  // Calibración de la celda de carga
  loadcell.set_scale(-105.71816);
  // Resetear la balanza a 0 
  loadcell.tare();
  
  // MQTT suscribirse para obtener la predicción
  mqttClient.subscribe(MQTT_SUB_PRODUCTO,2);

}

void loop() {
  // Obtener el peso
  peso = loadcell.get_units(10);
  
  // Obtener el estado de la celda de carga
  if(peso > 50){
		estado = 1;
  }else{
	  estado = 0;
  }
  
  // Se ejecuta si le pones peso a la celda de carga
  if ((estado == 1 && estadoPasado == 0)) {   
    delay(100);
    // Obtener el peso
    peso = loadcell.get_units(10);

    // MQTT publica el peso
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_PESO, 2, false , String(peso).c_str());

  }
  
  // Muestra el peso en la pantalla LCD
  lcd.setCursor(0,0);
  lcd.send_string(String(peso).c_str());
  
  loadcell.power_down();
  delay(1000);
  loadcell.power_up();
  estadoPasado = estado;
  
}
