#include <SPI.h>
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include "WiFiSettings.h"
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

#define period 1000*10
#define DEBUG true

#define D0 16 
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define D9 3
#define D10 1
#define SD2 9
#define SD3 10

const char* ssid = WIFI_SSID;
const char* pass = WIFI_PASSWORD;
int status = WL_IDLE_STATUS;     // the Wifi radio's status

// Domoticz
int idx = 2;
char* ipPort = "192.168.1.93:8080";

// NTP
unsigned int localPort = 2390;      // local port to listen for UDP packets
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


unsigned long sendNTPpacket(IPAddress& address);
void setTimeFromNTP(const int update);
void updateTimeFromInternalClock(const int seconds);

unsigned long localTime = 0;

void setup() {  
  pinMode(D0, OUTPUT);
  //Initialize serial and wait for port to open:
  EEPROM.begin(512);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  /* SETUP WIFI CONNECTION THROUGH WIFIMANAGER */
  WiFiManager wifiManager;
  //wifiManager.resetSettings();    //Uncomment this to wipe WiFi settings from EEPROM on boot.  Comment out and recompile/upload after 1 boot cycle.
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,94), IPAddress(192,168,1,94), IPAddress(255,255,255,0));
  wifiManager.autoConnect("NodeMCU");
  //if you get here you have connected to the WiFi
  Serial.println("Connected");

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  delay(5000);

  if (DEBUG) Serial.println("Starting UDP");
  udp.begin(localPort);
  if (DEBUG) Serial.print("Local port: ");
  if (DEBUG)Serial.println(udp.localPort());

  setTimeFromNTP(0);
}

void loop() {  
  updateTimeFromInternalClock(4);
  setTimeFromNTP(1);
  
  int data = (1000 - analogRead(17))/7.5;
  if (DEBUG) {
    Serial.print("Sending data to server: ");
    Serial.print(data);
    Serial.println();
  }
  HTTPClient http;
  char msg[95];
  snprintf(msg, 95, "http://%s/json.htm?type=command&param=udevice&idx=%d&nvalue=%d&svalue=%d", ipPort, idx, data, data);
  if (DEBUG) Serial.println(msg);
  http.begin(msg);

  int httpCode = http.GET();
  if (DEBUG) {
    Serial.print("http code: ");
    Serial.print(httpCode);
    Serial.println();
  }

  digitalWrite(D0, LOW);
  delay(1000);
  http.end();
  // check the network connection once every 10 seconds:
  digitalWrite(D0, HIGH);
  delay(1000);
}

void setTimeFromNTP(const int update) {
  WiFi.hostByName(ntpServerName, timeServerIP); 
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();

  if(update == 0) {
    while (!cb) {
      if (DEBUG)Serial.println("no packet yet");
    }
  }
  if((update == 1 && cb) || update == 0) {
    if (DEBUG) Serial.print("packet received, length=");
    if (DEBUG) Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
  
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    if (DEBUG) Serial.print("Seconds since Jan 1 1900 = " );
    if (DEBUG) Serial.println(secsSince1900);
  
    // now convert NTP time into everyday time:
    if (DEBUG) Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    
    // print the hour, minute and second:
    if (DEBUG) Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    if (DEBUG) Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    if (DEBUG) Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      if (DEBUG) Serial.print('0');
    }
    if (DEBUG) Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    if (DEBUG) Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      if (DEBUG) Serial.print('0');
    }
    if (DEBUG) Serial.println(epoch % 60); // print the second
  
    localTime = epoch;
  }
}

void updateTimeFromInternalClock(const int seconds) {
  localTime += seconds;
  if (DEBUG) Serial.println(localTime);
}

unsigned long sendNTPpacket(IPAddress& address){
  if (DEBUG) Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


