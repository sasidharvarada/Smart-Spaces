#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// WiFi params
// char* WIFI_SSID = "SCRC_LAB_IOT";
// char* WIFI_PSWD = "Scrciiith@123";
//  WiFi params
// char* WIFI_SSID ="IIIT-Guest";
// char* WIFI_PSWD = "I%GR#*S@!";

char* WIFI_SSID = "esw-m19@iiith";
char* WIFI_PSWD = "e5W-eMai@3!20hOct";

long rssi ;
// onem2m post 
#define CSE_IP      "dev-onem2m.iiit.ac.in"
#define OM2M_ORGIN    "Tue_20_12_22:Tue_20_12_22"
#define CSE_PORT    443
#define HTTPS     false
#define OM2M_MN     "/~/in-cse/in-name/"
#define OM2M_AE     "AE-AQ"
#define OM2M_NODE_ID   "AQ-SN00-00"
#define OM2M_DATA_CONT  "AQ-SN00-00/ACK-WINDOW"
#define OM2M_DATA_LBL   "[\"AE-AQ\", \"V4.0.0\", \"AQ-SN00-00\", \"AQ-V4.0.0\"]"

WiFiClient client;

const int button = 15;
const int relay1 = 17;
const int relay2 = 16;

int buttonState = 0;
int flag = 0;

String conValue = "0";  // Initialize conValue with a default value
String window = "NC";
String airpurifier = "NC";
String Act ;

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

void clockWise() {
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, HIGH);
}

void antiClockWise() {
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, LOW);
}

void stayLOW() {
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
}

void post_onem2m() {
  HTTPClient http;

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
  pinMode(relay2, OUTPUT);
  pinMode(button, INPUT);

  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);

  antiClockWise();
  Serial.println("Wait for 8 seconds ......");
  delay(8000);
}

void loop() {
  // Make an HTTP GET request to your desired URL and update conValue
  String url = "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-AQ/AQ-SN00-00/Data/la";
  conValue = sendGET(url);
  Serial.println("Window: " + window);
  Serial.println("Airpurifier: " + airpurifier);

  buttonState = digitalRead(button);

    // if (flag == 0 && (buttonState == 1 || window == "1")) {
  if (flag == 0 && (buttonState == 1 || window == "CLOSE")) {
  // if (flag == 0 &&  window == "CLOSE") {
    clockWise();
    delay(2000);
    Serial.println("Closing the window ......");
    flag = 1;
    window = "NC";
    buttonState = 0;
    Act="CLOSE";
    // delay(10000);
    delay(3000);
    stayLOW();
    post_onem2m();

  } 
  // else if (flag == 1 && (buttonState == 1 || window == "0")) {
  else if (flag == 1 && (buttonState == 1 || window == "OPEN")) {
  // else if (flag == 1 && window == "OPEN") {
    antiClockWise();
    flag = 0;
    window = "NC";
    buttonState = 0;
    Serial.println("Opening  the window......");
    // delay(10000);
    Act="OPEN";
    delay(4500);
    // delay(3000);
    stayLOW();
    post_onem2m();
  }   
  else {
    // stayLOW();
    Serial.println("No change detected!!");
    // post_onem2m();
  }
  delay(1000);
  // delay(60000);
}