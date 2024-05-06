#include <WiFi.h>
#include "ThingSpeak.h"
#include "SoftwareSerial.h"
#include <SDS011.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHT10.h>
#include <HTTPClient.h>

// WiFi Configuration
// const char* ssid = "IIIT-Guest";
// const char* password = "I%GR#*S@!";

const char* ssid = "esw-m19@iiith";
const char* password = "e5W-eMai@3!20hOct";  

WiFiClient client;
HTTPClient http;

#define CSE_IP "dev-onem2m.iiit.ac.in"
#define OM2M_ORGIN "Tue_20_12_22:Tue_20_12_22"
#define CSE_PORT 443
#define OM2M_MN "/~/in-cse/in-name/"
#define OM2M_AE "AE-AQ"

//node1
// #define OM2M_NODE_ID   "AQ-VN90-00"
// #define OM2M_DATA_CONT "AQ-VN90-00/Data"
// #define OM2M_DATA_LBL  "[\"AE-AQ\", \"V4.0.0\", \"AQ-VN90-00\", \"AQ-V4.0.0\"]"
// unsigned long myChannelNumber = 2250356;
// const char* myWriteAPIKey = "JU8NF1AJEXFALQLB"; // 202
//  float m_p25 = 1.24190607;
//   float c_p25 = 0.7643945779499859;
//   float m_p10 = 2.50519704;
//   float c_p10 =  1.8662012942554327;

// node 2
#define OM2M_NODE_ID "AQ-KH00-00"
#define OM2M_DATA_CONT "AQ-KH00-00/Data"
#define OM2M_DATA_LBL "[\"AE-AQ\", \"V4.0.0\", \"AQ-KH00-00\", \"AQ-V4.0.0\"]"
unsigned long myChannelNumber = 2346635;
const char* myWriteAPIKey = "M9EFT74QYIZI45QF";  //213
float m_p25 = 3.72914404;
float c_p25 = -2.706634452125016;
float m_p10 = 6.89905634;
float c_p10 = -0.5337796216823847;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;

float p10, p25;
float pm2_cal = 0;
float pm10_cal = 0;
int error;
SDS011 my_sds;
float aqi;

// AHT10 Configuration
Adafruit_AHT10 aht;
Adafruit_Sensor* aht_humidity;
Adafruit_Sensor* aht_temp;
float t;
float h;

void setup() {
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  Serial.begin(9600);
  my_sds.begin(16, 17);
  aht_setup();
}

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      delay(5000);
    }
    Serial.println("\nConnected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Mac Address :");
    Serial.println(WiFi.macAddress());
    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI): ");
    Serial.print(rssi);
  }
  Serial.println(WiFi.macAddress());
}

void SDS011() {
  error = my_sds.read(&p25, &p10);
  if (!error) {
    Serial.println("P2.5: " + String(p25));
    Serial.println("P10:  " + String(p10));
  }
  pm2_cal = (m_p25 * p25) + c_p25;
  pm10_cal = (m_p10 * p25) + c_p10;
  Serial.println("P2.5_cal: " + String(pm2_cal));
  Serial.println("P10_cal:  " + String(pm10_cal));
  delay(1000);
}

void aht_setup() {
  if (!aht.begin()) {
    Serial.println("Failed to find AHT10 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("AHT10 Found!");
  aht_temp = aht.getTemperatureSensor();
  aht_humidity = aht.getHumiditySensor();
}

void aht_loop() {
  sensors_event_t humidity;
  sensors_event_t temp;
  aht_humidity->getEvent(&humidity);
  aht_temp->getEvent(&temp);
  t = temp.temperature;
  h = humidity.relative_humidity;
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C\t");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("% rH");
}

float cal_aqi() {
  float s10, s25;

  // Calculate the sub-indices for PM10 and PM2.5
  if (pm10_cal <= 100) {
    s10 = pm10_cal;
  } else if (pm10_cal > 100 && pm10_cal <= 250) {
    s10 = 100 + ((pm10_cal - 100) * 100 / 150);
  } else if (pm10_cal > 250 && pm10_cal <= 350) {
    s10 = 200 + (pm10_cal - 250);
  } else if (pm10_cal > 350 && pm10_cal <= 430) {
    s10 = 300 + ((pm10_cal - 350) * 100 / 80);
  } else {
    s10 = 400 + ((pm10_cal - 430) * 100 / 80);
  }

  if (pm2_cal <= 30) {
    s25 = pm2_cal * 50 / 30;
  } else if (pm2_cal > 30 && pm2_cal <= 60) {
    s25 = 50 + ((pm2_cal - 30) * 50 / 30);
  } else if (pm2_cal > 60 && pm2_cal <= 90) {
    s25 = 100 + ((pm2_cal - 60) * 100 / 30);
  } else if (pm2_cal > 90 && pm2_cal <= 120) {
    s25 = 200 + ((pm2_cal - 90) * 100 / 30);
  } else if (pm2_cal > 120 && pm2_cal <= 250) {
    s25 = 300 + ((pm2_cal - 120) * 100 / 130);
  } else {
    s25 = 400 + ((pm2_cal - 250) * 100 / 130);
  }

  // Determine the final AQI by choosing the maximum of s10 and s25
  if (s10 > s25) {
    aqi = s10;
    Serial.print("AQI: ");
    Serial.println(aqi);
    return s10;
  } else {
    aqi = s25;
    Serial.print("AQI: ");
    Serial.println(aqi);
    return s25;
  }
  Serial.println("----------AQI updated successfully!-------------");
}

void thingspeak() {

  ThingSpeak.setField(3, t);
  ThingSpeak.setField(4, h);
  ThingSpeak.setField(1, pm2_cal);
  ThingSpeak.setField(2, pm10_cal);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}

void postToOneM2M() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String data = String(aqi);
 
  // String data = "[" + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(nan) + " , " + String(aqi) +" , " + String(nan) +" , " + String(nan) + " , " + String(nan) +  "]";
  
  String server = "http://" + String(CSE_IP) + ":" + String(CSE_PORT) + String(OM2M_MN);

  http.begin(server + OM2M_AE + "/" + OM2M_DATA_CONT + "/");

  http.addHeader("X-M2M-Origin", OM2M_ORGIN);
  http.addHeader("Content-Type", "application/json;ty=4");
  http.addHeader("Content-Length", String(data.length()));

  String req_data = "{\"m2m:cin\": {"
                    "\"con\": \""
                    + data + "\","
                             "\"lbl\": "
                    + OM2M_DATA_LBL + ","
                                      "\"cnf\": \"text\""
                                      "}}";
  int code = http.POST(req_data);
  http.end();
  Serial.println("OneM2M HTTP Response Code: " + String(code));
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    connectToWiFi();
    SDS011();
    aht_loop();
    cal_aqi();
    postToOneM2M();
    thingspeak();
    lastTime = millis();
  }
}