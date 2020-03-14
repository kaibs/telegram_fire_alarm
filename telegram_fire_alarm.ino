#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

//credentials
#include "credentials.h"

RCSwitch sender = RCSwitch();

//Initialize Temp-Sensor
#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
char temperatureString[6];



//Initialize other Sensors
int fireSensor = 2;
int smokeSensor = 16;
int relaisLED = 14;

//Flags for Sensor-Warnings
boolean flagTemp = false;
boolean flagSmoke = false;
boolean flagFire = false;

//Flag LED-Strip
boolean ledStatus = false;

//Flag Funksteckdosen
boolean AnetStatus = false;
boolean HypercubeStatus = false;

// Initialize Wifi
char ssid[] = WIFI_SSID;     // your network SSID (name)
char password[] = WIFI_PASSWORD; // your network key

// Initialize Telegram BOT
#define BOTtoken TELEGRAM_TOKEN;
//Own Telegram-ID
String defaultId = TELEGRAM_ID;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done
bool Start = false;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    if (text == "/Temperatur"){
      float temperature = getTemperature();
      bot.sendMessage(chat_id, String(temperature) + "°C", "");
    }

    if(text == "/ledon"){
      if(ledStatus == false){
        digitalWrite(relaisLED, HIGH);
        ledStatus = true;
        bot.sendMessage(chat_id, "LED angeschaltet", "");
      }
      else{
        bot.sendMessage(chat_id, "LED ist bereits angeschaltet", "");
      }
    }

    if(text == "/ledoff"){
      if(ledStatus == true){
        digitalWrite(relaisLED, LOW);
        ledStatus = false;
        bot.sendMessage(chat_id, "LED ausgeschaltet", "");
      }
      else{
        bot.sendMessage(chat_id, "LED ist bereits ausgeschaltet", "");
      }
    }

    if (text == "/Anet_on"){
      if(AnetStatus == false){
        sender.sendTriState("0FFF0FFFFFFF");
        AnetStatus = true;
        bot.sendMessage(chat_id, "Anet angeschaltet");
      }
      else{
        bot.sendMessage(chat_id, "Anet ist bereits angeschaltet");
      }
    }

  if (text == "/Anet_off"){
      if(AnetStatus == true){
        sender.sendTriState("0FFF0FFFFFF0");
        AnetStatus = false;
        bot.sendMessage(chat_id, "Anet ausgeschaltet");
      }
      else{
        bot.sendMessage(chat_id, "Anet ist bereits ausgeschaltet");
      }
    }

    if (text == "/Hypercube_on"){
      if(HypercubeStatus == false){
        sender.sendTriState("0FFFF0FFFFFF");
        HypercubeStatus = true;
        bot.sendMessage(chat_id, "Hypercube angeschaltet");
      }
      else{
        bot.sendMessage(chat_id, "Hypercube ist bereits angeschaltet");
      }
    }

    if (text == "/Hypercube_off"){
      if(HypercubeStatus == true){
        sender.sendTriState("0FFFF0FFFFF0");
        HypercubeStatus = false;
        bot.sendMessage(chat_id, "Hypercube ausgeschaltet");
      }
      else{
        bot.sendMessage(chat_id, "Hypercube ist bereits ausgeschaltet");
      }
    }
    
    

    

    if (text == "/options") {
      String keyboardJson = "[[\"/Temperatur\"] , [\"/ledon\", \"/ledoff\"] , [\"/Hypercube_on\", \"/Hypercube_off\"] , [\"/Anet_on\", \"/Anet_off\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson, true);
    }
  

    
   if (text == "/start") {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
      welcome += "Brandmelde-Bot\n\n";
      welcome += "/Temperatur : Gibt aktuelle Temperatur zurück\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void setup() {
  
  pinMode(smokeSensor, INPUT);
  pinMode(fireSensor, INPUT);
  pinMode(relaisLED, OUTPUT);
  
  
  Serial.begin(115200);

  DS18B20.begin();  

  //transmitter settings
  sender.enableTransmit(4);  // An Pin 3

  sender.setProtocol(1);
  sender.setPulseLength(426);


  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

float getTemperature() {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

void loop() {
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
 float temperature = getTemperature();
 Serial.println("Sensorwert");
 dtostrf(temperature, 2, 2, temperatureString);
 // send temperature to the serial console
 Serial.println(temperatureString);

 if(flagTemp == false && temperature >= 40){
  Serial.println("Temperaturwarnung");
  bot.sendMessage(defaultId, "Temperaturwarnung: " + String(temperature) + "°C" , "");
  flagTemp = true;
 }

if(flagTemp == true && temperature < 40){
  flagTemp = false;
 }

 if(flagSmoke == false && digitalRead(smokeSensor) == HIGH){
  Serial.println("Rauchwarnung!");
  bot.sendMessage(defaultId, "Rauchwarnung ", "");
  flagSmoke = true;
 }

 if(flagSmoke == true && digitalRead(smokeSensor) == LOW){
  flagSmoke = false;
 }

if(flagFire == false && digitalRead(fireSensor) == HIGH){
  Serial.println("Feuerentwicklung!");
  bot.sendMessage(defaultId, "Feuerwarnung ", "");
  flagFire = true;
 }

 if(flagFire == true && digitalRead(fireSensor) == LOW){
  flagFire = false;
 }
  
}



