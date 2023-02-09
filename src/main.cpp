#include "main.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "mytimer.h"

const char* strESPstateTopic    = "homeassistant/" ESPnode "/state";
const char* strESPdebugOTAstate = "homeassistant/" ESPnode "/OTA";
const char* strHAonlineTopic    = "homeassistant/status";

#ifdef enableLEDbuiltin
  #include "ledbuiltin.h"
  MYLEDBUILTIN myled(UPDATE_TIME);
#endif

#ifdef enableMQTTNumbers
  #include "MQTTnumbers.h"  
  MQTTNUMBERS mqttNumbers(UPDATE_TIME);
#endif

#ifdef enableBinarySensors
  #include "mybinarysensors.h"
  MYBINARYSENSORS mybinsensors;
#endif

#ifdef enableMQTTswitches
  #include "MQTTswitches.h"
  MQTTSWITCHES mqttSwitches(UPDATE_TIME);
#endif

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
char msg[MSG_BUFFER_SIZE];
char buf[AUX_BUFFER_SIZE];

/// short Script for Publishing payload to mqtt topic
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

  #ifdef enableLEDbuiltin
    PUBLISHMSG(myled, "Publish config   [%s], %s\n\n", false, MYLEDBUILTIN::cfg)
  #endif

  #ifdef enableBinarySensors
    for (int i = 0; i < BINSENSORS_QUANTITY; i++)
      { PUBLISHMSG(mybinsensors, "Publish config   [%s], %s\n\n", false, MYBINARYSENSORS::cfg, i) }
  #endif

  #ifdef enableMQTTswitches
    for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)
      { PUBLISHMSG(mqttSwitches, "Publish config   [%s], %s\n\n", false, MQTTSWITCHES::cfg, i)  }
  #endif

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

  #ifdef enableMQTTNumbers
    for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
      { PUBLISHMSG(mqttNumbers, "Publish config   [%s], %s\n\n", false, MQTTNUMBERS::cfg, i) }
  #endif

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
  bool getTopic = false, setTopic = false, undefined = true;
  String strTopic = String(topic);
  String strPayload;
  for (unsigned int i = 0; i < length; i++)
    strPayload += (char)payload[i];
  Serial.printf("Recieved message [%s] %s\n", topic, strPayload.c_str());
  
  // LED Builtin
  #ifdef enableLEDbuiltin
    undefined = myled.stateUndefined();
    setTopic = strTopic.equals(myled.getTopic(MYLEDBUILTIN::set));
    if (!setTopic)  getTopic = strTopic.equals(myled.getTopic(MYLEDBUILTIN::get));
    if (getTopic || setTopic)
    {
      if (myled.parsePayload(strPayload))
      {
        if (getTopic || (undefined && (!myled.stateUndefined())))          
          {
            mqttClient.unsubscribe(myled.getTopic(MYLEDBUILTIN::get));
            Serial.printf("Complying with previous LED state. Unsubscribed from GET topic [%s]\n", myled.getTopic(MYLEDBUILTIN::get));
          }    
        if (setTopic && (myled.stateChanged()))
          { PUBLISHMSG(myled, "Publish message  [%s] %s\n", true, MYLEDBUILTIN::get)  }      
      }
      else
        Serial.printf("Error parsing payload! (for 'myled' topic)\n");
    }
  #endif

  // MQTT Switches
  #ifdef enableMQTTswitches
    for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)
      if (!(setTopic||getTopic))
      {
        undefined = mqttSwitches.stateUndefined(i);
        setTopic = strTopic.equals(mqttSwitches.getTopic(MQTTSWITCHES::set,i));
        if (!setTopic)    getTopic = strTopic.equals(mqttSwitches.getTopic(MQTTSWITCHES::get,i));
        if (getTopic||setTopic)
        {
          if (mqttSwitches.parsePayload(strPayload,i))
          {
            if (getTopic || (undefined && (!mqttSwitches.stateUndefined(i))))
            {
              mqttClient.unsubscribe(mqttSwitches.getTopic(MQTTSWITCHES::get,i));
              Serial.printf("Complying with previous MQTT Switches state. Unsubscribed from GET topic [%s]\n", mqttSwitches.getTopic(MQTTSWITCHES::get, i));
            }
            if (setTopic && mqttSwitches.stateChanged(i))            
              { PUBLISHMSG(mqttSwitches, "Publish message  [%s] %s\n", true, MQTTSWITCHES::get, i)   }            
          }
          else
            Serial.printf("Error parsing payload! (for 'mqttSwitches' topic)\n");
        }
      }
  #endif

  // RGB LED-Strip
  #ifdef enableRGB
    if (!(setTopic||getTopic))
    {
      undefined = myrgb.stateUndefined();
      setTopic = strTopic.equals(myrgb.getTopic(MYRGB::set));
      if (!setTopic)  getTopic = strTopic.equals(myrgb.getTopic(MYRGB::get));
      if (setTopic || getTopic)
      {
        if (myrgb.parsePayload(strPayload))
        {
          if (myrgb.stateUndefined())   myrgb.stateUndefined(false);    // todo: work to get rid of this!!!
          if (getTopic || (undefined && (!myrgb.stateUndefined())))
          {
            mqttClient.unsubscribe(myrgb.getTopic(MYRGB::get));
            Serial.printf("Complying with previous RGB state. Unsubscribed from GET topic [%s]\n", myrgb.getTopic(MYRGB::get));
          }
          if (setTopic && myrgb.stateChanged())
            { PUBLISHMSG(myrgb, "Publish message  [%s] %s\n", true, MYRGB::get)  }          
        }
        else
          Serial.printf("Error parsing payload! (for 'myrgb' topic)\n");
      }
    }
  #endif

  // MQTT Numbers (used in couple with RGB LED-Strip)
  #ifdef enableMQTTNumbers
    for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
      if (!(setTopic||getTopic))
      {
        undefined = mqttNumbers.stateUndefined(i);
        setTopic = strTopic.equals(mqttNumbers.getTopic(MQTTNUMBERS::set, i));
        if (!setTopic)  getTopic = strTopic.equals(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
        if (getTopic||setTopic)
        {
          if (mqttNumbers.parsePayload(strPayload,i))
          {
            if (getTopic || (undefined && (!mqttNumbers.stateUndefined(i))))
            {
              mqttClient.unsubscribe(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
              Serial.printf("Complying with previous MQTT Numbers state. Unsubscribed from GET topic [%s]\n", mqttNumbers.getTopic(MQTTNUMBERS::get, i));
            }
            if (setTopic) // && mqttNumbers.stateChanged(i))
            {
              PUBLISHMSG(mqttNumbers, "Publish message  [%s] %s\n", true, MQTTNUMBERS::get, i)
              #ifdef enableRGB
                myrgb.setParam((MYRGB::PARAM_T)mqttNumbers.state(i),i);
                // MYRGB::MYDATA value = myrgb.state();
                // bool RGBundefined = myrgb.stateUndefined();
                // switch (i)
                // {
                //   case 0: value.quantity = mqttNumbers.state(i); break;
                //   case 1: value.speed = mqttNumbers.state(i); break;
                //   case 2: value.hue = mqttNumbers.state(i); break;
                //   case 3: value.transitionTime = mqttNumbers.state(i); break;
                // }            
                // myrgb.state(value);
                // if (RGBundefined)  myrgb.stateUndefined(true);
              #endif  
            }
          }
          else
            Serial.printf("Error parsing payload! (for 'mqttNumbers' topic)\n");
        }
      }
  #endif

  // Rarest to be message
  if (!(setTopic||getTopic))
    if (strTopic.equals(strHAonlineTopic))
      if (strPayload.equals("online"))
        publishConfigs();
}

/// Initiate MQTT (Re)Connection and topic Subscription
void reconnect() {
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" ESPnode;
    // clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), strESPstateTopic, 0, false, "offline"))
    {
      Serial.println(" connected!");
      Serial.printf("Published Last Will. Client_id:\"%s\", topic [%s] payload [\"offline\"], retained Will? \"false\"\n",
                    clientId.c_str(), strESPstateTopic);

      publishConfigs();

      #ifdef enableLEDbuiltin
        mqttClient.subscribe(myled.getTopic(MYLEDBUILTIN::set));
        mqttClient.subscribe(myled.getTopic(MYLEDBUILTIN::get));
      #endif

      #ifdef enableMQTTswitches
        for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)
        {
          mqttClient.subscribe(mqttSwitches.getTopic(MQTTSWITCHES::set, i));
          mqttClient.subscribe(mqttSwitches.getTopic(MQTTSWITCHES::get, i));
        }        
      #endif

      #ifdef enableMQTTNumbers
        for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
        {
          mqttClient.subscribe(mqttNumbers.getTopic(MQTTNUMBERS::set, i));
          mqttClient.subscribe(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
        }
      #endif

      #ifdef enableRGB
        mqttClient.subscribe(myrgb.getTopic(MYRGB::set));
        mqttClient.subscribe(myrgb.getTopic(MYRGB::get));
      #endif

      mqttClient.subscribe(strHAonlineTopic);            
      mqttClient.publish(strESPstateTopic, "online", true);
      Serial.printf("Published message: [%s] online\n", strESPstateTopic);
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

  ArduinoOTA.onStart(     []()  { Serial.printf("\nOTA Start\n"); });
  ArduinoOTA.onEnd(       []()  { Serial.printf("\nOTA End\n"); });
  ArduinoOTA.onProgress(  [](unsigned int progress, unsigned int total)
                                { Serial.printf("\nOTA Progress: %u%%\n", (progress / (total / 100))); });
  ArduinoOTA.onError(     [](ota_error_t error)
                                { Serial.printf("\nOTA Error[%u]: ", error);
                                  if (error == OTA_AUTH_ERROR)            { Serial.println("Auth Failed"); }
                                  else if (error == OTA_BEGIN_ERROR)      { Serial.println("Begin Failed"); }
                                  else if (error == OTA_CONNECT_ERROR)    { Serial.println("Connect Failed"); }
                                  else if (error == OTA_RECEIVE_ERROR)    { Serial.println("Receive Failed"); }
                                  else if (error == OTA_END_ERROR)        { Serial.println("End Failed"); }  });
  ArduinoOTA.begin();
  tmrUpdate.start();
}


/// Main loop
void loop() {
  // OTA update support
  ArduinoOTA.handle();
  // MQTT client handling
  if (!mqttClient.connected())    reconnect();
  mqttClient.loop();
 
  if (tmrUpdate.click())
  {    
    #ifdef enableDHT
      mydht.update();
      if (mydht.stateChanged(MYDHT::temperature))
        { PUBLISHMSG(mydht, "Publish message  [%s] %s\n", true, MYDHT::get, MYDHT::temperature)     }
      if (mydht.stateChanged(MYDHT::humidity))
        { PUBLISHMSG(mydht, "Publish message  [%s] %s\n", true, MYDHT::get, MYDHT::humidity)    }
    #endif

    #ifdef enableLUX
      mylux.update();
      if (mylux.stateChanged())
        { PUBLISHMSG(mylux, "Publish message  [%s] %s\n", true, MYLUX::get)      }
    #endif

    #ifdef enableDS18B20
      myds18b20.update();
      if (myds18b20.stateChanged())
        { PUBLISHMSG(myds18b20, "Publish message  [%s] %s\n", true, MYDS18B20::get)      }
    #endif

    #ifdef enableBMP180
      mybmp180.update();
      if (mybmp180.stateChanged(MYBMP180::pressure))
        { PUBLISHMSG(mybmp180, "Publish message  [%s] %s\n", true, MYBMP180::get, MYBMP180::pressure)      }
      if (mybmp180.stateChanged(MYBMP180::temperature))
        { PUBLISHMSG(mybmp180, "Publish message  [%s] %s\n", true, MYBMP180::get, MYBMP180::temperature)      }
    #endif

    #ifdef enableLEDbuiltin
      if (myled.stateUndefined() && myled.undefinedTimeout())
      {
        mqttClient.unsubscribe(myled.getTopic(MYLEDBUILTIN::get));
        Serial.printf("Timeout getting previous LED state. Unsubscribed from topic [%s]\n", myled.getTopic(MYLEDBUILTIN::get));
        PUBLISHMSG(myled,"Publish message  [%s] %s\n", true, MYLEDBUILTIN::get)
      }
    #endif

    #ifdef enableMQTTswitches
      for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)      
        if (mqttSwitches.stateUndefined(i) && mqttSwitches.undefinedTimeout())
        {
          mqttClient.unsubscribe(mqttSwitches.getTopic(MQTTSWITCHES::get,i));
          Serial.printf("Timeout getting previous MQTT Switches state. Unsubscribed from topic [%s]\n", mqttSwitches.getTopic(MQTTSWITCHES::get,i));
          PUBLISHMSG(mqttSwitches,"Publish message  [%s] %s\n", true, MQTTSWITCHES::get,i)
        }
    #endif

    #ifdef enableMQTTNumbers
      for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
        if (mqttNumbers.stateUndefined(i) && mqttNumbers.undefinedTimeout())
        {
          mqttClient.unsubscribe(mqttNumbers.getTopic(MQTTNUMBERS::get, i));
          Serial.printf("Timeout getting previous MQTT number state. Unsubscribed from topic [%s]\n", mqttNumbers.getTopic(MQTTNUMBERS::get, i));
          PUBLISHMSG(mqttNumbers,"Publish message  [%s] %s\n", true, MQTTNUMBERS::get, i)
        }
    #endif

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
      { PUBLISHMSG(myrgb, "Publish message  [%s] %s\n", true, MYRGB::get)    }  
  #endif

  #ifdef enableBinarySensors
    mybinsensors.update();
    for (int i = 0; i < BINSENSORS_QUANTITY; i++)
      if (mybinsensors.stateChanged(i))
        { PUBLISHMSG(mybinsensors, "Publish message  [%s] %s\n",true, MYBINARYSENSORS::get, i)     }
  #endif  
}