/*
  Autor: Fabian Haubenhofer
  Date:   26.6.2023
  Title: DLBINGDT01 - Temperaturlogger für Kühlhäuser
  Iteration: Initial
*/

#include <Arduino.h>

//TODO; FIX PINS
#define RAUM1_PIN 16
#define RAUM2_PIN 17
#define RAUM3_PIN 18

void kuehlhaus(int PIN, float SHH_A, float SHH_B, float SHH_C, String NAME);
float readTemperature(int PIN, float SHH_A, float SHH_B, float SHH_C);

void setup() {
  Serial.begin(9600);

  //init analog-inputs
  pinMode(RAUM1_PIN, INPUT);
  pinMode(RAUM2_PIN, INPUT);
  pinMode(RAUM3_PIN, INPUT);
}

void loop() {

  //TODO: Set wifi credentials
  //Kühlhaus 1
  kuehlhaus(RAUM1_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 1");

  //Kühlhaus 2
  kuehlhaus(RAUM2_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 2");

  //Kühlhaus 3
  kuehlhaus(RAUM3_PIN, 1.0, 1.0, 1.0, "Kuehlhaus 3");

}

void kuehlhaus(int PIN, float SHH_A, float SHH_B, float SHH_C, String NAME){
  float actTemp = readTemperature(PIN, SHH_A, SHH_B, SHH_C);
  Serial.println(NAME + " - Temperatur: " + String(actTemp) + " °C");
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
