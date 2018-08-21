/*
 *  ESP8266 Momentary Standalone Relay Switch with MQTT
 *  2018, Ron Holmes
 *
 * debounce from http://blog.erikthe.red/2015/08/02/esp8266-button-debounce/
 * mqtt client from https://github.com/knolleary/pubsubclient/tree/master/examples/mqtt_esp8266
 * Arduino OTA from https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
 * 
 *
 * system state on <mqttTopicPrefix>status (retained)
 * system ip on <mqttTopicPrefix>/ip (retained)
 * current switch state on <mqttTopicPrefix>/state
 * send 1/0 to <mqttTopicPrefix>/do to switch (retained)
*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library
#include <PubSubClient.h>
#include <Bounce2.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include "EspMqttSwitch.h"
#include "devicesettings.h"
// Now allow for serial debugging
#define debug
//--------- Configuration
// WiFi Settings
const char* ssid1 = SSID1;
const char* password1 = PASSWORD1;
const char* ssid2 = SSID2;
const char* password2 = PASSWORD2;
// MQTT
const char* mqttServer = MQTTSERVER;
const char* mqttUser = MQTTUSER;
const char* mqttPass = MQTTPASS;
const int mqtt_identity_address = 200;
char* mqttClientName = MQTTCLIENTNAME; //will also be used hostname and OTA name

// I/O - change the pins as required
const int numSwitches = 2;

const int btnPin1 = BUTTONPIN1;
const int relayPin1 = RELAYPIN1;
char* devicename1 = DEVICE1;

const int btnPin2 = BUTTONPIN2;
const int relayPin2 = RELAYPIN2;
char* devicename2 = DEVICE2;


// internal vars
ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266 WiFiMulti class, called 'wifiMulti'
WiFiClient espClient;           // And WiFi Client
PubSubClient client(espClient); // Subscribe to MQTT

char mqttUpdateTopic[64];       // the topic for updating the device
char mqttStatusTopic[64];       // The status and IP are per the device
char mqttIpTopic[64];
char controllerName[16];
char defaultControllerName[16];
char* strtmp = "xxxxxxxxxx";

// my new switches

EspMqttSwitch switch1(&client, controllerName, devicename1, btnPin1, relayPin1); //Lets have one of my new switches
EspMqttSwitch switch2(&client, controllerName, devicename2, btnPin2, relayPin2); // and another

long lastReconnectAttempt = 0; //For the non blocking mqtt reconnect (in millis)

void setup() {
  EEPROM.begin(512);              // Begin eeprom to store on/off state
  Serial.begin(115200);

// Put switches into last state
  switch1.setCurrentState(EEPROM.read(0));    // Read state from EEPROM
  switch2.setCurrentState(EEPROM.read(1));
  
  //set up mqtt identity
  // first get a default from the chip id
  sprintf(strtmp, "%08X", ESP.getChipId());
  int lenstr = strlen(strtmp);
  for (int i = 0; i <= lenstr ; i++) {
    defaultControllerName[i] = strtmp[i];
  }

  // then initialise the topic from this OR one stored in EEPROM
  setup_mqtt_topic();
  // and initialise the switches accordingly
  // and finally device status and IP
  sprintf(mqttUpdateTopic, "%s/updatename", defaultControllerName);
  sprintf(mqttStatusTopic, "%s/status", defaultControllerName);
  sprintf(mqttIpTopic, "%s/ip", defaultControllerName);

  setup_wifi();

  client.setServer(mqttServer, 1883); //
  client.setCallback(mqttCallback);

  //----------- OTA
  ArduinoOTA.setHostname(mqttClientName);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    delay(1000);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("WiFi and OTA ready...");
}

void loop() {

  //handle mqtt connection, non-blocking
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (MqttReconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
  client.loop(); // and now loop

  //handle state change

    if (switch1.checkState()) { // if we have changed the state
      EEPROM.write(0, switch1.getCurrentState());    // Write state to EEPROM

#ifdef debug
      Serial.print("Switch 1 state change to ");
      Serial.println(switch1.getCurrentState());
#endif
    }
    if (switch2.checkState()) { // if we have changed the state
      EEPROM.write(1, switch2.getCurrentState());    // Write state to EEPROM
#ifdef debug
      Serial.print("Switch 2 state change to ");
      Serial.println(switch2.getCurrentState());
#endif
    }
    EEPROM.commit();

  //handle OTA
  ArduinoOTA.handle();

}

void setup_wifi() {
  delay(10);
  Serial.println('\n');

  wifiMulti.addAP(ssid1, password1);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP(ssid2, password2);

  Serial.println("Connecting ...");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    for (int i = 0; i < 50; i++) {
      delay(10);
      Serial.print('.');
      switch1.checkState();                   // check the local buttons
      switch2.checkState();
    }

  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
}


bool MqttReconnect() {

  if (!client.connected()) {

    Serial.print("Attempting MQTT connection...");

    // Attempt to connect with last will retained
    if (client.connect(mqttClientName, mqttUser, mqttPass, mqttStatusTopic, 1, true, "offline")) {

      Serial.println("connected");

      // Once connected, publish an announcement...
      char curIp[16];
      sprintf(curIp, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      // tell serial what we are publishing to:
      Serial.print("Default Device Topic: ");
      Serial.println(defaultControllerName);
      
      Serial.print("Publishing to: ");
      Serial.print(mqttStatusTopic);
      Serial.print(" - on IP: ");
      Serial.print(curIp);
      Serial.print(" - ");
      Serial.print(switch1.getStateTopic());
      Serial.print(" - ");
      Serial.print(switch2.getStateTopic());

      Serial.println("");
      client.publish(mqttStatusTopic, "online", true);     
      client.publish(mqttIpTopic, curIp, true);
      
      // TODO correct this for two buttons
      switch1.mqttPublishState();
      switch2.mqttPublishState();

      // ... and (re)subscribe
      client.subscribe(mqttUpdateTopic);
      switch1.mqttSubscribe();
      switch2.mqttSubscribe();
      
#ifdef debug
      Serial.print("subscribed to ");
      Serial.print(mqttUpdateTopic);
      Serial.print (" and ");
      Serial.print(switch1.getDoTopic());
      Serial.print (" and ");
      Serial.print (switch2.getDoTopic());
      Serial.println("");
#endif
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for (int i = 0; i < 5000; i++) {
        switch1.checkState();
        switch2.checkState();
        delay(1);
      }
    }
  }
  return client.connected();
}

void setup_mqtt_topic() {
  if (EEPROM.read(mqtt_identity_address) == 0) {  // check to see if name stored in EEPROM
    strcpy(controllerName, defaultControllerName);
  } else {                                        // otherwise - use the chip id
//    strcpy(mqttTopic, defaultControllerName);
    strcpy(controllerName, defaultControllerName); 
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // TODO complete this
#ifdef debug
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.print(switch1.getDoTopic());
  Serial.println(" is to be matched");
#endif
  if (strcmp(topic, mqttUpdateTopic) == 0) {
    // TODO update the mqttTopic and store in EEProm
  }


  if (strcmp(topic, switch1.getDoTopic())== 0) {
    switch1.setWantedState(((char)payload[0] == 1) ? true : false );

#ifdef debug
    Serial.print("Switch 1 request: ");
    Serial.println(switch1.getWantedState());
#endif
  }
  if (strcmp(topic, switch2.getDoTopic())== 0) {
    switch2.setWantedState(((char)payload[0] == 1) ? true : false );

#ifdef debug
    Serial.print("Switch 2 request: ");
    Serial.println(switch2.getWantedState());
#endif
  }
}
