#ifdef ESP8266 || ESP32
  #define ISR_PREFIX ICACHE_RAM_ATTR
#else
  #define ISR_PREFIX
#endif

#if !(defined(ESP_NAME))
  #define ESP_NAME "buttons" 
#endif

#if !(defined(BUTTON_TIMEOUT))
  #define BUTTON_TIMEOUT 10000 
#endif

#include <Arduino.h>

/* WiFi */
#include <ESP8266WiFi.h> // WIFI support
#include <ArduinoOTA.h> // Updates over the air
char hostname[32] = {0};

/* mDNS */
#include <ESP8266mDNS.h> // For network discovery

/* WiFi Manager */
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

/* MQTT */
#include <PubSubClient.h>
char topic[40] = {0};
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

/* Button */
unsigned long buttonTimeout = 0;

/* LEDs */
unsigned long ledTimeout = 0;
int activeLed = 0;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println(F("Config Mode"));
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void reconnect() {
  Serial.print("MQTT Connecting");
  while (!mqtt.connected()) {
    if (mqtt.connect(hostname)) {
      Serial.println();
      Serial.println("MQTT connected");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

void setup()
{
  /* LED */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  /* Serial */
  Serial.begin(9600);
  Serial.println(F("Setup..."));

  /* WiFi */
  sprintf(hostname, "%s-%06X", ESP_NAME, ESP.getChipId());
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if(!wifiManager.autoConnect(hostname)) {
    Serial.println(F("WiFi Connect Failed"));
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  Serial.println(hostname);
  Serial.print(F("  "));
  Serial.print(WiFi.localIP());
  Serial.print(F("  "));
  Serial.println(WiFi.macAddress());

  /* OTA */
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) { 
      Serial.println(F("Auth Failed")); 
    }
    else if (error == OTA_BEGIN_ERROR) { 
      Serial.println(F("Begin Failed")); 
    }
    else if (error == OTA_CONNECT_ERROR) { 
      Serial.println(F("Connect Failed")); 
    }
    else if (error == OTA_RECEIVE_ERROR) { 
      Serial.println(F("Receive Failed"));
    } 
    else if (error == OTA_END_ERROR) { 
      Serial.println(F("End Failed")); 
    }
  });
  ArduinoOTA.begin();

  // Discover MQTT broker via mDNS
  Serial.print(F("Finding MQTT Server"));
  while (MDNS.queryService("mqtt", "tcp") == 0) {
    delay(1000);
    Serial.print(F("."));
    ArduinoOTA.handle();
  }
  Serial.println();

  Serial.println(F("MQTT: "));
  Serial.print(F("  "));
  Serial.println(MDNS.hostname(0));
  Serial.print(F("  "));
  Serial.print(MDNS.IP(0));
  Serial.print(F(":"));
  Serial.println(MDNS.port(0));

  mqtt.setServer(MDNS.IP(0), MDNS.port(0));

  // buttons
  pinMode(D1, INPUT_PULLUP); // 1
  pinMode(D2, INPUT_PULLUP); // 2
  pinMode(D3, INPUT_PULLUP); // 3
  pinMode(D4, INPUT_PULLUP); // 4

  // leds
  pinMode(D5, OUTPUT); // 1
  digitalWrite(D5, HIGH);
  
  pinMode(D6, OUTPUT); // 2
  digitalWrite(D6, HIGH);
  
  pinMode(D7, OUTPUT); // 3
  digitalWrite(D7, HIGH);
  
  pinMode(D8, OUTPUT); // 4
  digitalWrite(D8, HIGH);

  /* LED */
  digitalWrite(LED_BUILTIN, HIGH);
}

void TurnOffAllLedsExcept(int button) {
  digitalWrite(D5, (button != 1) ? LOW : HIGH); // 1
  digitalWrite(D6, (button != 2) ? LOW : HIGH); // 2
  digitalWrite(D7, (button != 3) ? LOW : HIGH); // 3
  digitalWrite(D8, (button != 4) ? LOW : HIGH); // 4
}

int getButtonPressed() {
  if (digitalRead(D1) == LOW) return 1;
  if (digitalRead(D2) == LOW) return 2;
  if (digitalRead(D3) == LOW) return 3;
  if (digitalRead(D4) == LOW) return 4;
  return 0;
}

void loop()
{
  /* OTA */
  ArduinoOTA.handle();

  /* MQTT */
  if (!mqtt.connected())
  {
    reconnect();
  }
  mqtt.loop();

  unsigned long currentMillis = millis();

  if (currentMillis > buttonTimeout)
  {
    int button = getButtonPressed();

    if (button > 0)
    {
      activeLed = button;
      TurnOffAllLedsExcept(activeLed);

      sprintf(topic, "%s/b%d", hostname, button);
      Serial.print(topic);
      mqtt.publish(topic, "1");

      buttonTimeout = currentMillis + BUTTON_TIMEOUT;
    }
    else
    {
      if (currentMillis > ledTimeout)
      {
        activeLed++;
        if (activeLed > 4)
        {
          activeLed = 1;
        }
        TurnOffAllLedsExcept(activeLed);

        ledTimeout = currentMillis + 1000;
      }
    }
  }
}

