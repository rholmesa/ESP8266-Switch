/*
 * Implementation of new class
 */
 #include <Arduino.h>
// #include <ESP8266WiFi.h>
 #include <PubSubClient.h>
 #include <Bounce2.h>
 #include "EspMqttSwitch.h"

  EspMqttSwitch::EspMqttSwitch(PubSubClient* client, char* controllerName, char* deviceName, int btnPin, int relayPin)
   {
    _btnPin = btnPin;
    _relayPin = relayPin;
    _currentState = false;
    _wantedState = false;
    _deviceName = deviceName;
    _controllerName = controllerName;
    _client = client;
    // now set up buttons and pins
    pinMode(btnPin, INPUT_PULLUP);
    pinMode(relayPin, OUTPUT);
    
    // and MQTT comms topics 
    sprintf(_stateTopic, "%s/%s/state", _controllerName, _deviceName);
    sprintf(_doTopic, "%s/%s/do", _controllerName, _deviceName);

    // now debounce the pin    
    _debouncer.attach(btnPin);
   }
   void EspMqttSwitch::setMqttClient(PubSubClient* client) {    
    _client = client;
   }
   
   void EspMqttSwitch::mqttSubscribe() {    
    _client->subscribe(_doTopic);
   }
   void EspMqttSwitch::mqttPublishState() {
    _client->publish(_stateTopic, (_currentState ? "1" : "0"));
   }
   void EspMqttSwitch::setCurrentState(bool state) {
    _currentState = state;
    _wantedState = _currentState;
    digitalWrite(_relayPin, _currentState);
   }

   char* EspMqttSwitch::getDeviceName() {
    return _deviceName;
   }

   char* EspMqttSwitch::getDoTopic() {
      return _doTopic;
   }

   char* EspMqttSwitch::getStateTopic() {
      return _stateTopic;
   }
   
   void EspMqttSwitch::setDeviceName(char* devicename) {
    //TODO set a new device name
   }
   char* EspMqttSwitch::getControllerName() {
    return _controllerName;
   }

   void EspMqttSwitch::setControllerName(char* devicename) {
    //TODO set a new device name
   }

   bool EspMqttSwitch::getCurrentState(){
      return _currentState;
    };
 
   void EspMqttSwitch::setWantedState(bool state){
      _wantedState = state;
    };
    
   bool EspMqttSwitch::getWantedState(){
      return _wantedState;
    };
  /*
   * This routine checks for a local button press. The button press is checked in the main
   * program and just an indication of a state change is sent here.
   */
  bool EspMqttSwitch::checkState() {
     _debouncer.update();     
     // Call code if Bounce fell (transition from HIGH to LOW) :
     if ( _debouncer.fell() ) {
       Serial.println("Button Pressed");
       // Toggle relay state :
       _currentState = !_currentState;
       _wantedState = _currentState;
       digitalWrite(_relayPin,_currentState);
       _client->publish(_stateTopic, (_currentState ? "1" : "0"));
       return true;
     } else {
        // The local button has not been pressed
        // check for mqtt triggers HORRIBLE XOR in cpp
        if (((_currentState == false) && (_wantedState == true)) || (( _currentState == true) && (_wantedState == false))) {
          _currentState = _wantedState;
          digitalWrite(_relayPin, _currentState);
          _client->publish(_stateTopic, (_currentState ? "1" : "0"));
          return true;  // and inform that the state has changed
        }
        return false; // nothing has changed
     }
  };
  // now functions to return the button pin  
  int EspMqttSwitch::getBtnPin() {
    return _btnPin;
  }

  int EspMqttSwitch::getRelayPin() {
    return _relayPin;
  }


