#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// WiFi params
// char* WIFI_SSID = "SCRC_LAB_IOT";
// char* WIFI_PSWD = "Scrciiith@123";

// char* WIFI_SSID = "IIIT-Guest";
// char* WIFI_PSWD = "I%GR#*S@!";

char* WIFI_SSID = "esw-m19@iiith";
char* WIFI_PSWD = "e5W-eMai@3!20hOct";

// onem2m post 
#define CSE_IP      "dev-onem2m.iiit.ac.in"
#define OM2M_ORGIN    "Tue_20_12_22:Tue_20_12_22"
#define CSE_PORT    443
#define HTTPS     false
#define OM2M_MN     "/~/in-cse/in-name/"
#define OM2M_AE     "AE-AQ"
#define OM2M_NODE_ID   "AQ-SN00-00"
#define OM2M_DATA_CONT  "AQ-SN00-00/ACK-AIRPURIFIER"
#define OM2M_DATA_LBL   "[\"AE-AQ\", \"V4.0.0\", \"AQ-SN00-00\", \"AQ-V4.0.0\"]"

WiFiClient client;
HTTPClient http;
long rssi;
const int button = 5;
const int relay1 = 14;

int buttonState = 0;
int flag = 0;
String conValue = "0";
String airpurifier = "NC";
String window = "NC";
String Act="NC";

void connect_to_WIFI() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PSWD);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED || WiFi.status() == WL_CONNECT_FAILED) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to " + String(WIFI_SSID) + " network"); 
    rssi = WiFi.RSSI();
    Serial.println("RSSI :" + String(rssi) + "db");
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
  }
}
String sendGET(String url) {
  HTTPClient http;
  http.setTimeout(10000); // Set a timeout of 10 seconds (adjust as needed)
  http.begin(url);
  http.addHeader("X-M2M-Origin", "dev_guest:dev_guest");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.GET();
  String payload = "";

  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("---------------------------------check onem2m after 10 sec ----------------------------------------------");
    // Serial.println(payload);
    // Find the "con" field within the payload
    int conStart = payload.indexOf("\"con\" : ");
    if (conStart >= 0) {
      int conEnd = payload.indexOf("\n", conStart);
      if (conEnd >= 0) {
        conValue = payload.substring(conStart + 8, conEnd);  // Extract "con" value
        conValue.trim();                                     // Remove leading and trailing spaces
        Serial.print("OneM2M value : ");
        Serial.println(conValue);

        // Remove square brackets
        conValue = conValue.substring(1, conValue.length() - 1);

        // Split the string by comma
        int commaIndex = conValue.indexOf(',');
        window = conValue.substring(0, commaIndex); // Update global variable
        airpurifier = conValue.substring(commaIndex + 1); // Update global variable

        // Print the extracted values without square brackets
        Serial.print("window: ");
        Serial.println(window.substring(1));
        window = conValue.substring(1, commaIndex); // Remove the leading '['
        airpurifier = airpurifier.substring(0, airpurifier.length() - 1);
        Serial.print("airpurifier: ");
        Serial.println(airpurifier);
      }
    }
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpCode);
  }

  http.end();
  return conValue;
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
                    + "\"rn\": \"" + "cin_" + String(millis()) + "\","
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
  Serial.begin(9600);
  connect_to_WIFI();
  pinMode(relay1, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(relay1, HIGH);
}

void loop() {
  // Make an HTTP GET request to your desired URL and update conValue
  String url = "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-AQ/AQ-SN00-00/Data/la";
  conValue = sendGET(url);
  buttonState = digitalRead(button);

  // if (flag == 0 && (buttonState == 1 || airpurifier == "1")) {
  if (flag == 0 && (buttonState == 1 || airpurifier == "ON")) {
    switchON();
    airpurifier = "NC";
    Serial.println("Switching ON......");
    flag = 1;
    Act="ON";
    post_onem2m();
  } 
  // else if (flag == 1 && (buttonState == 1 || airpurifier == "0")) {
  else if (flag == 1 && (buttonState == 1 || airpurifier == "OFF")) {
    switchOFF();
    airpurifier = "NC";
    Serial.println("Switching OFF......");
    flag = 0;
    Act="OFF";
    post_onem2m();
  } 
  else {
    Serial.println("No change detected!!");
    post_onem2m();
  }
  delay(1000);
}
