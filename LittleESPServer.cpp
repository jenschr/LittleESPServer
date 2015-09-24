#include <SPI.h>
#include <Ethernet.h>
#include "LittleESPServer.h";

LittleESPServer::LittleESPServer( Stream *serialConnection )
{
  _serialConnection = serialConnection;
  status = STATUS_INITIALIZING;
  // set defaults
  initialized = false;
  debugOutput = false;
  cipmodeDefault = 0;
  cipmuxDefault = 1;
  clients[0].active = false;
}

void LittleESPServer::debug( bool turnOn )
{
  debugOutput = turnOn;
}

bool LittleESPServer::begin( cwmode_t usageMode, int portNumber )
{
  bool success = true;
  // TODO: add support for name & PW:
  // https://github.com/espressif/esp8266_at/wiki/CWSAP
  // AT+CWSAP="esp_123","1234567890",2,0
  // AT+CWSAP="ssid name","pwd",channel,0=open/3=WPA2_PSK
  success = cwmode( usageMode );
  if( !success ){ status = CWMODE_ERROR; return false; }
  success = cipmode();
  if( !success ){ status = CIPMODE_ERROR; return false; }
  success = cipmux();
  if( !success ){ status = CIPMUX_ERROR; return false; }
  success = cipserver( portNumber );
  if( !success ){ status = CIPSERVER_ERROR; return false; }
  return success;
}

bool LittleESPServer::begin( cwmode_t usageMode, int portNumber, String ssid, String password, uint8_t attempts)
{
  bool success = begin( usageMode, portNumber );
  // check if we can use this order instead
  if( success ){
    success = wificonnect( ssid, password, attempts );
    if( !success ){ status = WIFI_CONNECT_ERROR; }
  }
  return success;
}

status_t LittleESPServer::getStatus(void)
{
  return status;
}

// 1 = ok, 0 = error
bool LittleESPServer::test(void)
{
  String commandd = "AT";
  commandd += " "; // toCharArray strips off the last char, so add a space for it to strip 
  char cmd[80];
  commandd.toCharArray( cmd, commandd.length() );
  return parseSerial( cmd, 1000, "OK", "ERROR" );
}

String LittleESPServer::getFirmwareVersion(void)
{
  /*

  AT+GMR

  AT version:0.40.0.0(Aug  8 2015 14:45:58)
  SDK version:1.3.0
  compile time:Aug  8 2015 17:19:38
  OK

  */

}

String LittleESPServer::listSSIDResults(void)
{

}

bool LittleESPServer::connectToAP(String& ssid, String& password, uint8_t attempts)
{
  byte success = wificonnect( ssid, password, attempts );
  if( !success ){ status = WIFI_CONNECT_ERROR; }
  return success;
}

void LittleESPServer::reboot(uint8_t patchReq = 0)
{
  bool statusTmp = parseSerial( "AT+RST", 1000, "ready", "ERROR" );
}

// Expected format: +CIPAP:ip:"192.168.4.1"
void LittleESPServer::getAPAddress( IPAddress &ipAddress )
{
  parseAddress( ipAddress, "AT+CIPAP?", "+CIPAP:ip:" );
}

// Expected format: +CIPSTA:ip:"192.168.1.239"
void LittleESPServer::getStandaloneAddress( IPAddress &ipAddress )
{
  parseAddress( ipAddress, "AT+CIPSTA?", "+CIPSTA:ip:" );
}

void LittleESPServer::parseAddress( IPAddress &ipAddress, char* whatToSend , char* whatToLookFor )
{
  bool statusTmp = parseSerial( whatToSend, 5000, "OK", "ERROR" );
  int startPos = parseBuffer.indexOf( whatToLookFor );
  int endPos = parseBuffer.indexOf('\r',startPos);
  int whatLength = strlen( whatToLookFor )+1;
  String ipFound = parseBuffer.substring(startPos+whatLength, endPos);
  int ipLength = ipFound.length();
  char charBuffer[ipLength];
  ipFound.toCharArray( charBuffer, ipLength );
  byte pos = 0;
  char *p = charBuffer;
  char *str;
  char *fsdfsd[4] = {0,0,0,0};
  while (( str = strtok_r(p, ".", &p) ) != NULL )
  {
    ipAddress[pos] = (uint8_t)atoi(str);
    pos++;
  }
}

