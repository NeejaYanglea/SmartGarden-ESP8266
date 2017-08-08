#include <SPI.h>
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

#define INDEX_HTML "<!DOCTYPE html><html><head><title>SmartGarden-ESP8266 Configuration</title><link rel='shortcut icon' href='{favicon}' /><style>body { font-family: Helvetica, Arial, sans-serif; font-size: 16px ;padding: 10px }</style></head><body><h1>SmartGarden-ESP8266 Configuration</h1><form method='GET' action='/conf' target='output_frame'><label><strong>Light start hour: </strong></label><br /><input type='number' min='0' max='23' step='1' pattern='([01]?[0-9]{1}|2[0-3]{1})' name='startTime' value='{startTime}' maxlength='2' size='1' style='font-size: 16px;' /><br /><br /><label><strong>Light end hour: </strong></label><br /><input type='number' min='0' max='23' step='1' pattern='([01]?[0-9]{1}|2[0-3]{1})' name='endTime' value='{endTime}' maxlength='2' size='1' style='font-size: 16px;' /><br /><br /><label><strong>Water hour: </strong></label><br /><input type='number' min='0' max='23' step='1' pattern='([01]?[0-9]{1}|2[0-3]{1})' name='waterTime' value='{waterTime}' maxlength='2' size='1' style='font-size: 16px;' /><br /><br /><strong>Password: </strong></label><br /><input type='password' maxlength='32' name='pwd' value='{pwd}' size='32' style='font-size: 16px;' /><br /><br /><input type='submit' value='Submit'></form><br /><br /><h3>Set a password </h3><p>Please set a password of max 32 characters. </p><form method='GET' action='/newpwd' target='output_frame'><label><strong>New Password: </strong></label><br /><input type='password' maxlength='32' name='newPwd' value='{newPwd}' size='32' style='font-size: 16px;' /><br /><br /><input type='submit' value='Submit'></form><br /><br /><iframe id='output_frame' name='output_frame' allowtransparency='true' width='350' height='80' frameBorder='0'>Browser not compatible</iframe></body></html>"
#define FAVICON "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAACHklEQVR4XoWT70tTURjHzz/in9CbueaYW2rlikCIftELdXOTpeJPlqNITV9LTWQzInMZc4Qvyk0NhYKoqDdtc7C1lFWv1FigsQiMxr6e5xzP9d6gPPDc59z7nOfz/DqXlctl0CJNUqlUxDtpZVN7ZXv3KYHO+1ZxjoknX5mvrzWQAirb23wc/TN29M7YhPQ/ssM9US0BKsq1e1ZhGI978H49oUWffnlDOF2POTD2/AxGl50YnG9Ayx2TEdB614RbT0/h5pM69EVqhVPy8wuhhxZOC+fhRCO6Z23oiFjgUgCVJhG7o7UYisvDgbk6+KN2DMZO4PaSEwM8g46HVvQ8dojo/ukmYw9agia4gtXoitjE4ZHFRpFuIOqAN3wcbZNmtE9ZhLNKX+sB9WHnZ5EbzKBSvGELPCEz+mbt8HGnZOEVfOF6eIJWrCTntAnRVLQmEu33nz2MxFwiwpvsEnoenIQvJMelmq3f0zcxxn8ZV3MxuEM1/weoC0SnCsPHsOFjyB3Ihpch08SQOsuwphP9xdIuEgFKqTgylxh23AzfW6XevszlAkOR63UOI5B+Gd+45aOHoTTAsNsr9berDJvnJSTNnYsLk0aAvgTaZylqlwTsEuDKISB1jqH8q2T4TwxTIMCXcScK7dJZAbZ5BlTCGgf83XQDgIw/PjxDrk2mr0oo8vS3LvJmHgVQ5VAZeT4BkvTBFGgS2eYqkYH+V98HQY0eaNw1dPsAAABRdEVYdENvbW1lbnQAQ29weXJpZ2h0IElOQ09SUyBHbWJIICh3d3cuaWNvbmV4cGVyaWVuY2UuY29tKSAtIFVubGljZW5zZWQgcHJldmlldyBpbWFnZbaaaaYAAAA4dEVYdENvcHlyaWdodABDb3B5cmlnaHQgSU5DT1JTIEdtYkggKHd3dy5pY29uZXhwZXJpZW5jZS5jb20pTs6ZTgAAAFp6VFh0Q29tbWVudAAAeJxzzi+oLMpMzyhR8PRz9g8KVnDPTfJQ0CgvL9fLTM7PS60oSC3KTM1LTtVLzs/VVNBVCM3LyUxOzStOTVEoKEoty0wtV8jMTUxPBQC4jxoknLyY4wAAAEF6VFh0Q29weXJpZ2h0AAB4nHPOL6gsykzPKFHw9HP2DwpWcM9N8lDQKC8v18tMzs9LrShILcpMzUtO1UvOz9UEAH02EGgc3eaPAAAAAElFTkSuQmCC"

