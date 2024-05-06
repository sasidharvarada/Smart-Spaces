#include <WiFi.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_SGP40.h"
#include "Adafruit_SHT4x.h"
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <ESP32Time.h>
#include "constants.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


#define OLED_SDA 21
#define OLED_SCL 22

#define i2c_Address 0x3c 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

//------------------------------------wifi setup------------------------------------------------------------//

// void initWiFi() {
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   Serial.println("Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print('.');
//     delay(1000);
//   }
//     Serial.println("Connected to Wifi");
//   Serial.println(WiFi.localIP());  long rssi = WiFi.RSSI();
//   Serial.print("signal strength (RSSI):");
//   Serial.print(rssi);
// }

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");
  unsigned long startTime = millis(); // Get the current time in milliseconds

  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    Serial.print('.');
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());    
    Serial.print("Mac Address :");
    Serial.println(WiFi.macAddress());
    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI): ");
    Serial.print(rssi);
  } else {
    Serial.print("Mac Address :");
    Serial.println(WiFi.macAddress());
    Serial.println("\nFailed to connect to WiFi within 30 seconds. Restarting...");
    ESP.restart(); // Restart the device
  }
}

//................................. time setup...........................................................................................................///////

void sync_time() {
  static bool first_time = true;
  int num_tries = 5;  // Number of attempts to get valid NTP time

  // Check if the ESP32 is connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    if (first_time == true) {
      // Initialize NTPClient only once
      timeClient.begin();
      timeClient.setTimeOffset(0);
      timeClient.update();
      first_time = false;
    }

    while (num_tries > 0) {
      
      // Get the obtained NTP time
      uint64_t ntp_time = timeClient.getEpochTime();
      // Serial.println(ntp_time);
      // Validate the obtained NTP time
      if (ntp_time != 0 && ntp_time >= MIN_VALID_TIME && ntp_time <= MAX_VALID_TIME) {
        // Set the RTC time using the NTP time
        rtc.setTime(ntp_time);
        ntp_epoch_time = rtc.getEpoch();
        // Serial.println(ntp_epoch_time);
        if(ntp_epoch_time < 1672000000){
          sync_time();
        }
        // Print the universal time and epoch time
          Serial.print("Universal time: " + rtc.getDateTime(true) + "\t");
          Serial.println(ntp_epoch_time, DEC); // For UTC, take timeoffset as 0
          // Serial.println("");
        // Update the last_update time
        prev_time = millis();
        return;  // Exit the function after successful synchronization
      } else {
        // Invalid NTP time, reduce the number of remaining attempts
        Serial.println("Invalid NTP time, retrying...");
        num_tries--;
        delay(1000);  
        Serial.println("All attempts failed, performing restarting time function ...");
        ESP.restart();   
      }
    }
  }
}

//..................................co2 .................................................................................................................................//


void CO2_Monitor() {


  th = pulseIn(PIN, HIGH, 2008000) / 1000;
  tl = 1004 - th;
  ppm = 2000 * (th - 2) / (th + tl - 4);

  Serial.print("CO2 Concentration: ");
  Serial.println(ppm);
  ThingSpeak.setField(5, ppm);
  Serial.println("----------CO2 updated successfully!-------------");
}



Adafruit_SGP40 sgp;
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

//........................................................................voc.................................////////////////////////////////////
void sht_sgp_setup() {
  while (!Serial) {
    delay(10);
  } // Wait for serial console to open!

  if (!sgp.begin()) {
    Serial.println("SGP Sensor not found :(");
    while (1)
      ;
  }
  if (!sht4.begin()) {
    Serial.println("SHT Sensor not found :(");
    while (1)
      delay(1);
  }
}

void sgp_loop() {
  voc_index = sgp.measureVocIndex(t, h);
  raw = sgp.measureRaw();
  Serial.print("Raw VOC: ");
  Serial.print(raw);
  Serial.println(" ppb");
  Serial.print("VOC Index: ");
  Serial.println(voc_index);
  ThingSpeak.setField(6, raw);
  ThingSpeak.setField(7, voc_index);
  Serial.println("----------SGP VOC updated successfully!-------------");
}
//.............................................................temp/rH............................................................................//