// reads in what the ESP sends into a 1024 byte buffer
bool LittleESPServer::available()
{
  requestedChannel = -1;
  int i = 0;
  while (_serialConnection->available() > 0) {
    char ch = _serialConnection->read();
    if( ch ){
      if( debugOutput ){
        Serial.print(ch);
      }
      readBuffer[i] = ch;
      i++;
      delay(1); // we absolutely have to do this....
    }
  }

  // if we got something, analyze it
  if( i>0 ){
    readBuffer[i] = '\0'; // Terminate the buffer

    // Look for connects
    if( strstr  ( readBuffer, "0,CONNECT" ) != NULL )
    {
      clients[0].active = true;
      Serial.println("Open 0");
    }
    if( strstr  ( readBuffer, "1,CONNECT" ) != NULL )
    {
      clients[1].active = true;
      Serial.println("Open 1");
    }
    if( strstr  ( readBuffer, "2,CONNECT" ) != NULL )
    {
      clients[2].active = true;
      Serial.println("Open 2");
    }
    if( strstr  ( readBuffer, "3,CONNECT" ) != NULL )
    {
      clients[3].active = true;
      Serial.println("Open 3");
    }

    // Look for closed connections (so we don't send to them)
    if( strstr  ( readBuffer, "0,CLOSED" ) != NULL )
    {
      clients[0].active = false;
      Serial.println("Close 0");
    }
    if( strstr  ( readBuffer, "1,CLOSED" ) != NULL )
    {
      clients[1].active = false;
      Serial.println("Close 1");
    }
    if( strstr  ( readBuffer, "2,CLOSED" ) != NULL )
    {
      clients[2].active = false;
      Serial.println("Close 2");
    }
    if( strstr  ( readBuffer, "3,CLOSED" ) != NULL )
    {
      clients[3].active = false;
      Serial.println("Close 3");
    }

    // Look for URL requests
    int requestStart = contains(readBuffer,"+IPD,");
    if( requestStart != -1 ){
      char channelAsString = readBuffer[requestStart+5]; // the ESP supports max 4 clients
      requestedChannel = channelAsString-'0'; // Nice hack from http://www.arduino-hacks.com/converting-integer-to-character-vice-versa/

      int foundAGet = contains(readBuffer,"GET ");
      if( foundAGet != -1 ){ // Yay! A URL to deliver!
        
        int urlEndAtPosition = contains(readBuffer, " HTTP");
        if( urlEndAtPosition != -1 ){
          int j = 0;
          int c = 0;
          for( j=(foundAGet+4); j<urlEndAtPosition; j++ )
          {
            requestedUrl[c] = readBuffer[j];
            c++;
          }
          //c++;
          requestedUrl[c] = '\0';
        }
        /*
        Serial.println("------------");
        Serial.print("requestedUrl: ");
        Serial.print(requestedUrl);
        Serial.print(" requestedChannel ");
        Serial.println( requestedChannel );
        */
        Serial.print(" requestedUrl ");
        Serial.println( requestedUrl );
        Serial.print(" requestedChannel ");
        Serial.println( channelAsString );
        Serial.println(" ********************* ");
        Serial.println( readBuffer );
        Serial.println(" ********************* ");

        /*
        * If the /favicon.ico file has already been sent,
        * the ESP unit may incorrectly report it as being
        * requested on channel 0.
        */

        if( requestedUrl == "/favicon.ico " ){
          Serial.print("favicon request ignored ");
          Serial.println(requestedUrl);
        } else if( requestedChannel == 0 ){
          strcpy( clients[0].url, requestedUrl );
          Serial.print("GET 0 ");
          Serial.println(requestedUrl);
        } else if( requestedChannel == 1 ){
          strcpy( clients[1].url, requestedUrl );
          Serial.print("GET 1 ");
          Serial.println(requestedUrl);
        } else if( requestedChannel == 2 ){
          strcpy( clients[2].url, requestedUrl );
          Serial.print("GET 2 ");
          Serial.println(requestedUrl);
        } else if( requestedChannel == 3 ){
          strcpy( clients[3].url, requestedUrl );
          Serial.print("GET 3 ");
          Serial.println(requestedUrl);
        }
      }
      //Serial.print("readBuffer ");
      //Serial.println( readBuffer );
    }
  }
  return requestedChannel > -1;
}

int LittleESPServer::contains( char *haystack, char *needle)
{
  int rez = -1;
  int haystackLength = strlen( haystack );
  char *found = strstr( haystack, needle);
  int needleLength = strlen( found );
  if(needleLength > 0)
  {
    rez = haystackLength - needleLength;
  }
  return rez;
}

