
// Knihovny
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <BH1750.h>
#include <SPI.h>
#include <Ethernet2.h>
// Teploměr a vlhkoměr DHT11
#include "MQ135.h"
// připojení knihovny DHT
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
// nastavení čísla pinu s připojeným DHT senzorem
#define pinDHT1 2
#define pinDHT2 5
#define pinA A0
#define pinD 4
#define typDHT11 DHT11

LiquidCrystal_I2C lcd(0x27, 20, 4);
BH1750 luxSenzor;
MQ135 senzorMQ = MQ135(pinA);
DHT DHT1(pinDHT1, typDHT11);
DHT DHT2(pinDHT2, typDHT11);
EthernetServer server(80);

// deklarace promenných
//
//
int row[4] = {0, 1, 2, 3};
int tep = 0;
int vlh = 0;
int lux = 0;
int ppm = 0;
int slider = 0;
int data[4];
const int buttonPin = 6;
byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

void setup()
{
  Wire.begin();
  luxSenzor.begin();
  lcd.init();
  lcd.clear();
  lcd.backlight();
  pinMode(buttonPin, INPUT);
  // komunikace přes sériovou linku rychlostí 9600 baud
  Serial.begin(9600);
  // zapnutí komunikace s teploměrem DHT
  DHT1.begin();
  DHT2.begin();
  // při nástupné hraně (log0->log1) se vykoná program prerus
}

void svitivost(int souradnice)
{
  lux = luxSenzor.readLightLevel();
  lcd.setCursor(0, row[souradnice]);
  lcd.print("Svitivost: ");
  lcd.print(lux);
  lcd.print(" lux");
  data[0] = lux;
}

void plyny(int souradnice)
{
  // načtení koncentrace plynů v ppm do proměnné
  ppm = senzorMQ.getPPM();
  lcd.setCursor(0, row[souradnice]);
  lcd.print("Konc. CO2: ");
  lcd.print(ppm);
  lcd.print(" ppm");
  data[1] = ppm;
}

void teplota(bool zapis, int souradnice1, int souradnice2)
{
  // inicializace DHT senzoru s nastaveným pinem a typem senzoru
  // pomocí funkcí readTemperature a readHumidity načteme
  // do proměnných tep a vlh informace o teplotě a vlhkosti,
  // čtení trvá cca 250 ms
  tep = (DHT1.readTemperature() + DHT2.readTemperature()) / 2;
  vlh = ((DHT1.readHumidity() + DHT2.readHumidity()) / 2);

  // kontrola, jestli jsou načtené hodnoty čísla pomocí funkce isnan
  if (isnan(tep) || isnan(vlh))
  {
    // při chybném čtení vypiš hlášku
    Serial.println("Chyba při čtení z DHT senzoru!");
  }
  else if (zapis == true)
  {
    // pokud jsou hodnoty v pořádku,
    // vypiš je po sériové lince
    lcd.setCursor(2, row[souradnice1]); // Move cursor to character 2 on line 1
    lcd.print("Teplota: ");
    lcd.print(tep);
    lcd.print(" ");
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(2, row[souradnice2]);
    lcd.print("Vlhkost: ");
    lcd.print(vlh);
    lcd.print(" %");
    data[2] = tep;
    data[3] = vlh;
  }
}

void loop()
{
  StaticJsonDocument<200> jsonDoc;
  if (digitalRead(buttonPin))
  {
    if (slider < 5)
    {
      slider++;
    }
    else
    {
      slider = 0;
    }
  }
  lcd.clear();
  teplota(false, 0, 1);
  if (slider == 0)
  {
    teplota(true, 0, 1);
    svitivost(2);
    plyny(3);
  }
  else if (slider == 1)
  {
    teplota(true, 1, 2);
  }
  else if (slider == 2)
  {
    svitivost(1);
  }
  else if (slider == 3)
  {
    plyny(1);
  }
  jsonDoc["temperature"] = data[0];
  jsonDoc["humidity"] = data[1];
  jsonDoc["lightLevel"] = data[2];
  jsonDoc["co2Level"] = data[3];

  String jsonString;
  serializeJson(jsonDoc, jsonString);
  delay(10);
  Serial.println(jsonString);
  delay(4000);
}
