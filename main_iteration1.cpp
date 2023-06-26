/*
  Autor: Fabian Haubenhofer
  Date:   26.6.2023
  Title: DLBINGDT01 - Temperaturlogger für Kühlhäuser
  Iteration: 1
*/

#include <Arduino.h>
#include <WiFi.h>
#include <PushoverESP32.h>


//TODO: FIX PINS
#define RAUM1_PIN 16
#define RAUM2_PIN 17
#define RAUM3_PIN 18

#define ALARM_TRESHOLD 10.0

void kuehlhaus(int PIN, float SHH_A, float SHH_B, float SHH_C, String NAME, float nomTemp);
float readTemperature(int PIN, float SHH_A, float SHH_B, float SHH_C);
void checkTemperature(float actTemp, float nomTemp, String NAME);
void sendAlarm(String NAME);

//TODO: Set wifi credentials
//wifi connection variables
const char* ssid = "SSID";
const char* password = "PASSWORD";

//TODO: Set Tokens
//Initialize Pushover
const char* token = "API-TOKEN";
const char* user = "USER-TOKEN";
Pushover pushoverClient(token, user);

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
}

void loop() {

  //TODO: Set wifi credentials
  //Kühlhaus 1
  kuehlhaus(RAUM1_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 1", 8.0);

  //Kühlhaus 2
  kuehlhaus(RAUM2_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 2", 3.0);
  
  //Kühlhaus 3
  kuehlhaus(RAUM3_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 3", 15.0);
}

void kuehlhaus(int PIN, float SHH_A, float SHH_B, float SHH_C, String NAME, float nomTemp){
  float actTemp = readTemperature(PIN, SHH_A, SHH_B, SHH_C);
  Serial.println(NAME + " - Temperatur: " + String(actTemp) + " °C");

  checkTemperature(actTemp, nomTemp, NAME);
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