char LittleESPServer::read(void)
{
  // save what to return
  char ret = readBuffer[0];
  // shift the array
  byte i;
  byte len = strlen(readBuffer);
  for(i = 0; i < len; i++)
  {
    readBuffer[i] = readBuffer[i+1];
  }
  return ret;
}

int LittleESPServer::clientID(void)
{
  return requestedChannel;
}

void LittleESPServer::pathRequested( char *pointerToSavePathIn )
{
  strcpy( pointerToSavePathIn, requestedUrl );
}

bool LittleESPServer::send( uint8_t clientID, String page )
{
  //AT+CIPSEND=id,length,data
  //if( clients[0].active != clientID){ return false; }

  int contentLength = page.length();
  String commandd = AT_DO_TRANSFER;
  
  commandd += requestedChannel;
  commandd += ",";
  commandd += contentLength;

  commandd += " "; // toCharArray strips off the last char, so add a space for it to strip
  char cmd[80];
  commandd.toCharArray( cmd, commandd.length() );

  delay(10);
  if( debugOutput ){
    Serial.println("------------------------------");
    Serial.print("Sending: ");
    Serial.println(cmd);
  }

  // send the return
  if ( parseSerial( cmd, 4000, ">", "ERROR" ) ) {  //prompt from ESP8266 indicating ready
    _serialConnection->print( page );
    delay(50);
    _serialConnection->print( "+++" );
    if( debugOutput ){
      Serial.println("Page returned!");
    }

    while( !parseSerial( "", 4000, "SEND OK", "busy s..." ) )
    {
      delay(1000);
      if( debugOutput ){
        Serial.println("Waiting... ");
      }
    }
    
  } else if( debugOutput ){
    Serial.println("Sending failed");
    Serial.println("------------------------------");
  }
  // should return SEND OK
  delay(10);

  // Close to save connections
  commandd = AT_CLOSE;
  commandd += requestedChannel;
  commandd += " ";

  char cmd2[80];
  commandd.toCharArray( cmd2, commandd.length() );
  bool wasClosed = parseSerial( cmd2, 4000, "OK", "ERROR" );
  if( debugOutput ){
    Serial.print("Closed? ");
    Serial.println(wasClosed);
    Serial.println("------------------------------");
  }
  return true;
}

// Private stuff
typedef enum 
{
  PARSING = 1,
  OK      = 2,
  ERROR   = 3,
  TIMEOUT = 4,
  BUSY    = 5
} parseStatus_t;

bool LittleESPServer::parseSerial( const char *command, int millisecondTimeout, String parseUntilWeSeeThis, String errorIfWeSeeThis )
{
  String commandd = command;
  commandd += " "; // toCharArray strips off the last char, so add a space for it to strip 
  char cmd[80];
  commandd.toCharArray( cmd, commandd.length() );
  parseSerial( cmd, 1000, "OK", "ERROR" );
}

bool LittleESPServer::parseSerial( char *command, int millisecondTimeout, String parseUntilWeSeeThis, String errorIfWeSeeThis )
{
  checkForInit();
  if( debugOutput ){
    Serial.print("parseSerial: ");
    Serial.println(command);
    Serial.print("Sending: ");
    Serial.println(command);
  }
  // reset
  int now = millis();
  int timeout = now + millisecondTimeout;
  byte result = PARSING;
  parseBuffer = "";

  // pass the command
  if( command )
  {
    _serialConnection->println(command);
    delay(1);
  } else {
    Serial.println("No command?");
  }
  
  while( result == PARSING )
  {
    // grab any chars available & add to buffer
    if (_serialConnection->available() > 0) {
      char ch = _serialConnection->read();
      if( ch ){
        if( debugOutput ){
          //Serial.print(ch);
        }
        parseBuffer += ch;
      }
    }

    // Setup exit conditions
    int l = parseBuffer.length()-1;
    int okLength = parseUntilWeSeeThis.length();
    String correctString = parseBuffer.substring( l-okLength, l);
    
    int errorLength = errorIfWeSeeThis.length();
    String errorString = parseBuffer.substring( l-errorLength, l);

    String busyIfWeSeeThis = "busy p...";
    int busyLength = busyIfWeSeeThis.length();
    String busyString = parseBuffer.substring( l-busyLength, l);

    // Test for exit conditions
    if( correctString == parseUntilWeSeeThis )
    {
      result = OK;
    } 
    else if( errorString == errorIfWeSeeThis )
    {
      result = ERROR;
    }
    else if( busyString == busyIfWeSeeThis )
    {
      result = BUSY;
    } 
    else if( now >= timeout )
    {
      result = TIMEOUT;
    }

    // all calls go through here, so we can test for certain things that may happen at any time
    // Note that the order of these are of utmost importance...
    if( parseBuffer.indexOf("WIFI DISCONNECT") != -1)
    {
      status = STATUS_DISCONNECTED;
    }
    if( parseBuffer.indexOf("WIFI CONNECTED") != -1)
    {
      status = STATUS_CONNECTED;
    }
    if( parseBuffer.indexOf("WIFI GOT IP") != -1)
    {
      status = STATUS_CONNECTED_W_IP;
    }
    
    // Keep looping
    now = millis();
    delay(1); // Take a short break before continuing or fast MCU's like the Teensy will choke
  }
  
  if( debugOutput ){
    Serial.print("Exiting with status: ");
    Serial.println( result );
    Serial.println("Buffer: ");
    Serial.println( parseBuffer );
  }

  if( result == OK )
  {
    return true;
  }
  return false;
}

