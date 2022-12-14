#include "main.h"
#include "ledbuiltin.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "mytimer.h"
#include "MQTTnumbers.h"

#ifdef enableDHT
  #include "mydht.h"
  MYDHT mydht;
#endif

#ifdef enableLUX
  #include "mygy30.h"
  MYLUX mylux;
#endif

#ifdef enableRGB
  #include "myrgb.h"
  MYRGB myrgb(UPDATE_TIME);
#endif

#ifdef enableDS18B20                // or if defined any other sensor that uses OneWire
  #include <OneWire.h>
  OneWire onewire(ONEWIRE_PIN);
#endif

#ifdef enableDS18B20
  #include "myds18b20.h"
  MYDS18B20 myds18b20;
#endif

#ifdef enableBMP180
  #include "mybmp180.h"
  MYBMP180 mybmp180;
#endif

const char* ssid = "Keenetic-9158";
const char* password = "tSkjdXFC";
const char* mqtt_server = "raspberrypi.local";
WiFiClient espClient;
PubSubClient mqttClient(espClient);

MYTIMER tmrUpdate(UPDATE_TIME);
MYLEDBUILTIN myled(UPDATE_TIME);
MQTTNUMBERS mqttNumbers(UPDATE_TIME);
char msg[MSG_BUFFER_SIZE];
char buf[AUX_BUFFER_SIZE];

const char* strESPstateTopic    = "homeassistant/" ESPnode "/state";
const char* strESPdebugOTAstate = "homeassistant/" ESPnode "/OTA";
const char* strHAonlineTopic    = "homeassistant/status";

#define PUBLISHMSG(OBJ, STRINGMSG, RETAINED, ...) \
    OBJ.getPayload(msg, __VA_ARGS__); \
    Serial.printf(STRINGMSG, OBJ.getTopic(__VA_ARGS__), msg); \
    mqttClient.publish(OBJ.getTopic(__VA_ARGS__), msg, RETAINED);

