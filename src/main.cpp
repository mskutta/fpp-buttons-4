#ifdef ESP8266 || ESP32
  #define ISR_PREFIX ICACHE_RAM_ATTR
#else
  #define ISR_PREFIX
#endif

#if !(defined(ESP_NAME))
  #define ESP_NAME "fpp-buttons" 
#endif

#include <Arduino.h>

#include <ESP8266WiFi.h> // WIFI support
#include <ESP8266mDNS.h> // For network discovery
#include <WiFiUdp.h> // OSC over UDP
#include <ArduinoOTA.h> // Updates over the air

// WiFi Manager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

// MQTT
#include <PubSubClient.h>

/* WIFI */
char hostname[32] = {0};

/* MQTT */
WiFiClient wifiClient;
PubSubClient client(wifiClient);
const char* broker = "10.81.95.165";

/* Button */
unsigned long waitTimeout = 0;
int lastButton = 0;
int activeButton = 0;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println(F("Config Mode"));
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("MQTT Connecting...");
    if (client.connect(hostname)) {
      Serial.println("MQTT connected");
      client.subscribe("fpp/falcon/player/FPP/playlist/name/status"); // "fpp/#"
      client.subscribe("fpp/falcon/player/FPP/set/playlist/#");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic,"fpp/falcon/player/FPP/playlist/name/status") == 0)
  {
    if ((char)payload[0] == '1')
      activeButton = 1;
    else if ((char)payload[0] == '2')
      activeButton = 2;
    else if ((char)payload[0] == '3')
      activeButton = 3;
    else if ((char)payload[0] == '4')
      activeButton = 4;
    else if ((char)payload[0] == '5')
      activeButton = 5;
    else if ((char)payload[0] == '6')
      activeButton = 6;
    else if ((char)payload[0] == '7')
      activeButton = 7;
    else if ((char)payload[0] == '8')
      activeButton = 8;
    return;
  }

  activeButton = -1;
}

void setup()
{
  Serial.begin(9600);

  /* LED */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  delay(1000);

  /* WiFi */
  sprintf(hostname, "%s-%06X", ESP_NAME, ESP.getChipId());
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if(!wifiManager.autoConnect(hostname)) {
    Serial.println("WiFi Connect Failed");
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
    if (error == OTA_AUTH_ERROR) { Serial.println(F("Auth Failed")); }
    else if (error == OTA_BEGIN_ERROR) { Serial.println(F("Begin Failed")); }
    else if (error == OTA_CONNECT_ERROR) { Serial.println(F("Connect Failed")); }
    else if (error == OTA_RECEIVE_ERROR) { Serial.println(F("Receive Failed")); } 
    else if (error == OTA_END_ERROR) { Serial.println(F("End Failed")); }
  });
  ArduinoOTA.begin();

  // buttons
  pinMode(D1, INPUT_PULLUP); // 1 | 5
  pinMode(D2, INPUT_PULLUP); // 2 | 6
  pinMode(D3, INPUT_PULLUP); // 3 | 7
  pinMode(D4, INPUT_PULLUP); // 4 | 8

  // leds
  pinMode(D5, OUTPUT); // 1 | 5
  digitalWrite(D5, HIGH);
  
  pinMode(D6, OUTPUT); // 2 | 6
  digitalWrite(D6, HIGH);
  
  pinMode(D7, OUTPUT); // 3 | 7
  digitalWrite(D7, HIGH);
  
  pinMode(D8, OUTPUT); // 4 | 8
  digitalWrite(D8, HIGH);
  
  /* MQTT */
  client.setServer(broker, 1883);
  client.setCallback(callback);

  /* LED */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void TurnOnAllLeds() {
  digitalWrite(D5, HIGH); // 1 | 5
  digitalWrite(D6, HIGH); // 2 | 6
  digitalWrite(D7, HIGH); // 3 | 7
  digitalWrite(D8, HIGH); // 4 | 8
}

void TurnOffAllLeds() {
  digitalWrite(D5, LOW); // 1 | 5
  digitalWrite(D6, LOW); // 2 | 6
  digitalWrite(D7, LOW); // 3 | 7
  digitalWrite(D8, LOW); // 4 | 8
}

int getButtonPressed() {
#if defined BUTTONS_1TO4
  if (digitalRead(D1) == LOW) return 1;
  if (digitalRead(D2) == LOW) return 2;
  if (digitalRead(D3) == LOW) return 3;
  if (digitalRead(D4) == LOW) return 4;
#elif defined BUTTONS_5TO8
  if (digitalRead(D1) == LOW) return 5;
  if (digitalRead(D2) == LOW) return 6;
  if (digitalRead(D3) == LOW) return 7;
  if (digitalRead(D4) == LOW) return 8;
#endif
  return 0;
}

void TurnOffAllLedsExcept(int button) {
#if defined BUTTONS_1TO4
  digitalWrite(D5, (button != 1) ? LOW : HIGH); // 1
  digitalWrite(D6, (button != 2) ? LOW : HIGH); // 2
  digitalWrite(D7, (button != 3) ? LOW : HIGH); // 3
  digitalWrite(D8, (button != 4) ? LOW : HIGH); // 4
#elif defined BUTTONS_5TO8
  digitalWrite(D5, (button != 5) ? LOW : HIGH); // 5
  digitalWrite(D6, (button != 6) ? LOW : HIGH); // 6
  digitalWrite(D7, (button != 7) ? LOW : HIGH); // 7
  digitalWrite(D8, (button != 8) ? LOW : HIGH); // 8
#endif
}

void PublishFpp(int button) {
  Serial.print("Publishing start default playlist ");
  Serial.println(button);
  if (button == 1)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/1/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/1/repeat", "1");
  }
  else if (button == 2)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/2/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/2/repeat", "1");
  }
  else if (button == 3)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/3/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/3/repeat", "1");
  }
  else if (button == 4)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/4/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/4/repeat", "1");
  }
  else if (button == 5)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/5/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/5/repeat", "1");
  }
  else if (button == 6)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/6/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/6/repeat", "1");
  }
  else if (button == 7)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/7/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/7/repeat", "1");
  }
  else if (button == 8)
  {
    client.publish("fpp/falcon/player/FPP/set/playlist/8/start", "");
    client.publish("fpp/falcon/player/FPP/set/playlist/8/repeat", "1");
  }
}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  
  if (activeButton != 0)
  {
    TurnOffAllLedsExcept(activeButton);
    waitTimeout = millis() + 10000;
    lastButton = activeButton;
    activeButton = 0;
  }

  if (millis() > waitTimeout)
  {
    int button = getButtonPressed();

    if (button != lastButton)
    {
      TurnOnAllLeds();
      lastButton = button;
    }

    if (button > 0)
    {
      waitTimeout = millis() + 10000;

      TurnOffAllLeds();

      PublishFpp(button);
    }
  }
}