// makes a status request and searches the buffer for the "STATUS:" string
uint8_t LittleESPServer::pollForStatus()
{
  bool statusTmp = parseSerial( AT_STATUS, 1000, "status:", "busy" );
  byte startPos = parseBuffer.indexOf("STATUS:");
  String tmp = parseBuffer.substring(startPos+7, startPos+8);
  uint8_t extractedStatus = tmp.toInt();
  //Serial.println( extractedStatus );
  return extractedStatus;
}

bool LittleESPServer::cwmode( cwmode_t usageMode )
{
  // First set to something else
  cwmode_t tempMode;
  if( usageMode == STANDALONE){
    tempMode = ACCESS_POINT;
  } else if( usageMode == ACCESS_POINT){
    tempMode = STANDALONE_AND_AP;
  } else {
    tempMode = STANDALONE;
  }

  String command = AT_SET_MODE;
  command += tempMode;
  command += " "; // toCharArray strips off the last char, so add a space for it to strip 
  char tmpcmd[80];
  command.toCharArray( tmpcmd, command.length() );
  parseSerial( tmpcmd, 1000, "OK", "ERROR" ); // ignore this result

  // Then set to what we really want
  command = AT_SET_MODE;
  command += usageMode;
  command += " ";
  char cmd[80];
  command.toCharArray( cmd, command.length() );
  return parseSerial( cmd, 1000, "OK", "ERROR" );
}

bool LittleESPServer::cipmode()
{
  parseSerial( AT_GET_DATA_MODE, 1000, "OK", "ERROR" );
  byte startPos = parseBuffer.indexOf("+CIPMODE:");
  String tmp = parseBuffer.substring(startPos+1, startPos+2);
  uint8_t extractedStatus = tmp.toInt();
  if( extractedStatus == 0)
  {
    return true;
  }
  else
  {
    return parseSerial( AT_SET_DATA_MODE, 1000, "OK", "ERROR" );
  }
}

bool LittleESPServer::cipmux()
{
  return parseSerial( AT_SET_MULTI_CONNECTION, 1000, "OK", "ERROR" );
}

bool LittleESPServer::cipserver( int portNumber )
{
  String command = AT_START_SERVER;
  command += portNumber;
  command += " ";
  char cmd[80];
  command.toCharArray( cmd, command.length() );
  return parseSerial( cmd, 2000, "OK", "ERROR" );
}

// AT+CWJAP="wifi-1","12345678"
bool LittleESPServer::wificonnect( String ssid, String password, uint8_t attempts )
{
  String command = AT_CONNECT_TO_WAN;
  command += "\"";
  command += ssid;
  command += "\",\"";
  command += password;
  command += "\" ";
  char cmd[80];
  command.toCharArray( cmd, command.length() );
  bool rez = parseSerial( cmd, 15000, "OK", "ERROR" );

  if( !rez && attempts > 1 )
  {
    delay(3000);
    rez = wificonnect( ssid, password, attempts-1 );
  }
  return rez;
}

// the ESP needs a couple seconds to start up...
void LittleESPServer::checkForInit()
{
  if(!initialized)
  {
    delay(2000);
    /*
    bool rez = parseSerial( AT_TEST, 1000, "OK", "ERROR" );
    if( rez )
    {
      Serial.println( "Unable to contact the ESP8266?" );
      Serial.println( "Please check http://flashgamer.com/arduino/LittleESPServer for hints on how to solve this." );
      return;
    }
    */
    initialized = true;
  }
}

