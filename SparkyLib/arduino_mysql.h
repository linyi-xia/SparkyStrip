#ifndef ARDUINO_MSQL
#define ARDUINO_MSQL

#include "settings.h"
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <MySQL_Encrypt_Sha1.h>
#include <MySQL_Packet.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11


Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
Adafruit_CC3000_Client client;        // For WiFi connections
MySQL_Connection conn((Client *)&client);
MySQL_Cursor *cursor;


const uint32_t DHCP_TIMEOUT = 50000;

bool setup_cc3000()
{
  if (!cc3000.begin())
  {
    if(Serial)
      Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    while(1);
  }
  if (!cc3000.deleteProfiles()) {
    if(Serial)
      Serial.println(F("Failed to deleting old connection profiles!"));
    while(1);
  }
  /* Attempt to connect to an access point */
  const char *ssid = WLAN_SSID;             /* Max 32 chars */
  if(Serial){
    Serial.print(F("Connect to ")); 
    Serial.print(ssid);
    Serial.print(F("... ")); 
  }
  /* NOTE: Secure connections are not available in 'Tiny' mode!
     By default connectToAP will retry indefinitely, however you can pass an
     optional maximum number of retries (greater than zero) as the fourth parameter.
  */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    if(Serial)
      Serial.println(F("Failed!"));
    //while(1);
    return false;
  }

  if(Serial)
  {
    Serial.print(F("Connected!\nRequesting DHCP"));
  }

  uint32_t DHCP_time = 160;
  while (!cc3000.checkDHCP())
  {
    delay(1000); // ToDo: Insert a DHCP timeout!
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
    Serial.print(server_addr);
    Serial.print(":");
    Serial.println(3306);
  }
  if (conn.connect(server_addr, 3306, user, password)) {
    delay(1000);
    // Initiate the query class instance
    if(Serial)
      Serial.println(F("Connected!"));
    cursor= new MySQL_Cursor(&conn);
    if(Serial)
      Serial.println(F("Ready to push data!"));
    return true;
  }
  else if(Serial)
    Serial.println(F("Connection failed."));
  return false;
}

bool send_mysql(float data[])
{
    char call[256];
    const char call_template[]= "CALL SparkyStrip.pushData(%f,%f,%f,%f,%f,%f,%f,%f,%f);";
    sprintf(call, call_template, data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
    // Execute the call
    while(!conn.connected()){
      conn.close();
      mysql_connect();
    }
    cursor->execute(call);
    if(Serial){
      int last = strlen(call);
      call[last-2]='\0';
      Serial.println(call+26);
    }
}

#endif // ARDUINO_MSQL

