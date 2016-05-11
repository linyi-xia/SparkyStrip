#ifndef ARDUINO_MSQL
#define ARDUINO_MSQL

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <MySQL_Encrypt_Sha1.h>
#include <MySQL_Packet.h>

#include <common.h>
#include <watchdog.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11



const char* _WLAN_SSID;        // cannot be longer than 32 characters!
const char* _WLAN_PASS;

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
int _WLAN_SECURITY;

//IPAddress server_addr(72,219,144,187);  // IP of the MySQL *server* here
IPAddress _SERVER_ADDR;

char user[12];                          // MySQL user login username generated from mac address
char password[] = "sparkystrip_device"; // MySQL user login password


Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
Adafruit_CC3000_Client WIFI_CLIENT;        // For WiFi connections
MySQL_Connection MYSQL_CONNECTION((Client *)&WIFI_CLIENT);
MySQL_Cursor *MYSQL_CURSOR;


const uint32_t DHCP_TIMEOUT = 50000;

void pass_wifi_values(const char* ssid, const char* password, int wlan_security, const IPAddress& ip){
  _WLAN_SSID = ssid;
  _WLAN_PASS = password;
  _WLAN_SECURITY = wlan_security;
  _SERVER_ADDR = ip;
}

bool setup_cc3000()
{
  digitalWrite(RED_LED, LED_OFF);
  digitalWrite(GREEN_LED, LED_OFF);
  digitalWrite(AMBER_LED, LED_ON);
  watchdogReset();
  if (!cc3000.begin())
  {
    if(Serial)
      Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    go_error(500);
  }
  if (!cc3000.deleteProfiles()) {
    if(Serial)
      Serial.println(F("Failed to deleting old connection profiles!"));
    go_error(125);
  }
  /* Attempt to connect to an access point */
  if(Serial){
    Serial.print(F("Connect to ")); 
    Serial.print(_WLAN_SSID);
    Serial.print(F("... ")); 
  }
  /* NOTE: Secure connections are not available in 'Tiny' mode!
     By default connectToAP will retry indefinitely, however you can pass an
     optional maximum number of retries (greater than zero) as the fourth parameter.
  */
  digitalWrite(RED_LED, LED_ON);
  while (!cc3000.connectToAP(_WLAN_SSID, _WLAN_PASS, _WLAN_SECURITY)) {
    if(Serial){
      Serial.print(F("Failed! Cannot connect to "));
      Serial.print(_WLAN_SSID);
      Serial.println(F("!\nCannot continue, check if SSID is correct!"));
    }
    //while(1);
    go_error(250);
  }
  if(Serial)
  {
    Serial.print(F("Connected!\nRequesting DHCP"));
  }
  uint32_t DHCP_time = 160;
  bool stat = true;
  digitalWrite(RED_LED, LED_OFF);
  while (!cc3000.checkDHCP())
  {
    watchdogReset();
    delay(1000);
    stat ^= true;
    if(stat)
      digitalWrite(RED_LED, LED_ON);
    else
      digitalWrite(RED_LED, LED_OFF);
    if(Serial)
    {
      Serial.print('.');
      if( DHCP_time % 1000 == 0 )
        Serial.println();
    }
    DHCP_time += 1000;
    if (DHCP_time >= DHCP_TIMEOUT){
      if(Serial)
        Serial.println(F("Failed!"));
      return false;
    }
  }
  digitalWrite(RED_LED, LED_OFF);
  if (cc3000.checkConnected())
  {
    union{
      uint16_t parts[3];
      uint8_t address[6];
    }mac;
    cc3000.getMacAddress(mac.address);
    uint16_t devID = mac.parts[0]^mac.parts[1]^mac.parts[2];
    sprintf(user, "d%u", devID);
    if(Serial){
      char hex[5];
      Serial.print(F("\nSuccesfully connected to internet.\n\nDeviceID: "));
      Serial.println(user);
    }
    return true;
  }
  return false;
}


bool mysql_connect() 
{
  while(!cc3000.checkConnected())
    setup_cc3000();
  if(Serial){
    Serial.print(F("Connecting the MySQL server at "));
    Serial.print(_SERVER_ADDR);
    Serial.print(":");
    Serial.println(3306);
  }
  if (MYSQL_CONNECTION.connect(_SERVER_ADDR, 3306, user, password)) {
    delay(1000);
    // Initiate the query class instance
    if(Serial)
      Serial.println(F("Connected!"));
    MYSQL_CURSOR = new MySQL_Cursor(&MYSQL_CONNECTION);
    if(Serial)
      Serial.println(F("Ready to push data!"));
    digitalWrite(GREEN_LED, LED_ON);
    digitalWrite(AMBER_LED, LED_OFF);
    return true;
  }
  else if(Serial)
    Serial.println(F("Connection failed."));
  return false;
}

bool send_mysql(float data[])
{
    digitalWrite(RED_LED, LED_ON);
    char call[256];
    const char call_template[]= "CALL SparkyStrip.pushData(%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f);";
    sprintf(call, call_template, data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13]);
    // Execute the call
    while(!MYSQL_CONNECTION.connected()){
      MYSQL_CONNECTION.close();
      mysql_connect();
    }
    MYSQL_CURSOR->execute(call);
    if(Serial){
      int last = strlen(call);
      call[last-2]='\0';
      Serial.println(call+26);
    }
    digitalWrite(RED_LED, LED_OFF);
}

#endif // ARDUINO_MSQL

