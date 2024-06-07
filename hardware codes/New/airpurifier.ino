#include "WiFi.h"
#include "ESPAsyncWebSrv.h"
#include <string.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define PORT 8100
#define BAUD 115200

#define CSE_IP "dev-onem2m.iiit.ac.in"
#define OM2M_ORGIN "Tue_20_12_22:Tue_20_12_22"
#define CSE_PORT 443
#define HTTPS false
#define OM2M_MN "/~/in-cse/in-name/"
#define OM2M_AE "AE-AQ"
#define OM2M_NODE_ID "AQ-SN00-00"
#define OM2M_DATA_CONT "AQ-SN00-00/ACK-AIRPURIFIER"
#define OM2M_DATA_LBL "[\"AE-AQ\", \"V4.0.0\", \"AQ-SN00-00\", \"AQ-V4.0.0\"]"

String data;
long rssi;
const int button = 5;
const int relay1 = 14;
int buttonState = 0;
int flag = 0;

String conValue = "0";
String airpurifier = "NC";
String window = "NC";
String Act = "NC";

WiFiClient client;
HTTPClient http;

// WiFi credentials
const char *ssid = "esw-m19@iiith";
const char *password = "e5W-eMai@3!20hOct";

// Space required by the packages received
StaticJsonDocument<512> doc;

// Hosted on port 8100
AsyncWebServer server(PORT);

void action(char *data_input) {
    const char *data_in = data_input;
    // Add your function here!!!
}

void wifi_init() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
    rssi = WiFi.RSSI();
}

bool isDataReceived = false;

void data_receive(AsyncWebServerRequest *request, unsigned char *data, size_t len, size_t index, size_t total) {
    String stri;
    Serial.print("\n");
    for (int i = 0; i < len; i++) {
        stri += (char)data[i];
    }
    Serial.print(stri);
    Serial.print("\n");

    DeserializationError error = deserializeJson(doc, stri);

    if (error) {
        // Serial.print("deserializeJson() failed: ");
        // Serial.println(error.c_str());
        return;
    }

    JsonObject m2m_cin = doc["m2m:sgn"]["m2m:nev"]["m2m:rep"]["m2m:cin"];
    const char *m2m_cin_con = m2m_cin["con"];
    String conv_data;
    conv_data = m2m_cin_con;
    Serial.print("DATA: ");
    Serial.println(conv_data);
    Serial.println("======== oneM2M DATA PARSED! ==========");

    // Use if the data required needs to be in the format of char*
    char *c = const_cast<char *>(conv_data.c_str());
    action(c);

    conValue = conv_data;
    conValue = conValue.substring(1, conValue.length() - 1);

    // Split the string by comma
    int commaIndex = conValue.indexOf(',');
    window = conValue.substring(0, commaIndex); // Update global variable
    airpurifier = conValue.substring(commaIndex + 1); // Update global variable

    // Print the extracted values without square brackets
    Serial.print("window: ");
    Serial.println(window);
    Serial.print("airpurifier: ");
    Serial.println(airpurifier);

    if (!conv_data.isEmpty()) {
        isDataReceived = true;
    }

    // Send back a response!
    request->send(200, "application/json", stri);
}

void switchOFF() {
    digitalWrite(relay1, HIGH);
}

void switchON() {
    digitalWrite(relay1, LOW);
}

void post_onem2m() {
    String data  = "[" + String(Act) + "," + String(rssi) +"]";
    String server = "http://" + String(CSE_IP) + ":" + String(CSE_PORT) + String(OM2M_MN);

    http.begin(client, server + OM2M_AE + "/" + OM2M_DATA_CONT);
    http.addHeader("X-M2M-Origin", OM2M_ORGIN);
    http.addHeader("Content-Type", "application/json;ty=4");

    String req_data = String() + "{\"m2m:cin\": {"
                    + "\"con\": \"" + data + "\","
                    + "\"cnf\": \"text\""
                    + "}}";  

    Serial.println("Server URL: " + server);
    Serial.println("Request Data: " + req_data);

    int code = http.POST(req_data);
    http.end();
    Serial.println("HTTP Response Code: " + String(code));
    delay(1000);
}

void setup() {
    Serial.begin(115200);
    wifi_init();
    server.on(
        "/",
        HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        data_receive);
    server.begin();
    pinMode(relay1, OUTPUT);
    pinMode(button, INPUT);
    digitalWrite(relay1, HIGH);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
    }
    if (isDataReceived) {
        buttonState = digitalRead(button);

        if (flag == 0 && (buttonState == 1 || airpurifier == "ON")) {
            switchON();
            airpurifier = "NC";
            Serial.println("Switching ON...");
            flag = 1;
            Act = "ON";
            post_onem2m();
        } else if (flag == 1 && (buttonState == 1 || airpurifier == "OFF")) {
            switchOFF();
            airpurifier = "NC";
            Serial.println("Switching OFF...");
            flag = 0;
            Act = "OFF";
            post_onem2m();
        } else {
            Serial.println("No change detected!!");
            post_onem2m();
        }
        isDataReceived = false;
        delay(100); // Reset the flag
    }
}
