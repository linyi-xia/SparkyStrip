#ifndef SETTINGS_H
#define SETTINGS_H

#include "Arduino.h"
#include <IPAddress.h>


#define WLAN_SSID       "JeffnTahe"        // cannot be longer than 32 characters!
#define WLAN_PASS       "1loveTahereh!"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//The stuff down there needs to be global
//IPAddress server_addr(72,219,144,187);  // IP of the MySQL *server* here
IPAddress server_addr(169,234,209,144);

char user[12];                          // MySQL user login username generated from mac address
char password[] = "sparkystrip_device"; // MySQL user login password




#endif //SETTINGS_H