#define period 1000*10
#define DEBUG false

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

extern "C" {
#include "user_interface.h"
}

os_timer_t myTimer;
int timeSyncCounter = 0;
#define TIME_SYNC_SECONDS 3600

unsigned long localTime = 0;

bool eepromReadRequired = true;
int startTime, endTime, waterTime, day;
int isPasswordSet;
char pwd[33];
char server_pwd[33];
char new_pwd[33];

//TODO set from server web
int GMT = 2; 

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
void sendDataToServer();
void timerCallback(void *pArg);
void readDataFromEeprom(int& startTime, int& endTime, int& waterTime);
void readIsPasswordSetFromEeprom(int& isPasswordSet);
void readPasswordFromEeprom();
void resetPassword();
void writeIsPasswordSetOnEeprom();
void resetPasswordFromButton();

//starts http server on port 8080
ESP8266WebServer server(8080);

int getHour(long epoch);
bool inRange(int currentHour, int startTime, int endTime);
bool waterToday(int currentDate, int savedDate);
int getDay(long epoch);
void writeDayOnEeprom(int day);
void readDayFromEeprom(int &day);

void setup() {
  // Moisture sensor in water tank
  pinMode(D0, INPUT);
  // Moisture sensor in pots
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  // Led input
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
  // Water tank input
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  //Password reset button
  pinMode(D6, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D6), resetPasswordFromButton, CHANGE);
  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 1000, true);
  
  /* SETUP WIFI CONNECTION THROUGH WIFIMANAGER */
  WiFiManager wifiManager;
  //wifiManager.resetSettings();    //Uncomment this to wipe WiFi settings from EEPROM on boot.  Comment out and recompile/upload after 1 boot cycle.
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,94), IPAddress(192,168,1,94), IPAddress(255,255,255,0));
  wifiManager.autoConnect("SmartGarden-ESP8266", "smartgarden");
  //if you get here you have connected to the WiFi
  Serial.println("Connected");

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  delay(500);

  if (DEBUG) Serial.println("Starting UDP");
  udp.begin(localPort);
  if (DEBUG) Serial.print("Local port: ");
  if (DEBUG) Serial.println(udp.localPort());

  setTimeFromNTP(0);

  //check if password is set and written on EEPROM
  readIsPasswordSetFromEeprom(isPasswordSet);
  if (DEBUG) Serial.println("isPasswordSet: " + String(isPasswordSet));
  if(isPasswordSet != 1){
    resetPassword();
  }
  readIsPasswordSetFromEeprom(isPasswordSet);
  if (DEBUG) Serial.println("isPassword RESET: " + String(isPasswordSet));
  

  if(isPasswordSet){
    readPasswordFromEeprom();
      if (DEBUG) {
        Serial.println("PASSWORD FROM EEPROM: ");
        Serial.println(pwd);
      }
  }
  // Server configuration 
  
  server.on("/conf", [](){
    // saves user data to the eeprom
    // TODO verify user input
    strncpy(server_pwd, server.arg("pwd").c_str(), 33);  
    if (DEBUG) {
        Serial.println("Password from server: ");
        Serial.println(server_pwd);
      }
    if ((isPasswordSet && strcmp(server_pwd, pwd) == 0) || !isPasswordSet) {
      if (DEBUG) {
        Serial.print("Password is set: ");
        Serial.print(isPasswordSet);
        Serial.println(" and it is correct.");
      }
      
      startTime = server.arg("startTime").toInt();
      endTime = server.arg("endTime").toInt();
      waterTime = server.arg("waterTime").toInt();    
      
      EEPROM.begin(512);
      EEPROM.put(0, startTime);
      EEPROM.put(sizeof(startTime), endTime);
      EEPROM.put(sizeof(startTime) + sizeof(endTime), waterTime);
      EEPROM.commit();
      EEPROM.end();
  
      eepromReadRequired = true;
      
      server.send(200, "text/plain", "Saved: " + String(startTime) + " " + String(endTime) + " " + String(waterTime));
    }
    else {
      if (DEBUG) {
        Serial.print("Password is set: ");
        Serial.print(isPasswordSet);
        Serial.println(" but it is not correct.");
      }
      server.send(200, "text/plain", "Wrong Password.");
    }
  });

  server.on("/newpwd", [](){
    // saves user set password to the eeprom
    if (!isPasswordSet) {
      
      strncpy(new_pwd, server.arg("newPwd").c_str(), 33);  
      if (DEBUG) {
          Serial.println("New password from server: ");
          Serial.println(new_pwd);
        }   
        
      EEPROM.begin(512);
      EEPROM.put(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime) + sizeof(day) + sizeof(isPasswordSet), new_pwd);
      EEPROM.commit();
      EEPROM.end();
      
      writeIsPasswordSetOnEeprom();
    
      readIsPasswordSetFromEeprom(isPasswordSet);
      if (DEBUG) Serial.println("isPasswordSet: " + String(isPasswordSet));
      
      readPasswordFromEeprom();
      if (DEBUG) {
        Serial.println("Password from eeprom: ");
        Serial.println(pwd);
      }
      server.send(200, "text/plain", "Password Saved.");
    }
    else {
      server.send(200, "text/plain", "Password Already Set, press the balck button on Smart Garden to reset.");
    }
        
  });

  server.on("/", [](){
    String html_home_page = INDEX_HTML;

    readDataFromEeprom(startTime, endTime, waterTime);

    html_home_page.replace("{favicon}", FAVICON);
    html_home_page.replace("{startTime}", String(startTime));
    html_home_page.replace("{endTime}", String(endTime));
    html_home_page.replace("{waterTime}", String(waterTime));

    server.send(200, "text/html", html_home_page);
  });

  server.begin();
}

