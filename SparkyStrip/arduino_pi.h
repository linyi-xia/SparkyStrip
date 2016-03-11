#ifndef ARDUINO_PI
#define ARDUINO_PI

#include "settings.h"
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include "utility/debug.h"

//The stuff down there needs to be global

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
Adafruit_CC3000_Client pi_connection;

const uint32_t DHCP_TIMEOUT = 300000;
const bool STATUS_MESSAGES = true;
const uint32_t DATA_LEN = 40;
//enter integer IP for the server

// Forward declarations
void displayDriverMode(void);
uint16_t checkFirmwareVersion(void);
void displayMACAddress(void);
bool displayConnectionDetails(void);
bool write_data(float data[]);
void delay_loop(void);

/*--------------------------------------
 * ---SETUP---
 * setup_cc3000() : returns true for successful wifi connection, else false
 * cc3000.checkConnected() : returns true if connected to wifi, else false, should check connection occasionally
 * tcp_connect(IP, PORTNO) : returns true if successful connection to socket, else false
 *
 * ---SENDING DATA---
 * write_data(10 element float array) : sends a 10 element float array to pi
 *
 * ---CLOSING---
 * close_connection()
 ---------------------------------------*/


/*
void setup(void)
{

  Serial.begin(115200);
  if(!setup_cc3000(STATUS_MESSAGES))
  {
    Serial.println(F("Setup error."));
    return;
  }
  Serial.println(F("Setup success."));
  if (cc3000.checkConnected())
    Serial.println(F("Connected to network for TCP attempt."));
  if(!tcp_connect(IP, PORTNO))
  {
    Serial.println(F("TCP connect error. First attempt."));
    if (cc3000.checkConnected())
      Serial.println(F("Connected to network for TCP attempt."));
    if (!tcp_connect(IP, PORTNO))
    {
      Serial.println(F("TCP connect error. Second attempt."));
      return;
    }
  }
  Serial.println(F("TCP connect success."));

  //Keep the rest of the stuff in this file, it's needed for the WIFI.
  //----------------------------------------------------
  //Jeff's code goes here, use write_data to send a string.
  float your_string[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  write_data(your_string);

  Serial.println(F("Write data success."));
  //----------------------------------------------------


  close_connection();
  Serial.println(F("Connection close success."));
}
*/


void delay_loop(void)
{
  delay(1000);
}

bool setup_cc3000()
{
  if(Serial)
  {
    displayDriverMode();
    Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
    Serial.println(F("Initialising the CC3000 ..."));
  }

  if (!cc3000.begin())
  {
    if(Serial)
      Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    //while(1);
    return false;
  }

  uint16_t firmware = checkFirmwareVersion();
  if (firmware < 0x113) {
    if(Serial)
      Serial.println(F("Wrong firmware version!"));
    //for(;;);
    return false;
  }

  if(Serial)
  {
    displayMACAddress();
    Serial.println(F("Deleting old connection profiles"));
  }
  if (!cc3000.deleteProfiles()) {
    if(Serial)
      Serial.println(F("Failed!"));
    //while(1);
    return false;
  }

  /* Attempt to connect to an access point */
  const char *ssid = WLAN_SSID;             /* Max 32 chars */
  if(Serial)
    Serial.print(F("Attempting to connect to ")); Serial.println(ssid);

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
    Serial.println(F("Connected!"));
    Serial.println(F("Request DHCP"));
  }

  uint32_t DHCP_time = 0;
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
    DHCP_time += 100;
    if (DHCP_time >= DHCP_TIMEOUT)
      return false;
  }

  if (Serial)
  {
    while (! displayConnectionDetails()) {
      delay(1000);
    }
    Serial.println(F("IP to connect to : "));
    cc3000.printIPdotsRev(IP);
    Serial.println();
  }

  if (cc3000.checkConnected())
  {
    if(Serial)
      Serial.println(F("Succesfully connected to internet."));
    return true;
  }
}

bool tcp_connect (uint32_t tcp_ip, uint16_t tcp_port)
{
  pi_connection = cc3000.connectTCP(tcp_ip, tcp_port);
  if (pi_connection.connected())
    return true;
  else
    return false;
}


bool write_data(float data[])
{


  while (!cc3000.checkConnected())
  {
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  }
  while(!pi_connection.connected() )
 #ifdef USE_UDP
      pi_connection = cc3000.connectUDP(IP, PORTNO);
 #else   
      pi_connection = cc3000.connectTCP(IP, PORTNO);
 #endif
      
  pi_connection.write(data, DATA_LEN);
 #ifdef USE_UDP
  pi_connection.close();
 #endif
  return true;
}

void close_connection()
{
  if (cc3000.checkConnected())
  {
    float termination_string[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pi_connection.write(termination_string, DATA_LEN);
  }
  pi_connection.close();
  cc3000.disconnect();
}


/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;

#ifndef CC3000_TINY_DRIVER
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];

  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33];

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);

    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}




#endif // ARDUINO_PI

