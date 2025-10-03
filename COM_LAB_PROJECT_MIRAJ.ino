#include <DFRobot_SIM808.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PIN_TX 7  
#define PIN_RX 8  
#define BUTTON_PIN 2
#define GSMLED 3
#define wl 4
#define GPSLED 6

#define DHTPIN 5
#define DHTTYPE DHT11
#define PHONE_NUMBER "+8801744132580"


SoftwareSerial mySerial(PIN_TX, PIN_RX);  
DFRobot_SIM808 sim808(&mySerial);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() 
{  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GSMLED, OUTPUT);
  pinMode(wl, OUTPUT);
  pinMode(GPSLED, OUTPUT);
  
  mySerial.begin(9600);  
  Serial.begin(9600);  
  dht.begin();
  lcd.begin(16, 2); 
  lcd.backlight();

  while(!sim808.init()) 
  {   
    delay(1000);  
    Serial.print("Sim808 init error\r\n");  
  }  
  Serial.println("Sim808 init success");  
  Serial.println("Device ready");

 
  mySerial.println("AT+IPR=9600");
  delay(1000);
  while (mySerial.available()) 
  {
    Serial.write(mySerial.read());
  }

 
  mySerial.println("AT+CGPSPWR=1");
  delay(1000);
  while (mySerial.available()) 
  {
    Serial.write(mySerial.read());
  }
  mySerial.println("AT+CGPSRST=0");
  delay(1000);
  while (mySerial.available()) 
  {
    Serial.write(mySerial.read());
  }
}

bool getSignalStrength() 
{
  int retryCount = 3;  
  int signalStrength = 0;

  for (int i = 0; i < retryCount; i++) 
  {
    if (sim808.getSignalStrength(&signalStrength)) 
    {
      if (signalStrength > 0) 
      {
        Serial.println("GSM Signal Good");
        return true;
      } else 
      {
        Serial.println("GSM Signal Bad");
        return false;
      }
    } 
    else 
    {
      Serial.println("Attempt failed to get signal strength");
      delay(500);
    }
  }
  
  Serial.println("Failed to get signal strength after retries");
  return false;
}

String getGPSData() 
{
  mySerial.println("AT+CGPSINF=0");
  delay(1000);

  String gpsData = "";
  while (mySerial.available()) 
  {
    char c = mySerial.read();
    gpsData += c;
  }

  if (gpsData.length() > 0) 
  {
    Serial.println("Raw GPS Data:");
    Serial.println(gpsData);


    if (gpsData.indexOf("$GPGGA") != -1)
    {
      int comma1 = gpsData.indexOf(",");
      int comma2 = gpsData.indexOf(",", comma1 + 1);
      int comma3 = gpsData.indexOf(",", comma2 + 1);
      int comma4 = gpsData.indexOf(",", comma3 + 1);
      int comma5 = gpsData.indexOf(",", comma4 + 1);
      /*int comma6 = gpsData.indexOf(",", comma5 + 1);
      int comma7 = gpsData.indexOf(",", comma6 + 1);
      int comma8 = gpsData.indexOf(",", comma7 + 1);
      int comma9 = gpsData.indexOf(",", comma8 + 1);
      int comma10 = gpsData.indexOf(",", comma9 + 1);*/

      
      String latDeg = gpsData.substring(comma2 + 1, comma2 + 3);
      String latMin = gpsData.substring(comma2 + 3, comma3);
      float latitude = latDeg.toInt() + latMin.toFloat() / 60.0;

      String lonDeg = gpsData.substring(comma4 + 1, comma4 + 4);
      String lonMin = gpsData.substring(comma4 + 4, comma5);
      float longitude = lonDeg.toInt() + lonMin.toFloat() / 60.0;

      
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);

      
      String googleMapsUrl = "https://maps.google.com/maps?q=" + String(latitude, 6) + "," + String(longitude, 6);
      Serial.println("Google Maps URL:");
      Serial.println(googleMapsUrl);

      return googleMapsUrl;
    }
  }
  return "";
}

void loop() 
{  
  static bool previousSignalStatus = false;
  bool currentSignalStatus = getSignalStrength();
  
  
  if (currentSignalStatus != previousSignalStatus) 
  {
    digitalWrite(GSMLED, currentSignalStatus ? HIGH : LOW);
    previousSignalStatus = currentSignalStatus;
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  
  lcd.setCursor(0, 0);
  if (currentSignalStatus) 
  {
    lcd.print("Temp: ");
    lcd.print(t);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(h);
    lcd.print(" %");
  } 
  else 
  {
    lcd.print("Signal Lost  ");
    lcd.setCursor(0, 1);
    lcd.print("              ");
  }

  if (digitalRead(BUTTON_PIN) == LOW) 
  {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) 
    {
      Serial.println("Button pressed");
      lcd.clear();
      lcd.print("Button Pressed");

      digitalWrite(wl, HIGH);

      String message = "";
      String googleMapsUrl = getGPSData();

      if (googleMapsUrl != "") 
      {
        digitalWrite(GPSLED, HIGH);
        message += googleMapsUrl + " ";
      }
      else 
      {
        digitalWrite(GPSLED, LOW);
        message += "Out of range. ";
      }

      message += "Temp: " + String(t) + "C, Humidity: " + String(h) + "%";
      sim808.sendSMS(PHONE_NUMBER, message.c_str());
      Serial.println("SMS sent");

      
      lcd.clear();
      lcd.print("Calling...");
      if (sim808.callUp(PHONE_NUMBER)) 
      {
        Serial.println("Calling");
        delay(30000); 
        sim808.hangup();
        Serial.println("Call ended");
      }

      digitalWrite(wl, LOW);
      lcd.clear();

      while (digitalRead(BUTTON_PIN) == LOW) 
      {
        delay(10);
      }
    }
  }

  delay(1000);
}