void loop() {  
  if (DEBUG)   delay(5000);
  server.handleClient();
  
  if(timeSyncCounter >= TIME_SYNC_SECONDS) {
    setTimeFromNTP(1);
    timeSyncCounter = 0;
    if(DEBUG) Serial.println("Time sync from NTP");
  }
  //if(DEBUG) Serial.print("Time: ");
  //if(DEBUG) Serial.println(localTime);

  //TODO send all info to domoticz
  //sendDataToServer();

  if (eepromReadRequired) {
    readDataFromEeprom(startTime, endTime, waterTime);    
    eepromReadRequired = false;
  }

  if(DEBUG) Serial.print("startTime, endTime, waterTime: ");
  if(DEBUG) Serial.print(startTime);
  if(DEBUG) Serial.print(" ");
  if(DEBUG) Serial.print(endTime);
  if(DEBUG) Serial.print(" ");
  if(DEBUG) Serial.println(waterTime);

  // LED control
  if (inRange(getHour(localTime), startTime, endTime)){
      if (DEBUG) Serial.println("In range and led off");
      digitalWrite(D3, LOW);
   } else {
      if (DEBUG) Serial.println("Not in range and led on");
      digitalWrite(D3, HIGH);
  }

  //WATER control
  readDayFromEeprom(day);
  
  if (DEBUG) Serial.print("Current Day: ");
  if (DEBUG) Serial.println(getDay(localTime));
  if (DEBUG) Serial.print("Saved Day: ");
  if (DEBUG) Serial.println(day);
  if (DEBUG) Serial.print("Water today? ");
  if (DEBUG) Serial.println(waterToday(getDay(localTime), day));
  if (DEBUG) Serial.print("Water tank and ground? ");
  if (DEBUG) Serial.println(String(digitalRead(D0)) + " " + String(digitalRead(D1)) + " " + String(digitalRead(D2)));
  
  if (waterTime == getHour(localTime) && waterToday(getDay(localTime), day)) {
    // if (tank not dry && pot1 dry && pot2 dry)
    if (!digitalRead(D0) && digitalRead(D1) && digitalRead(D2)){
      if (DEBUG) Serial.println("Watering... ");
      digitalWrite(D4, LOW);
      delay(13000);
      digitalWrite(D4, HIGH);
      if (DEBUG) Serial.println("Watered. ");
      writeDayOnEeprom(getDay(localTime));
    }
  }

  delay(100);
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
  
    localTime = epoch + (3600 * GMT);
  }
}