void sht_loop() {
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  t = temp.temperature;
  h = humidity.relative_humidity;
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C\t");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("% rH");
  ThingSpeak.setField(3, t);
  ThingSpeak.setField(4, h);
  Serial.println("----------SHT Data updated successfully!-------------");
}


//.............................................................temp/rH............................................................................//


byte command_frame[9] = {0xAA, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67, 0xBB};
byte received_data[9];
int sum = 0;

void send_command(byte command) {
  command_frame[1] = command;
  sum = command_frame[0] + command_frame[1] + command_frame[2] + command_frame[3] + command_frame[4] + command_frame[5] + command_frame[8];
  int rem = sum % 256;
  command_frame[6] = (sum - rem) / 256;
  command_frame[7] = rem;
  delay(1000);
  Serial.write(command_frame, 9);
}

bool checksum() {
  sum = received_data[0] + received_data[1] + received_data[2] + received_data[3] + received_data[4] + received_data[5] + received_data[8];
  return sum == (received_data[6] * 256 + received_data[7]);
}

void calculate_pm() {
  pm2 = received_data[4] * 256 + received_data[5];
  delay(2000);
  pm10 = received_data[2] * 256 + received_data[3];
  Serial.print("PM2.5: ");
  Serial.println(pm2);
  Serial.print("PM10: ");
  Serial.println(pm10);
  ThingSpeak.setField(1, pm2);
  ThingSpeak.setField(2, pm10);
  Serial.println("----------PM updated successfully!-------------");
}

void PM_setup() {
  send_command(0x01);
}

void PM_Reading() {
  delay(5000);
  if (millis() - prev_time > 5000) {
    send_command(0x02);
    prev_time = millis();
  }
  if (Serial.available()) {
    Serial.readBytes(received_data, 9);
    if (checksum()) {
      calculate_pm();
    }
  }
}

float cal_aqi(){
  float s10, s25;

  // Calculate the sub-indices for PM10 and PM2.5
  if (pm10 <= 100) {
    s10 = pm10;
  } else if (pm10 > 100 && pm10 <= 250) {
    s10 = 100 + ((pm10 - 100) * 100 / 150);
  } else if (pm10 > 250 && pm10 <= 350) {
    s10 = 200 + (pm10 - 250);
  } else if (pm10 > 350 && pm10 <= 430) {
    s10 = 300 + ((pm10 - 350) * 100 / 80);
  } else {
    s10 = 400 + ((pm10 - 430) * 100 / 80);
  }

  if (pm2 <= 30) {
    s25 = pm2 * 50 / 30;
  } else if (pm2 > 30 && pm2 <= 60) {
    s25 = 50 + ((pm2 - 30) * 50 / 30);
  } else if (pm2 > 60 && pm2 <= 90) {
    s25 = 100 + ((pm2 - 60) * 100 / 30);
  } else if (pm2 > 90 && pm2 <= 120) {
    s25 = 200 + ((pm2 - 90) * 100 / 30);
  } else if (pm2 > 120 && pm2 <= 250) {
    s25 = 300 + ((pm2 - 120) * 100 / 130);
  } else {
    s25 = 400 + ((pm2 - 250) * 100 / 130);
  }

  // Determine the final AQI by choosing the maximum of s10 and s25
  if (s10 > s25) {
    aqi = s10;
    Serial.print("AQI: ");
    Serial.println(aqi);
    ThingSpeak.setField(8, aqi);    
    return s10;
  } else {
    aqi = s25;
    Serial.print("AQI: ");
    Serial.println(aqi);
    ThingSpeak.setField(8, aqi);
    return s25;
  }
  Serial.println("----------AQI updated successfully!-------------");
}
//.............................................................Thingspeak............................................................................//


void postToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected ");
  }
  int x = ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_API_KEY);
  ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_API_KEY);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}


//.............................................................oneM2M...........................................................................//

void postToOneM2M() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String data = "[" + String(ntp_epoch_time) + " , " + String(pm2) + " , " + String(pm10) + " , " + String(ppm) + " , " + String(voc_index) + " , " + String(raw) + " , " + String(t) + " , " + String(h) + " , " + String(aqi) +"]";

  String server = "http://" + String(CSE_IP) + ":" + String(CSE_PORT) + String(OM2M_MN);

  http.begin(server + OM2M_AE + "/" + OM2M_DATA_CONT + "/");

  http.addHeader("X-M2M-Origin", OM2M_ORGIN); 
  http.addHeader("Content-Type", "application/json;ty=4");
  http.addHeader("Content-Length", String(data.length()));

  String req_data = "{\"m2m:cin\": {"
                    "\"con\": \"" + data + "\","
                    "\"lbl\": " + OM2M_DATA_LBL + ","
                    "\"cnf\": \"text\""
                    "}}";
  int code = http.POST(req_data);
  http.end();
  Serial.println("OneM2M HTTP Response Code: " + String(code));
}

//-----------------------------------------------------------------------setup & loooooop-------------------------//

void oled(){

  // Display Text
  delay(3000);
	display.clearDisplay();
  // display.display();
  display.drawBitmap(0, 0,  iiith, 128, 64, SH110X_WHITE);
  display.display();
	delay(3000);

  // display scrc logo 
  display.clearDisplay();
  display.drawBitmap(0, 0,  bitmap_scrc, 128, 64, SH110X_WHITE);
  display.display();
  delay(3000);

  // display.clearDisplay();
  // display.setTextSize(2);
  // display.setTextColor(SH110X_WHITE);
  // display.setCursor(0,8);
  // display.println(OM2M_Node_ID);
  // display.display();
  // delay(3000);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,8);
  display.println("pm_2.5 =");
  display.println(pm2);
  display.display();
  delay(3000);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,8);
  display.println("pm_10 =");
  display.println(pm10);
  display.display();
  delay(3000);

  // display.clearDisplay();
  // display.setTextSize(2);
  // display.setTextColor(SH110X_WHITE);
  // display.setCursor(0,8); 
  // display.println("Voc index");
  // display.println(voc_index);
  // display.display();
  // delay(3000);

  // display.clearDisplay();
  // display.setTextSize(2);
  // display.setTextColor(SH110X_WHITE);
  // display.setCursor(0,8); 
  // display.println("Voc raw");
  // display.println(raw);
  // display.display();
  // delay(3000);

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,8);
  display.println("AQI =");
  display.println(aqi);
  display.display();
  delay(3000);
  
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,8);
  display.println("temp =");
  display.println(t);
  display.display();
  delay(3000);

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,8);
  display.println("rH =");
  display.println(h);
  display.display();
  delay(3000);

  //  display.clearDisplay();
  // display.setTextSize(3);
  // display.setTextColor(SH110X_WHITE);
  // display.setCursor(0,8);
  // display.println("CO2 = ");
  // display.println(ppm);
  // display.display();
  // delay(3000);
    
  display.clearDisplay();

}

//-----------------------------------------------------------------------setup & loooooop-------------------------//

void setup() {
  Serial.begin(9600);
  initWiFi();
  PM_setup();
  sht_sgp_setup();
  ThingSpeak.begin(client);
  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
   //display.setContrast (0); // dim display
  pinMode(OLED_SDA, INPUT_PULLUP);           // set pin to input
  pinMode(OLED_SCL, INPUT_PULLUP);
  display.display();
  delay(10000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
  PM_Reading();
  sht_loop();
  CO2_Monitor();
  sgp_loop();
  float calculatedAQI = cal_aqi();
  // sync_time();
  postToThingSpeak();
  postToOneM2M();
  oled();
  }
  Serial.println("Loop completed, waiting for 10 seconds...");
  delay(10000);
}
