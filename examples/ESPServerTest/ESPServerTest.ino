#include <SPI.h>
#include <Ethernet.h>
#include "ESPServer.h"

// Note: Smaller Arduino boards such as the UNO should typically use SoftwareSerial
// instead. Look here for instructions https://www.arduino.cc/en/Reference/SoftwareSerial
ESPServer server = ESPServer( &Serial1 );

String WLAN_SSID = "YourSSID";        // cannot be longer than 32 characters!
String WLAN_PASS = "YourPassword";

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200); // Note: ESP modules that don't have updated firmware (1.1 or higher) will fail to init at this speed
  
  // Test that we can communicate with the ESP8266 module
  int rez = server.test();
  if(!rez)
  {
    Serial.println("test() failed. Check your connections.");
    while(1);
  }
  Serial.println("test() was successful");
  
  // Setup as one of the following:
  // STANDALONE - Access your ESP from the network
  // ACCESS_POINT - Access the ESP directly and get your own IP from it
  // STANDALONE_AND_AP - Enable both ways to access the ESP module
  Serial.print("begin() success? ");
  rez = server.begin(STANDALONE_AND_AP,80);
  Serial.println(rez);
  
  // Connect it to a Wifi access point
  Serial.print("connectToAP() success? ");
  server.connectToAP( WLAN_SSID, WLAN_PASS );
  Serial.println(rez);
  
  // Check status
  /*
  0 = STATUS_UNKNOWN
  1 = STATUS_SCANNING
  2 = STATUS_CONNECTED
  3 = STATUS_CONNECTING
  4 = STATUS_DISCONNECTED
  5 = STATUS_INITIALIZING
  6 = STATUS_CONNECTED_W_IP
  7 = CIPMODE_ERROR
  8 = CIPMUX_ERROR
  9 = CIPSERVER_ERROR
  10 = CWMODE_ERROR
  11 = WIFI_CONNECT_ERROR
  
  0-6 indicates normal operation
  7-11 indicates what part of the init process that failed
  */
  Serial.print("getStatus(): ");
  rez = server.getStatus();
  Serial.println(rez);
  
  IPAddress ipAddress = {0,0,0,0};

  Serial.print("AP ip: ");
  server.getAPAddress( ipAddress );  
  Serial.println(ipAddress);
  
  Serial.print("Standalone ip: ");
  ipAddress = {0,0,0,0};
  //while( ipAddress[0] == 0 )
  {
    server.getStandaloneAddress( ipAddress );
    Serial.println(ipAddress);
    delay(1000);
    int serverStatus = server.pollForStatus();
    Serial.print("serverStatus ");
    Serial.println(serverStatus);
    delay(1000);
  }
}

boolean parseItYourself = false;
char theURL[255] = {};

void loop(){
  while( server.available() )
  {
    int clientID = server.clientID();
    // calc time
    
    
    // to parse the request yourself, read back the buffer
    if( parseItYourself )
    {
      char ret = server.read();
      Serial.print(ret);
      // this will just read out whatever the ESP sends
    } else if( clientID == 0 ){
      // We're passing in a char array that will be filled with the URL requested
      server.pathRequested( theURL );
      // Build the return page
      String baseText = "Heya! Thanks for requesting ";
      baseText += theURL;
      baseText += "<br>We've been up and running for ";
      baseText += makeTimeString();
      baseText += "!";
      String page = makeBasePage("Hello world!", baseText);
      server.send( clientID, page );
    }
  }
  
  delay(20);
}

String makeBasePage( String title, String content){
  String page = "";
  page += "HTTP/1.1 200 OK\r\n";
  page += "Content-Type: text/html\r\n";
  page += "Connection: close\r\n";
  page += "Server: BitmartSign\r\n";
  page += "Refresh: 5\r\n";
  // Send an empty line to signal start of body.
  page += "\r\n";
  // Now send the response data.
  page += "<html><head><title>";
  page += title;
  page += "</title>\r\n";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n";
  page += "</head><body>\r\n";
  page += content;
  page += "</body><html>\r\n\r\n";
  return page;
}

String makeTimeString(){
  int temp = 0;
  int seconds = 0;
  int minutes = 0;
  int hours = 0;
  int days = 0;
  
  temp = millis();
  days = temp/1000/60/60/24;
  temp -= days*1000*60*60*24;
  hours = temp/1000/60/60;
  temp -= hours*1000*60*60;
  minutes = temp/1000/60;
  temp -= minutes*1000*60;
  seconds = temp/1000;
  
  String baseText = "";
  if( days > 0 ){
    baseText += days;
    baseText += " days ";
  }
  if( hours > 0 ){
    baseText += hours;
    baseText += " hours ";
  }
  if( minutes > 0 ){
    baseText += minutes;
    baseText += " minutes ";
  }
  if( seconds > 0 ){
    baseText += seconds;
    baseText += " seconds ";
  }
  return baseText;
}
