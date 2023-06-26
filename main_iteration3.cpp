/*
  Autor: Fabian Haubenhofer
  Date:   26.6.2023
  Title: DLBINGDT01 - Temperaturlogger für Kühlhäuser
  Iteration: 3
*/

#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PushoverESP32.h>
#include <ArduinoJSON.h>
#include <Arduino_JSON.h>


//TODO; FIX PINS
#define RAUM1_PIN 16
#define RAUM2_PIN 17
#define RAUM3_PIN 18

#define ALARM_TRESHOLD 10.0

void kuehlhaus(int PIN, float SHH_A, float SHH_B, float SHH_C, String NAME, float nomTemp, float TempArr[], unsigned long TimeArr[]);
float readTemperature(int PIN, float SHH_A, float SHH_B, float SHH_C);
void checkTemperature(float actTemp, float nomTemp, String NAME);
void sendAlarm(String NAME);
String formatTemperaturesForWebpage();


//TODO: Set wifi credentials
//wifi connection variables
const char* ssid = "SSID";
const char* password = "PASSWORD";

//Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//Create an Event Source on /events
AsyncEventSource events("/events");

//JSON Variable to keep readings
JSONVar readings;

//Initialize Pushover
const char* token = "API-TOKEN";
const char* user = "USER-TOKEN";
Pushover pushoverClient(token, user);

//NTP Variables
WiFiUDP Udp;
const char* ntpServerName = "de.pool.ntp.org";
const int timeZone = 1;
NTPClient timeClient(Udp, ntpServerName, timeZone * 3600, 60000);

//global arrays for storage
const int valuesToStore = 60;
float arr_Temp_kuehlhaus1[valuesToStore] = {0};
unsigned long arr_Time_kuehlhaus1[valuesToStore] = {0};

float arr_Temp_kuehlhaus2[valuesToStore] = {0};
unsigned long arr_Time_kuehlhaus2[valuesToStore] = {0};

float arr_Temp_kuehlhaus3[valuesToStore] = {0};
unsigned long arr_Time_kuehlhaus3[valuesToStore] = {0};


void setup() {
  Serial.begin(9600);

  //init analog-inputs
  pinMode(RAUM1_PIN, INPUT);
  pinMode(RAUM2_PIN, INPUT);
  pinMode(RAUM3_PIN, INPUT);

  //initialize WiFi
  WiFi.begin(ssid, password);
  Serial.println(WiFi.macAddress());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi..");
    delay(500);
  }
  
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
    Serial.println("SPIFFS mounted successfully");
  }

  //start time-client
  timeClient.begin();
  timeClient.update();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = formatTemperaturesForWebpage();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {

  //TODO: Set wifi credentials
  //Kühlhaus 1
  kuehlhaus(RAUM1_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 1", 8.0, arr_Temp_kuehlhaus1, arr_Time_kuehlhaus1);

  //Kühlhaus 2
  kuehlhaus(RAUM2_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 2", 3.0, arr_Temp_kuehlhaus2, arr_Time_kuehlhaus2);
  
  //Kühlhaus 3
  kuehlhaus(RAUM3_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 3", 15.0, arr_Temp_kuehlhaus3, arr_Time_kuehlhaus3);


  delay(60000);
}

void kuehlhaus(int PIN, float SHH_A, float SHH_B, float SHH_C, String NAME, float nomTemp, float TempArr[], unsigned long TimeArr[]){
  float actTemp = readTemperature(PIN, SHH_A, SHH_B, SHH_C);
  Serial.println(NAME + " - Temperatur: " + String(actTemp) + " °C");
  
  checkTemperature(actTemp, nomTemp, NAME);

  // shift values and times one index back, starting from the end.
  for (int i = valuesToStore - 1; i >= 1; i--) {
    TempArr[i] = TempArr[i-1];
    TimeArr[i] = TimeArr[i-1];
  }

  timeClient.update();
  TempArr[0] = actTemp;
  TimeArr[0] = timeClient.getEpochTime();
}


float readTemperature(int PIN, float SHH_A, float SHH_B, float SHH_C){
  //read analogInput and transform to resistance
  //formula: R_NTC = R_VoltageDivider / ((ADC_max/ADC_actValue)-1)
  float resistance = 10000.0 / ((4095.0/(float(analogRead(PIN)))) - 1.0);
  
  //convert from resistance to °C
  //formula: 1 / T = A + B*ln(R) + C*ln(R)³
  //store log(resistance) to prevent multiple function-calls
  float temp_logRes = log(resistance) * 1.0;
  float degreeCelsius =  1.0 / (SHH_A + (SHH_B * temp_logRes) + (SHH_C + (temp_logRes * temp_logRes * temp_logRes)));

  return degreeCelsius;
}

void checkTemperature(float actTemp, float nomTemp, String NAME){
  if(abs(actTemp-nomTemp) > ALARM_TRESHOLD)
    sendAlarm(NAME);
}

void sendAlarm(String NAME){
  String temp_string = NAME + ": Temperaturdifferenz zu hoch!";
    
  PushoverMessage myMessage;

  myMessage.title = NAME.c_str();
  myMessage.message = temp_string.c_str();
  pushoverClient.send(myMessage);
}

String formatTemperaturesForWebpage() {
  // Create an empty array to store the temperature data
  DynamicJsonDocument doc(12000);
  JsonArray data = doc.to<JsonArray>();

  // Add the temperature data for each sensor to the array
  JsonObject sensor1 = data.createNestedObject();
  sensor1["key"] = "sensor1";
  JsonArray values1 = sensor1.createNestedArray("values");

  for (int i = valuesToStore-1; i > 0; i--) {
    JsonArray valuePair = values1.createNestedArray();

    valuePair.add(arr_Temp_kuehlhaus1[i]); // Y-value
    valuePair.add(arr_Time_kuehlhaus1[i]); // Timestamp in epoch format
  }

  // Add the temperature data for each sensor to the array
  JsonObject sensor2 = data.createNestedObject();
  sensor2["key"] = "sensor2";
  JsonArray values2 = sensor2.createNestedArray("values");

  for (int i = valuesToStore-1; i > 0; i--) {
    JsonArray valuePair = values2.createNestedArray();

    valuePair.add(arr_Temp_kuehlhaus2[i]); // Y-value
    valuePair.add(arr_Time_kuehlhaus2[i]); // Timestamp in epoch format
  }

  // Add the temperature data for each sensor to the array
  JsonObject sensor3 = data.createNestedObject();
  sensor3["key"] = "sensor3";
  JsonArray values3 = sensor3.createNestedArray("values");

  for (int i = valuesToStore-1; i > 0; i--) {
    JsonArray valuePair = values3.createNestedArray();

    valuePair.add(arr_Temp_kuehlhaus3[i]); // Y-value
    valuePair.add(arr_Time_kuehlhaus3[i]); // Timestamp in epoch format
  }

  // Convert the temperature data to a JSON string and return it
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}
