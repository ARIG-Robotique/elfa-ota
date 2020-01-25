#include "define.h"

#include <FS.h>                   // This needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino

#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager

#include <RemoteDebug.h>

#include <ElegantOTA.h>

ESP8266WebServer server(80);
RemoteDebug Debug;
int lastValue;

void setup() {
    // Configuration des IOs //
    // --------------------- //

    // Inputs
    pinMode(GPIO_WIFI_RESET, INPUT_PULLUP);

    // Outputs


    // Peripheral configuration //
    // ------------------------ //

    // Serial
    Serial.begin(115200);
   
    // SPIFFS
    SPIFFS.begin();

    // WiFi

    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    
    // Reset saved settings
    if (digitalRead(GPIO_WIFI_RESET)) {
        wifiManager.resetSettings();
        Serial.println(" * RESET WIFI Settings");
        delay(5000);

        ESP.restart();
    }

    // Fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("ElfaOtaAP", WIFI_AP_PASSWORD);

    // If you get here you have connected to the WiFi
    String hostname = WiFi.hostname();
    IPAddress ip = WiFi.localIP();
    Serial.println(" * Connected to :");
    Serial.println("   -> SSID        : " + WiFi.SSID());
    Serial.println("   -> Hostname    : " + hostname);
    Serial.println("   -> IP          : " + ip.toString());

    MDNS.begin(hostname);
    MDNS.addService("telnet", "tcp", 23);

    // Initialize RemoteDebug
    Debug.begin(hostname);
    Debug.setResetCmdEnabled(true); // Enable the reset command
	Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
	Debug.showColors(true); // Colors

    // OTA Self
    ElegantOTA.begin(&server); 

    // Routing
    server.on("/", []() {
        File f = SPIFFS.open("/index.html", "r");
        server.send(200, "text/html", f.readString());
    });
  
    // Start WebServer
    server.begin();
    lastValue = millis();
}

void loop() {
    server.handleClient();
    Debug.handle();

    if (millis() - lastValue >= 1000) {
        lastValue = millis();
        debugI("Coucou");
    }

}