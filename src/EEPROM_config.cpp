//Dependencies
#include "defenitions.h"
#include <EEPROM.h>
#include <SafeStringReader.h>
#include <Preferences.h>

void initHostWifiConfig(bool clear) {

    Preferences preferences;

    const char* host = DFLT_HOSTNAME;
    const char* ssid = DFLT_WIFI_SSID;
    const char* pass = DFLT_WIFI_PASS;
    const char* ns = CONFIG_NS;

    preferences.begin(ns, false); //RW new namepace

    if (clear) {
        preferences.clear();
    }

    preferences.putString("host", host);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.putBool("init", true);
    preferences.putBool("defaultinit", true);

    preferences.end();
}

bool isHostWifiConfigured() {

    const char* ns = CONFIG_NS;
    bool result;
    Preferences preferences;
    
    preferences.begin(ns, true); //read-only mode
    result = preferences.getBool("init", false);
    preferences.end();
    
    return result;
}


bool isDefaultHostWifiConfig() {
    
    const char* ns = CONFIG_NS;
    bool result;
    Preferences preferences;
    
    preferences.begin(ns, true); //read-only mode
    result = preferences.getBool("defaultinit", false);
    preferences.end();
    
    return result;
}


void printHostWifiConfig() {

    const char* ns = CONFIG_NS;
    Preferences preferences;
    preferences.begin(ns, true); //read-only mode

    Serial.print("Wifi/Host configuration is: ");
    if  (!isHostWifiConfigured()) { //have we done initial setup? EEPROM OK? 
        Serial.println("NOT SET, or NVM is corrupted.");
    }
    else { 
        Serial.println("SET");
        Serial.println("---------------------------------------");
        Serial.println("Connect to this Wifi AP on your device:");
        Serial.println();
        Serial.print("  SSID: ");
        Serial.println(preferences.getString("ssid"));   
        Serial.print("  Password: ");
        Serial.println(preferences.getString("pass"));
        Serial.println();
        Serial.println("And open this address on your brower: ");
        Serial.println();
        Serial.print("  http://");
        Serial.print(preferences.getString("host"));
        Serial.println(".local");
        Serial.println();
        Serial.println("---------------------------------------");

        if (preferences.getBool("defaultinit", false)) {
            Serial.println("This is the DEFAULT Wifi and Host Configuration - change it ASAP!");
            Serial.println("---------------------------------------");
        }

        preferences.end();
    }
}


bool getHostWifiConfig( host_wifi_config_t* host_wifi_config) {

    const char* ns = CONFIG_NS;
    Preferences preferences;
    preferences.begin(ns, true); //read-only mode

    if  (!isHostWifiConfigured()) {
        preferences.end();
        return false;
    }
    else { 
        host_wifi_config->ssid = preferences.getString("ssid");   
        host_wifi_config->pass = preferences.getString("pass");
        host_wifi_config->host = preferences.getString("host");
        preferences.end();
        return true;
    }
}

bool setHostWifiConfig(host_wifi_config_t* host_wifi_config) {

    const char* ns = CONFIG_NS;
    Preferences preferences;
    preferences.begin(ns, false); //RW mode

    if  (!isHostWifiConfigured()) {
        preferences.end();
        return false;
    }
    else { 

        preferences.putBool("defaultinit", false);
        preferences.putString("ssid", host_wifi_config->ssid);   
        preferences.putString("pass", host_wifi_config->pass);
        preferences.putString("host", host_wifi_config->host);
        preferences.end();
        return true;
    }
}