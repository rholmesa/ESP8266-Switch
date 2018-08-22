/* 
 *  rons_multi_mqtt_switch.h
 *  Define a class for switch with mqtt and an override facility
 *  This class defines a 'switch' with local override, but which can be switched via mqtt
 *  This change is of documentation only for GIT Test
 */
 #ifndef EspMqttSwitch_h
 #define EspMqttSwitch_h

 #include <Arduino.h>
 #include <ESP8266WiFi.h>
 #include <Bounce2.h>

 class EspMqttSwitch
 {
  public:
    EspMqttSwitch(PubSubClient* client, char* controllerName, char* deviceName, int btnPin, int relayPin);
    void setCurrentState(bool state);
    bool getCurrentState();
    void setWantedState(bool state);
    bool getWantedState();
    void setControllerName(char* controllername);
    char* getControllerName();
    void setMqttClient(PubSubClient* client);
    void mqttSubscribe();
    void mqttPublishState();
    char* getDeviceName();
    void setDeviceName(char* devicename);
    char* getDoTopic();
    char* getStateTopic();
    bool checkState();
    int getBtnPin();
    int getRelayPin();

  private:
    int _btnPin;
    int _relayPin;
    char* _deviceName;
    char* _controllerName;
    char _doTopic[48];
    char _stateTopic[48];
    bool _currentState;
    bool _wantedState;
    Bounce _debouncer;
    PubSubClient* _client;           // And an MQTT Client
 };
 #endif
 

