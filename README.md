# LittleESPServer
A simple webserver for Internet of Things + Arduino/Teensy.

I've looked around to find a simple and solid setup for using the ESP8266 module to provide wifi for my Arduino projects. What I want is to type a few lines of code and then have access to the web or a simple web server. In lack of other solutions, I came up with this and I use it for some of my own projects. If it's useful to others - feel free to use it.

I made this because I use it for my own stuff. If you choose to use it and your stuff fails, don't blame me. I do however appreciate pull requests to fix/improve stuff. The more formal License is below:

# Version history

0.1 Initial release (16. Sept 2015)

  * A basic, bare bones server setup that can connect to access points or be one by itself
  * Serves up any kind of pages in response to a request
  * Really just works with a single connected client. Will solve this with a tiny que-system in next version that can handle the possible 4 requests that the 
  * Not optimised for use with Arduino Uno and other MCU's with less memory. I'll later optimise and change most Strings to char arrays to save memory
  * Only has a single example, showing how to set up a simple http-server

#Requirements
The most stable setup is to use an ESP-12 module (or similar) with the latest firmware/SDK from Espressif. Read here to learn how to update the firmware http://flashgamer.com/hardware/comments/esp8266-and-stability

I'll post details on hooking up the different modules at http://www.flashgamer.com/arduino/LittleESPServer in the future, but for now - just follow the instructions here https://www.arduino.cc/en/Guide/Libraries to install the downloaded files 

#License
This program is free software; you can redistribute it and/or modify it under the terms of the GNU License. This program is distributed in the hope that it will be useful but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the The MIT License (MIT) for more details. https://en.wikipedia.org/wiki/MIT_License.

