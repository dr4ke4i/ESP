# ESP

This is my home project.

I'm using free and open-source HomeAssistant (www.home-assistant.io) as a shell to control devices and gather information on my Home. 

MQTT protocol is used to interact with ESP-12E (NodeMCU 1.0) modules, which are connected to devices and sensors physically (with wires).

HomeAssistant has MQTT integration with MQTT server. HomeAssistant and MQTT server are both running on my RaspberryPi ARM computer.

MQTT protocol allows auto-discovery of sensors and devices in the HomeAssistant shell. All of the included sensors and devices have auto-discovery.

Each ESP module interacts remotely with MQTT server using WiFi connection via home router.

What you see here is a code for all of my ESP modules.


## Info about the code

Each ESP has a profile of sensors connected. Choose the contents of the profile in the config section of 'main.h'. Also check sensor pins.

For this project I'm using free Visual Studio Code in pair with Platform.io to allow it to work with Arduino-like platforms.

That's why it's also required to modify 'platformio.ini' to choose your upload method (Over The Air or via COM-port for the first time) and path to the libraries.

Check required library name in corresponding #include <*.h> file.


## Other

Currently supported sensors are:
- DHT (DHT22) for temperature and humidity,
- DS18B20 for temperature,
- BMP180 for atmospheric pressure,
- BH1750FVI for light intensity.

Devices are:
- built-in LED, which coupled with a transistor can also be used as ouput relay,
- LED strip as light component, using FastLED library

All of these devices have been tested in my home environment.
