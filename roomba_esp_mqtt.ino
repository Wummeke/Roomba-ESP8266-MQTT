#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>
#include <Roomba.h>
#include "config.h"


//USER CONFIGURED SECTION START//
const char *hostName = "Roomba";
const int FW_VERSION = 18060503;
const int noSleepPin = 0;
const int wakeupInterval = 28000;
bool retain_value = false;
//USER CONFIGURED SECTION END//


WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
Roomba roomba(&Serial, Roomba::Baud115200);
ADC_MODE(ADC_VCC);


// Variables
bool boot = true;
long battery_Current_mAh = 0;
long battery_Voltage = 0;
long battery_Total_mAh = 0;
long battery_percent = 0;
char battery_percent_send[50];
char battery_Current_mAh_send[50];
uint8_t tempBuf[10];
char vcc[10];
IPAddress ip; 

//Functions

void setup_wifi() 
{
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostName);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
  ip = WiFi.localIP();
}

void reconnect() 
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) 
  {
    if(retries < 50)
    {
      // Attempt to connect
      if (client.connect(hostName, "roomba/status", 0, 0, "Dead Somewhere")) 
      {
        // Once connected, publish an announcement...
        if(boot == false)
        {
          client.publish("checkin/roomba", "Reconnected"); 
        }
        if(boot == true)
        {
          client.publish("checkin/roomba", "Rebooted");
          boot = false;
        }
        // ... and resubscribe
        client.subscribe("roomba/commands");
      } 
      else 
      {
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries >= 50)
    {
    ESP.restart();
    }
  }
}


void callback(const MQTT::Publish& pub) {
  String newTopic = pub.topic();
  String newPayload = pub.payload_string();
  if (newTopic == "roomba/commands") 
  {
    if (newPayload == "start")
    {
      // I noticed I need to send the start cleaning command twice when Roomba is docked, lets test if this fixes that issue
      for (int i=0; i <= 1; i++){
        startCleaning();
        delay(500);
      }
    }
    else if (newPayload == "stop")
    {
      stopCleaning();
    }
    else if (newPayload == "update")
    {
      OTAupdate();
    }
  }  
}

String getMAC()
{
  uint8_t mac[6];
  char result[14];
  WiFi.macAddress(mac);

 snprintf( result, sizeof( result ), "%02x%02x%02x%02x%02x%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );

  return String( result );
}

void OTAupdate(){
  String mac = getMAC();
  String fwURL = String( fwUrlBase );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    int newVersion = newFWVersion.toInt();

    if( newVersion > FW_VERSION ) {
      client.publish("roomba/otastatus", "Preparing to update");

      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          client.publish("roomba/otastatus", "Update failed");
          break;

        case HTTP_UPDATE_NO_UPDATES:
          client.publish("roomba/otastatus", "No updates");
          break;
      }
    }
    else {
      client.publish("roomba/otastatus", "Already on latest version");
    }
  }
  else {
	String httperrorpayload = "Firmware version check failed, got HTTP response code " + String(httpCode);
    client.publish("roomba/otastatus", httperrorpayload);
    
  }
  httpClient.end();
}

void startCleaning()
{
  roomba.start();
  delay(50);
  roomba.safeMode();
  delay(50);
  roomba.cover();
  client.publish("roomba/status", "Cleaning");
}

void stopCleaning()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(143);
  client.publish("roomba/status", "Returning");
}

void sendStatus() {
  // Flush serial buffers
  while (Serial.available()) {
    Serial.read();
  }

  uint8_t sensors[] = {
    Roomba::SensorDistance, // 2 bytes, mm, signed
    Roomba::SensorChargingState, // 1 byte
    Roomba::SensorVoltage, // 2 bytes, mV, unsigned
    Roomba::SensorCurrent, // 2 bytes, mA, signed
    Roomba::SensorBatteryCharge, // 2 bytes, mAh, unsigned
    Roomba::SensorBatteryCapacity // 2 bytes, mAh, unsigned
  };
  uint8_t values[11];

  bool success = roomba.getSensorsList(sensors, sizeof(sensors), values, 11);
  if (!success) {
    client.publish("roomba/debug", "Failed to read sensor values from Roomba");
    return;
  }
  int16_t distance = values[0] * 256 + values[1];
  uint8_t chargingState = values[2];
  uint16_t voltage = values[3] * 256 + values[4];
  int16_t current = values[5] * 256 + values[6];
  uint16_t charge = values[7] * 256 + values[8];
  uint16_t capacity = values[9] * 256 + values[10];

  bool cleaning = false;
  bool docked = false;

  if (current < -300) {
    cleaning = true;
  } else if (current > -50) {
    docked = true;
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["battery_level"] = (charge * 100)/capacity;
  root["cleaning"] = cleaning;
  root["docked"] = docked;
  root["chargetype"] = chargingState;
  root["voltage"] = voltage;
  root["current"] = current;
  root["charge"] = charge;
  String jsonStr;
  root.printTo(jsonStr);
  client.publish("roomba/state", jsonStr.c_str());
}


void send_esp_status(){
  String mac = getMAC();
  float espvoltage  = ESP.getVcc() / 1024.0;

  if (client.connected()) {
    const size_t bufferSize = JSON_OBJECT_SIZE(4);
    DynamicJsonBuffer jsonBuffer(bufferSize);
    
    JsonObject& JSONencoder = jsonBuffer.createObject();
    JSONencoder["macadress"] = mac;
    JSONencoder["firmware"] = FW_VERSION;
    JSONencoder["ipaddress"] = ip.toString();
    JSONencoder["voltage"] = espvoltage;
    
    String esp_Status;
    JSONencoder.printTo(esp_Status);
    client.publish("roomba/espstate", esp_Status);
  }
}


void setup() 
{
  Serial.begin(115200);
  Serial.write(129);
  delay(50);
  Serial.write(11);
  delay(50);
  setup_wifi();
  client.set_server(MQTT_IP, MQTT_PORT);
  client.set_callback(callback);
  timer.setInterval(5000, sendStatus);
  timer.setInterval(10001, send_esp_status);
  timer.setInterval(wakeupInterval, stayAwakeLow);
}

void stayAwakeLow()
{
  digitalWrite(noSleepPin, LOW);
  timer.setTimeout(1000, stayAwakeHigh);
}

void stayAwakeHigh()
{
  digitalWrite(noSleepPin, HIGH);
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
   
  client.loop();
  timer.run();
}


