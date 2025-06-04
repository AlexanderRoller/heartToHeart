#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <math.h>

// global BPM variable
float bpm = 80.0f;

// FastLED
#include <FastLED.h>
#define NUM_LEDS 1
#define DATA_PIN 8
CRGB leds[NUM_LEDS];

// mqtt topics
#define AWS_IOT_SUBSCRIBE_TOPIC "hr/test"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  // use AWS credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // connect to AWS
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IoT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(1000);
  }

  if (!client.connected())
  {
    Serial.println("Failed to connect to AWS IoT");
    return;
  }

  // subscribe to topic
  bool ok = client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.printf("Subscribe to “%s”: %s\n", AWS_IOT_SUBSCRIBE_TOPIC, ok ? "OK" : "FAIL");

  Serial.println("Connected to AWS IoT");
}

void messageHandler(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // update the global bpm
  bpm = doc["bpm"].as<float>();
  Serial.print("Updated global BPM: ");
  Serial.println(bpm);

}

void setup()
{
  Serial.begin(115200);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  connectAWS();
}

void loop()
{
  client.loop();
  wave(int(bpm));
}

void blink(float freq){  
  // freq is in cycles per minute, so one cycle = 60000 ms / freq
  float periodMs = 60000.0f / freq;      
  float halfPeriod = periodMs / 2.0f;    // on for half cycle, off for half cycle

  // turn LED(s) on
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
  delay((uint32_t)halfPeriod);

  // turn LED(s) off
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  delay((uint32_t)halfPeriod);
}

void wave(int freq) {
  // non-blocking sine-wave pulsing based on millis()
  static float angle = 0.0f;
  static int prevFreq = -1;
  static unsigned long lastMs = 0;
  const float twoPi = 6.28318530718f;
  unsigned long now = millis();

  // reset phase when frequency changes
  if (freq != prevFreq) {
    angle = 0.0f;
    prevFreq = freq;
    lastMs = now;
  }

  // advance phase by elapsed time
  unsigned long dt = now - lastMs;
  float periodMs = 60000.0f / (float)freq;
  angle += twoPi * dt / periodMs;
  if (angle >= twoPi) angle -= twoPi;

  // compute brightness and update LED
  uint8_t bright = (uint8_t)(255.0f * (0.5f + 0.5f * sin(angle)));
  FastLED.setBrightness(bright);
  leds[0] = CRGB::Red;
  FastLED.show();

  lastMs = now;
}