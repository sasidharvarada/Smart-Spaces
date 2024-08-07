#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <string.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define PORT 8100
#define BAUD 115200

// #define CSE_IP "dev-onem2m.iiit.ac.in"
// #define OM2M_ORGIN "Tue_20_12_22:Tue_20_12_22"
// #define CSE_PORT 443
#define HTTPS false
#define OM2M_MN "/~/in-cse/in-name/"
// #define OM2M_AE "AE-AQ"

// #define OM2M_NODE_ID "AQ-SN00-00"
// #define OM2M_DATA_CONT "AQ-SN00-00/ACK-AIRPURIFIER"
// #define OM2M_DATA_LBL "[\"AE-AQ\", \"V4.0.0\", \"AQ-SN00-00\", \"AQ-V4.0.0\"]"


// #define OM2M_NODE_ID_1 "AQ-MG00-00"
// #define OM2M_DATA_CONT_1 "AQ-MG00-00/Data"
// #define OM2M_DATA_LBL_1 "[\"AE-AQ\", \"V4.0.0\", \"AQ-MG00-00\", \"AQ-V4.0.0\"]"

#define CSE_IP "10.3.1.224"
#define OM2M_ORGIN "admin:admin"
#define CSE_PORT 5683
#define OM2M_AE "AE-TEST"
#define OM2M_NODE_ID "ACK-airpurifier"
#define OM2M_DATA_CONT "ACK-airpurifier/Data"
#define OM2M_DATA_LBL"[\"AE-TEST\", \"V4.0.0\", \"ACK-airpurifier\", \"AQ-V4.0.0\"]"


#define OM2M_NODE_ID_1 "ACK-airpurifier"
#define OM2M_DATA_CONT_1 "ACK-airpurifier/Latency"
#define OM2M_DATA_LBL_1"[\"AE-TEST\", \"V4.0.0\", \"ACK-airpurifier\", \"AQ-V4.0.0\"]"


String data;
long rssi;
const int relay1 = 14;
int flag = 0;

String conValue = "0";
String airpurifier = "NC";
String window = "NC";
String Act = "NC";

WiFiClient client;
HTTPClient http;

unsigned long startTime;
unsigned long duration;

// Timestamp variables
unsigned long time_1;
unsigned long time_2;
unsigned long time_3;
unsigned long time_4;

// Global variables for steps
float step1_sec = 0;
float step2_sec = 0;
float step3_sec = 0;
float step4_sec = 0;
float time_send = 0;

// WiFi credentials
const char *ssid = "esw-m19@iiith";
const char *password = "e5W-eMai@3!20hOct";

// const char *ssid = "Sasi_PC";
// const char *password = "Internet";

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
    // Clear previous timestamps
    time_1 = 0;
    time_2 = 0;
    time_3 = 0;
    time_4 = 0;

    startTime = millis(); // Record the start time
    time_1 = startTime;   // Save the timestamp for time_1
    String stri;
    for (int i = 0; i < len; i++) {
        stri += (char)data[i];
    }
    DeserializationError error = deserializeJson(doc, stri);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    JsonObject m2m_cin = doc["m2m:sgn"]["m2m:nev"]["m2m:rep"]["m2m:cin"];
    const char *m2m_cin_con = m2m_cin["con"];
    String conv_data;
    conv_data = m2m_cin_con;

    char *c = const_cast<char *>(conv_data.c_str());
    action(c);

    conValue = conv_data;
    conValue = conValue.substring(1, conValue.length() - 1);

    int commaIndex = conValue.indexOf(',');
    window = conValue.substring(0, commaIndex);
    airpurifier = conValue.substring(commaIndex + 1);
    time_2 = millis();    // Save the timestamp for time_2
    if (!conv_data.isEmpty()) {
        isDataReceived = true;
    }

    request->send(200, "application/json", stri);
}

void switchOFF() {
    digitalWrite(relay1, HIGH);
}

void switchON() {
    digitalWrite(relay1, LOW);
}