void updateTimeFromInternalClock(const int seconds) {
  localTime += seconds;
  //if (DEBUG) Serial.println(localTime);
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

void sendDataToServer(){
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

   http.end();
}

void timerCallback(void *pArg) {
  updateTimeFromInternalClock(1);
  timeSyncCounter++;
}

int getHour(long epoch){
  if (DEBUG) {
    Serial.print ("getHour: ");
    Serial.println ((epoch  % 86400L) / 3600);
  }
  return (epoch  % 86400L) / 3600;
}

bool inRange(int currentHour, int startTime, int endTime){
  if (startTime >= endTime){
    return (currentHour >= startTime && currentHour <= 23) || (currentHour >= 0 && currentHour < endTime);
  }
  return currentHour >= startTime && currentHour < endTime;
}

void readDataFromEeprom(int& startTime, int& endTime, int& waterTime) {
  EEPROM.begin(512);
  EEPROM.get(0, startTime);
  EEPROM.get(sizeof(startTime), endTime);
  EEPROM.get(sizeof(startTime) + sizeof(endTime), waterTime);
  EEPROM.end();
}

void writeDayOnEeprom(int day){
  EEPROM.begin(512);
  EEPROM.put(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime), day);
  EEPROM.commit();
  EEPROM.end();
}

void readDayFromEeprom(int &day){
  EEPROM.begin(512);
  EEPROM.get(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime), day);
  EEPROM.end();
}

void readIsPasswordSetFromEeprom(int& isPasswordSet){
  EEPROM.begin(512);
  EEPROM.get(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime) + sizeof(day), isPasswordSet);
  EEPROM.end();
}

void readPasswordFromEeprom() {
  EEPROM.begin(512);
  EEPROM.get(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime) + sizeof(day) + sizeof(isPasswordSet), pwd);
  EEPROM.end();
}

void resetPassword(){
  EEPROM.begin(512);
  EEPROM.put(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime) + sizeof(day), 0);
  EEPROM.commit();
  EEPROM.end();
}

void writeIsPasswordSetOnEeprom(){
  EEPROM.begin(512);
  EEPROM.put(sizeof(startTime) + sizeof(endTime) + sizeof(waterTime) + sizeof(day), 1);
  EEPROM.commit();
  EEPROM.end();
}

void resetPasswordFromButton(){
  resetPassword();
  readIsPasswordSetFromEeprom(isPasswordSet);
  if (DEBUG) {
    Serial.print("Resetting Password. isPasswordSet: ");
    Serial.println(isPasswordSet);
  }
}

bool waterToday(int currentDate, int savedDate){
  return currentDate != savedDate;
}

int getDay(long epoch){
  return (epoch / 86400L);
}

