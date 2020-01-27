#include "define.h"

#include <FS.h>  
#include <ArduinoJson.h>                

#include <ESP8266WiFi.h>          

#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>      

#include <RemoteDebug.h>

#include <ElegantOTA.h>

// Prototype des fonctions

// Business
void heartBeat();
void wifiReset();

// HTTP
void httpError();
void httpRoot();
void httpGetRobot();
void httpGetCapteurs();
void httpSetIO();
void httpGetServos();
void httpSetServos();
void httpResetESP();
void httpRestartESP();

// Déclaration des objets
ESP8266WebServer server(80);
RemoteDebug Debug;
WiFiManager wifiManager;

// Variables locales
int heartTimePrec, heartTime;
boolean heart;

// Initialisation de la puce
void setup() {
    // Configuration des IOs //
    // --------------------- //

    // Inputs
    pinMode(GPIO_WIFI_RESET, INPUT_PULLUP);

    // Outputs
    pinMode(GPIO_HEARTBEAT, OUTPUT);
    pinMode(GPIO_RESET_STM32, OUTPUT);
    pinMode(GPIO_BOOT0_STM32, OUTPUT);

    // Configuration des périphériques //
    // ------------------------------- //

    // Serial
    Serial.begin(115200);
   
    // SPIFFS
    SPIFFS.begin();

    // WiFi

    // Fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("ElfaOtaAP", WIFI_AP_PASSWORD);

    // If you get here you have connected to the WiFi
    MDNS.begin(WiFi.hostname());
    MDNS.addService("telnet", "tcp", 23);

    // Initialize RemoteDebug
    Debug.begin(WiFi.hostname());
    Debug.setResetCmdEnabled(true); // Enable the reset command
	Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
	Debug.showColors(true); // Colors

    // OTA Itself !!
    ElegantOTA.begin(&server); 

    // Routing
    server.onNotFound(httpError);
    server.on("/", httpRoot);
    server.on("/robot", httpGetRobot);
    server.on("/capteurs", httpGetCapteurs);
    server.on("/io", HTTP_POST, httpSetIO);
    server.on("/servos", httpGetServos);
    server.on("/servos", HTTP_POST, httpSetServos);

    server.on("/reset", httpResetESP);
    server.on("/restart", httpRestartESP);

    // Start WebServer
    server.begin();
    
    // Etat par défaut //
    // --------------- //

    digitalWrite(GPIO_RESET_STM32, HIGH); // Enable STM32

    heartTime = heartTimePrec = millis();
	heart = false;
}

void loop() {
    heartBeat();
    wifiReset();
    server.handleClient();
    Debug.handle();

    if (Serial.available()) {
        String inputJson = Serial.readString();
        if (inputJson == "GET_IP") {
            Serial.println(WiFi.localIP().toString());
        }        
    }
}

// ------------------------------------------------------- //
// -------------------- BUSINESS METHODS ----------------- //
// ------------------------------------------------------- //

void heartBeat() {
	heartTime = millis();
	if (heartTime - heartTimePrec > 1000) {
		heartTimePrec = heartTime;
		digitalWrite(LED_BUILTIN, (heart) ? HIGH : LOW);
		heart = !heart;
	}
}

void wifiReset() {
    // Reset EEPROM saved settings
    if (digitalRead(GPIO_WIFI_RESET)) {
        wifiManager.resetSettings();
        debugI(" * RESET WIFI Settings");

        ESP.restart();
        delay(10000); // Not continue
    }
}

// ------------------------------------------------------- //
// ---------------------- HTTP METHODS ------------------- //
// ------------------------------------------------------- //

void httpError() {
    debugE("Affichage de la page d'erreur");
    File f = SPIFFS.open("/404.html", "r");
    server.send(404, "text/html", f.readString());
    f.close();
}

void httpRoot() {
    debugI("Page racine");
    File f = SPIFFS.open("/index.html", "r");
    server.send(200, "text/html", f.readString());
    f.close();
}

void httpGetRobot() {
    debugI("Info robot");
    File f = SPIFFS.open("/robot.json", "r");
    server.send(200, "application/json", f.readString());
    f.close();
}

void httpGetCapteurs() {
    debugI("Liste des Capteurs");
    // TODO : Call STM32 to get IO values
    File f = SPIFFS.open("/capteurs.json", "r");
    server.send(200, "application/json", f.readString());
    f.close();
}

void httpSetIO() {
    StaticJsonDocument<256> query;
    StaticJsonDocument<256> response;
    String jsonResponse;
    int httpCode = 204;

    debugI("Changement d'une IO");

    DeserializationError err = deserializeJson(query, server.arg("plain"));
    if (err) {
        debugE("DeserializeJson() failed with code %s", err.c_str());
        response["error"] = err.c_str();
        httpCode = 500;

    } else {
        String io = query["ioName"];
        boolean value = query["ioValue"];

        if (io == "RESET_STM32") {
            digitalWrite(GPIO_RESET_STM32, value);
        } else if (io == "BOOT0_STM32") {
            digitalWrite(GPIO_BOOT0_STM32, value);
        } else {
            response["error"] = "IO inconnu";
            httpCode = 404;
        }
    }

    serializeJson(response, jsonResponse);
    server.send(httpCode, "application/json", jsonResponse);
}

void httpGetServos() {
    debugI("Liste des Servos");
    String uri = server.uri();

    if (uri == "/servos") {
        File f = SPIFFS.open("/servosConfig.json", "r");
        server.send(200, "application/json", f.readString());
        f.close();
    } else {
        // Lecture d'un servos en particulier
        server.send(200, "application/json", {});
    }
}

void httpSetServos() {
    server.send(501, "application/json", "{\"error\":\"Non implementé\"}");
}

void httpResetESP() {
    server.send(204, "application/json", "{}");
    delay(2000);
    ESP.reset();
}

void httpRestartESP() {
    server.send(204, "application/json", "{}");
    delay(2000);
    ESP.restart();
}