/// Setup WIFI Connection
void setup_wifi() {
  delay(10);
  Serial.printf("\nConnecting to %s", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.print("\nWiFi connected\nIP address:\n");
  Serial.println(WiFi.localIP());
}

/// Publish MQTT configs on Start or Reboot
void publishConfigs() {
  mqttClient.publish(strESPdebugOTAstate, "Ready", true);

  PUBLISHMSG(myled, "Publish config   [%s], %s\n\n", false, MYLEDBUILTIN::cfg)

  #ifdef enableLUX
    PUBLISHMSG(mylux, "Publish config   [%s], %s\n\n", false, MYLUX::cfg)
  #endif

  #ifdef enableDHT
    PUBLISHMSG(mydht, "Publish config   [%s], %s\n\n", false, MYDHT::cfg, MYDHT::temperature)
    PUBLISHMSG(mydht, "Publish config   [%s], %s\n\n", false, MYDHT::cfg, MYDHT::humidity)
  #endif

  #ifdef enableRGB
    PUBLISHMSG(myrgb, "Publish config   [%s], %s\n\n", false, MYRGB::cfg)
  #endif

  for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
    { PUBLISHMSG(mqttNumbers, "Publish config   [%s], %s\n\n", false, MQTTNUMBERS::cfg, i) }

  #ifdef enableDS18B20
    PUBLISHMSG(myds18b20, "Publish config   [%s], %s\n\n", false, MYDS18B20::cfg)
  #endif

  #ifdef enableBMP180
    PUBLISHMSG(mybmp180, "Publish config   [%s], %s\n\n", false, MYBMP180::cfg, MYBMP180::pressure)
    PUBLISHMSG(mybmp180, "Publish config   [%s], %s\n\n", false, MYBMP180::cfg, MYBMP180::temperature)
  #endif
} 

/// CALLBACK function for processing of Incoming Payloads
void callback(char* topic, byte* payload, unsigned int length) {
  String strTopic = String(topic);
  String strPayload;
  for (unsigned int i = 0; i < length; i++)
    strPayload += (char)payload[i];
  Serial.printf("Recieved message [%s] %s\n", topic, strPayload.c_str());
  
  if (strTopic.equals(myled.getTopic(MYLEDBUILTIN::get)))
  {
    if (myled.parsePayload(strPayload))
    {
      mqttClient.unsubscribe(myled.getTopic(MYLEDBUILTIN::get));
      Serial.printf("Complying with previous get LED state. Unsubscribed from topic [%s]\n", myled.getTopic(MYLEDBUILTIN::get));
    }
  }
  else if (strTopic.equals(myled.getTopic(MYLEDBUILTIN::set)))
  {
    myled.parsePayload(strPayload);
    if (myled.stateChanged())
      {
        PUBLISHMSG(myled, "Publish message  [%s] %s\n", true, MYLEDBUILTIN::get)
      }
  }
  #ifdef enableRGB
  else if (strTopic.equals(myrgb.getTopic(MYRGB::get)))
  {    
    if (myrgb.stateUndefined() && myrgb.parsePayload(strPayload))
    {
      myrgb.stateUndefined(false);
      mqttClient.unsubscribe(myrgb.getTopic(MYRGB::get));
      Serial.printf("Complying with previous RGB get state. Unsubscribed from topic [%s]\n", myrgb.getTopic(MYRGB::get));
    }
  }  
  else if (strTopic.equals(myrgb.getTopic(MYRGB::set)))
  {
    if (myrgb.stateUndefined())      
    {
      if (myrgb.parsePayload(strPayload))
      {
        myrgb.stateUndefined(false);
        mqttClient.unsubscribe(myrgb.getTopic(MYRGB::get));
        Serial.printf("Complying with new RGB set state. Unsubscribed from topic [%s]\n", myrgb.getTopic(MYRGB::get));
      }
    }
    else
    {
      myrgb.parsePayload(strPayload);
    }        
  }
  #endif
  else if (strTopic.equals(strHAonlineTopic))
  {
    if (strPayload.equals("online"))
      publishConfigs();
  }

  for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
  {
      bool equalsSet = strTopic.equals(mqttNumbers.getTopic(MQTTNUMBERS::set, i));
      bool equalsGet = strTopic.equals(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
      if (equalsGet || equalsSet)
      {
        bool numberUndefined = mqttNumbers.stateUndefined(i);
        if (mqttNumbers.parsePayload(strPayload, i))
        {
            if (numberUndefined)
            {
                mqttClient.unsubscribe(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
                Serial.printf("Complying with previous get MQTT Numbers state. Unsubscribed from topic [%s]\n", mqttNumbers.getTopic(MQTTNUMBERS::get, i));
            }
            if (mqttNumbers.stateChanged(i))
            {
              PUBLISHMSG(mqttNumbers, "Publish message  [%s] %s\n", true, MQTTNUMBERS::get, i)
              #ifdef enableRGB
                MYRGB::MYDATA value = myrgb.state();
                bool RGBundefined = myrgb.stateUndefined();
                switch (i)
                {
                  case 0: value.quantity = mqttNumbers.state(i); break;
                  case 1: value.speed = mqttNumbers.state(i); break;
                  case 2: value.hue = mqttNumbers.state(i); break;
                  case 3: value.transitionTime = mqttNumbers.state(i); break;
                }            
                myrgb.state(value);
                if (RGBundefined)  myrgb.stateUndefined(true);
              #endif          
            }
        }
      }
  }
}

/// Initiate MQTT (Re)Connection and topic Subscription
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" ESPnode;
    // clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), strESPstateTopic, 0, false, "offline"))
    {
      Serial.println(" connected!");
      Serial.printf("Published Last Will. Client_id:\"%s\", topic [%s] payload [\"offline\"], retained Will? \"false\"\n",
                    clientId.c_str(), strESPstateTopic);
      publishConfigs();
      mqttClient.subscribe(myled.getTopic(MYLEDBUILTIN::set));
      mqttClient.subscribe(myled.getTopic(MYLEDBUILTIN::get));
      myled.stateUndefined(true);
      for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
      {
        mqttClient.subscribe(mqttNumbers.getTopic(MQTTNUMBERS::set, i));
        mqttClient.subscribe(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
      }
      #ifdef enableRGB
        mqttClient.subscribe(myrgb.getTopic(MYRGB::set));
        mqttClient.subscribe(myrgb.getTopic(MYRGB::get));
      #endif
      mqttClient.subscribe(strHAonlineTopic);
      snprintf (msg, MSG_BUFFER_SIZE, "online"); 
      Serial.printf("Publish message: [%s] %s\n", strESPstateTopic, msg);
      mqttClient.publish(strESPstateTopic, msg, true);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

/// Setup WiFi, MQTT Connections, Arduino OTA
void setup() {
  Serial.begin(115200);
  Serial.println("\nRESET");
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(512);  // since ESP8266 has enough memory
  // mqttClient.setKeepAlive(5);  // (seconds)
  Serial.printf("PubSubClient.getBufferSize()=%u\n", mqttClient.getBufferSize());

  ArduinoOTA.onStart([]() { Serial.println("OTA Start"); mqttClient.publish(strESPdebugOTAstate, "Start"); });
  ArduinoOTA.onEnd([]()   { Serial.println("\nOTA End"); mqttClient.publish(strESPdebugOTAstate, "End"  ); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)  {
    Serial.printf("OTA Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) { Serial.println("Auth Failed"); mqttClient.publish(strESPdebugOTAstate, "Auth Failed"); }
    else if (error == OTA_BEGIN_ERROR) { Serial.println("Begin Failed"); mqttClient.publish(strESPdebugOTAstate, "Begin Failed"); }
    else if (error == OTA_CONNECT_ERROR) { Serial.println("Connect Failed"); mqttClient.publish(strESPdebugOTAstate, "Connect Failed"); }
    else if (error == OTA_RECEIVE_ERROR) { Serial.println("Receive Failed"); mqttClient.publish(strESPdebugOTAstate, "Receive Failed"); }
    else if (error == OTA_END_ERROR) { Serial.println("End Failed"); mqttClient.publish(strESPdebugOTAstate, "End Failed"); }
  });
  ArduinoOTA.begin();
  tmrUpdate.start();
}

void loop() {

  ArduinoOTA.handle();
  if (!mqttClient.connected())
    { reconnect(); }

  mqttClient.loop();
  // Serial.println("loop()");
 
  if (tmrUpdate.click()) {    
    // Serial.println("tmrUpdate.click()");
    #ifdef enableDHT
      mydht.update();
      if (mydht.stateChanged(MYDHT::temperature))
      {
        PUBLISHMSG(mydht, "Publish message  [%s] %s\n", true, MYDHT::get, MYDHT::temperature)
      }
      if (mydht.stateChanged(MYDHT::humidity))
      {
        PUBLISHMSG(mydht, "Publish message  [%s] %s\n", true, MYDHT::get, MYDHT::humidity)
      }
    #endif

    #ifdef enableLUX
      mylux.update();
      if (mylux.stateChanged())
      {
        PUBLISHMSG(mylux, "Publish message  [%s] %s\n", true, MYLUX::get)
      }
    #endif

    #ifdef enableDS18B20
      myds18b20.update();
      if (myds18b20.stateChanged())
      {
        PUBLISHMSG(myds18b20, "Publish message  [%s] %s\n", true, MYDS18B20::get)
      }
    #endif

    #ifdef enableBMP180
      mybmp180.update();
      if (mybmp180.stateChanged(MYBMP180::pressure))
      {
        PUBLISHMSG(mybmp180, "Publish message  [%s] %s\n", true, MYBMP180::get, MYBMP180::pressure)
      }
      if (mybmp180.stateChanged(MYBMP180::temperature))
      {
        PUBLISHMSG(mybmp180, "Publish message  [%s] %s\n", true, MYBMP180::get, MYBMP180::temperature)
      }
    #endif

    if (myled.stateUndefined() && myled.undefinedTimeout())
    {
      mqttClient.unsubscribe(myled.getTopic(MYLEDBUILTIN::get));
      Serial.printf("Timeout getting previous LED state. Unsubscribed from topic [%s]\n", myled.getTopic(MYLEDBUILTIN::get));
      PUBLISHMSG(myled,"Publish message  [%s] %s\n", true, MYLEDBUILTIN::get)
    }

    for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
    {
      if (mqttNumbers.stateUndefined(i) && mqttNumbers.undefinedTimeout())
      {
        mqttClient.unsubscribe(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
        Serial.printf("Timeout getting previous MQTT number state. Unsubscribed from topic [%s]\n", mqttNumbers.getTopic(MQTTNUMBERS::get, i));
        PUBLISHMSG(mqttNumbers,"Publish message  [%s] %s\n", true, MQTTNUMBERS::get, i)
      }
    }

    #ifdef enableRGB
    if (myrgb.stateUndefined() && myrgb.undefinedTimeout())
    {
      myrgb.stateUndefined(false);
      mqttClient.unsubscribe(myrgb.getTopic(MYRGB::get));
      Serial.printf("Timeout getting previous RGB state. Unsubscribed from topic [%s]\n", myrgb.getTopic(MYRGB::get));
      PUBLISHMSG(myrgb, "Publish message  [%s] %s\n", true, MYRGB::get)
    }
    #endif
  }

  #ifdef enableRGB
    myrgb.update();
    if ((!myrgb.stateUndefined()) && (myrgb.stateChanged()))
    {
      PUBLISHMSG(myrgb, "Publish message  [%s] %s\n", true, MYRGB::get)
    }  
  #endif
}