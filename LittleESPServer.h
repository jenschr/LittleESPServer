/***************************************************************************
 *                                                                         *
 * LittleESPServer                                                         *
 * A simple webserver for Internet of Things + Arduino/Teensy              *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 * Requires that an ESP8266 based chip is connected correctly. Check       *
 * http://www.flashgamer.com/arduino/LittleESPServer for details on        *
 * hooking up your ESP 8266 based module.                                  *
 *                                                                         *
 ***************************************************************************
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU License.                                  *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * The MIT License (MIT) for more details.                                 *
 * https://en.wikipedia.org/wiki/MIT_License                               *
 ***************************************************************************/

#if ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif

#include <Stream.h>

#ifndef LittleESPServer_h
#define LittleESPServer_h

#define AT_TEST                 "AT" // 'ok' just test to see if serial works
#define AT_SET_MODE             "AT+CWMODE=" // add cwmode_t after this. returns 'cwmode3 failed or unchanged' if same
#define AT_GET_DATA_MODE        "AT+CIPMODE?" // Check for single
#define AT_SET_DATA_MODE        "AT+CIPMODE=0" // 0 is correct for a server setup like this
#define AT_SET_MULTI_CONNECTION "AT+CIPMUX=1" // Use 1 since we want to support multiple connections
#define AT_CONNECT_TO_WAN       "AT+CWJAP=" // add SSID+pass strings to this with comma in between
#define AT_START_SERVER         "AT+CIPSERVER=1," //'ok' add port number after this. 8080 is typical
#define AT_SET_BAUD             "AT+CIOBAUD=" // add baud rate after this. 57600 is a good option
#define AT_CHECK_VERSION        "AT+GMR" // 'ok' returns a string with numbers like '0018000902'
#define AT_CHECK_IP_ADDRESS     "AT+CIFSR" // 'ok' returns whatever IP's the chip has. Up to two possible.
#define AT_RESET_CHIP           "AT+RST" // Tries to restart the ESP8266 chip
#define AT_UPDATE_CHIP          "AT+CIUPDATE" // Tries to do a OTA update of the chip firmware. Connect 2 web first!
#define AT_STATUS               "AT+CIPSTATUS" // Tries to do a OTA update of the chip firmware. Connect 2 web first!
#define AT_START                "AT+CIPSTATUS" // followed by id (channel) 0-4, type "TCP", addr, port
#define AT_CLOSE                "AT+CIPCLOSE=" // close a channel

// AP handling
#define AT_SEARCH_FOR_WIFI      "AT+CWLAP" // Starts looking ofr wifi access points

// transfers
#define AT_START_CONNECTION     "AT+CIPSTART?"
#define AT_DO_TRANSFER          "AT+CIPSEND="
#define AT_CLOSE_CONNECTION     "AT+CIPCLOSE=?"

#define DATA_RETURNED_HEADER    "+IPD," // Indicates the start of received data,followed by id/channel of the requester, length of data
                                       // Example: +IPD,0,336:GET /someurl HTTP/1.1 

typedef enum 
{
  STATUS_UNKNOWN          = 0,
  STATUS_SCANNING         = 1,
  STATUS_CONNECTED        = 2,
  STATUS_CONNECTING       = 3,
  STATUS_DISCONNECTED     = 4,
  STATUS_INITIALIZING     = 5,
  STATUS_CONNECTED_W_IP   = 6,
  CIPMODE_ERROR           = 7,
  CIPMUX_ERROR            = 8,
  CIPSERVER_ERROR         = 9,
  CWMODE_ERROR            = 10,
  WIFI_CONNECT_ERROR      = 11
} status_t;

typedef enum 
{
  STANDALONE        = 1,
  ACCESS_POINT      = 2,
  STANDALONE_AND_AP = 3
} cwmode_t;

class LittleESPServer
{
public:
  status_t status;       // set when the server status changes
  bool debugOutput = false;

  // The server will just take a serial connection as Input
  LittleESPServer( Stream *serialConnection );

  void     debug( bool turnOn );
  bool     begin( cwmode_t usageMode, int portNumber );
  bool     begin( cwmode_t usageMode, int portNumber, String ssid, String password, uint8_t attempts = 5);
  status_t getStatus(void);
  void     getAPAddress( IPAddress &ipAddress );
  void     getStandaloneAddress( IPAddress &ipAddress );
  bool     test(void); // 1 = ok, 0 = error
  String   getFirmwareVersion(void);
  String   listSSIDResults(void);
  bool     connectToAP(String& ssid, String& password, uint8_t attempts = 3);
  void     reboot(uint8_t patchReq);
  bool     available();
  char     read(void);
  int      clientID(void);
  void     pathRequested( char *pointerToSavePathIn );
  bool     send( uint8_t clientID, String page );

  // move to private later
  uint8_t  pollForStatus(void);

private:
  byte cipmodeDefault = 0;
  byte cipmuxDefault = 1;
  int portNumber;
  Stream *_serialConnection;
  String parseBuffer;

  // The ESP can serve 4 simultaneous connections
  char requestedUrl[255];
  char requestedUrl0[255];
  char requestedUrl1[255];
  char requestedUrl2[255];
  char requestedUrl3[255];
  bool activeClient0 = false;
  bool activeClient1 = false;
  bool activeClient2 = false;
  bool activeClient3 = false;
  int requestedChannel;
  char readBuffer[1024];

  bool initialized = false;
  int contains( char *haystack, char *needle);
  bool parseSerial( const char *command, int millisecondTimeout, String parseUntilWeSeeThis, String errorIfWeSeeThis );
  bool parseSerial( char *command, int millisecondTimeout, String parseUntilWeSeeThis, String errorIfWeSeeThis );
  bool cwmode( cwmode_t usageMode );
  bool cipmode();
  bool cipmux();
  bool cipserver( int portNumber );
  bool wificonnect( String ssid, String password, uint8_t attempts );
  void checkForInit();
  
  void parseAddress( IPAddress &ipAddress, char* whatToSend , char* whatToLookFor );
  byte _buff[6] ;    //6 bytes buffer for saving data read from the device
};

#endif