void post_onem2m() {
    time_send = (time_3 - time_1) / 1000.0;
    int code = -1;
    while (code != 201) {
        String data = "[" + String(Act) + "," + String(rssi) + "," + String(time_send, 3) + "]";
        String server = "http://" + String(CSE_IP) + ":" + String(CSE_PORT) + String(OM2M_MN);

        http.begin(client, server + OM2M_AE + "/" + OM2M_DATA_CONT);
        http.addHeader("X-M2M-Origin", OM2M_ORGIN);
        http.addHeader("Content-Type", "application/json;ty=4");

        String req_data = String() + "{\"m2m:cin\": {"
                        + "\"con\": \"" + data + "\","
                        + "\"cnf\": \"text\""
                        + "}}";  

        Serial.println("Request Data: " + req_data);

        code = http.POST(req_data);
        delay(100); // Delay to avoid hammering the server
    }

    http.end();
    Serial.println("HTTP Response Code: " + String(code));
    // delay(1000);
    time_4 = millis(); // Save the timestamp for time_4 after receiving 201 response
}

void post_onem2msec() {
    int code = -1;
    while (code != 201) {
        String data = "[" + String(step1_sec,3) + "," + String(step2_sec,3) + "," + String(step3_sec,3) + "," + String(step4_sec,3)+"]";
        String server = "http://" + String(CSE_IP) + ":" + String(CSE_PORT) + String(OM2M_MN);

        http.begin(client, server + OM2M_AE + "/" + OM2M_DATA_CONT_1);
        http.addHeader("X-M2M-Origin", OM2M_ORGIN);
        http.addHeader("Content-Type", "application/json;ty=4");

        String req_data = String() + "{\"m2m:cin\": {"
                        + "\"con\": \"" + data + "\","
                        + "\"cnf\": \"text\""
                        + "}}";  

        Serial.println("Request Data: " + req_data);
        code = http.POST(req_data);
        delay(100); // Delay to avoid hammering the server
    }
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
    digitalWrite(relay1, HIGH);
}

void printTimestamps() {
    float t1 = time_1 / 1000.0;
    float t2 = time_2 / 1000.0;
    float t3 = time_3 / 1000.0;
    float t4 = time_4 / 1000.0;

    // Serial.println("Timestamp values:");
    // Serial.println("time_1: " + String(t1, 3) + " seconds");
    // Serial.println("time_2: " + String(t2, 3) + " seconds");
    // Serial.println("time_3: " + String(t3, 3) + " seconds");
    // Serial.println("time_4: " + String(t4, 3) + " seconds");

    // Calculate steps in seconds with three decimal places
    step1_sec = (time_2 - time_1) / 1000.0;
    step2_sec = (time_3 - time_2) / 1000.0;
    step3_sec = (time_4 - time_3) / 1000.0;
    step4_sec = step1_sec + step2_sec + step3_sec;

    // Serial.println("Time taken for each step:");
    // Serial.println("Step 1: " + String(step1_sec, 3) + " seconds");
    // Serial.println("Step 2: " + String(step2_sec, 3) + " seconds");
    // Serial.println("Step 3: " + String(step3_sec, 3) + " seconds");
    // Serial.println("Step 4 (sum of step1 + step2 + step3): " + String(step4_sec, 3) + " seconds");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
    }
    if (isDataReceived) {
        if (flag == 0 && airpurifier == "ON") {
            switchON();
            airpurifier = "NC";
            flag = 1;
            Act = "ON";
            time_3 = millis();  // Save the timestamp for time_3
            post_onem2m();
        } else if (flag == 1 && airpurifier == "OFF") {
            switchOFF();
            airpurifier = "NC";
            flag = 0;
            Act = "OFF";
            time_3 = millis();  // Save the timestamp for time_3
            post_onem2m();
        } else {
            Serial.println("No change detected!!");
            time_3 = millis();  // Save the timestamp for time_3
            post_onem2m();
        }
        // Print the timestamps
        printTimestamps();
        post_onem2msec();
        isDataReceived = false;
        delay(100);
    }
}
