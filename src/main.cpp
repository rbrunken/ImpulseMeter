#include <array>
#include <vector>
#include <sstream>
#include <Arduino.h>
#include <EspMQTTClient.h>
#include <MyDateTime.h>
#include "ImpulseMeter.h"
#include "Logger.h"

using namespace std;

#define MY_NAME "ESP1"

unsigned long impulsesOverAll;

void plotImpulses(ImpulseMeterStatus status);

vector<string> split (const string &s, char delim) {
    vector<string> result;
    stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

Logger logger;

#define SPLIT_TAB(x) split(x,'\t')

std::array<ImpulseMeter*,MAX_COUNTERS> impulseMeters;

//***************** Begin MQTT *********************************

EspMQTTClient mqttClient(
  "PothornFunk",
  "Sguea@rbr",
  "192.168.2.20",   // MQTT Broker server ip on OpenHab PI
  "",             // Can be omitted if not needed
  "",             // Can be omitted if not needed
  MY_NAME         // Client name that uniquely identify your device
);

void restartHandler(const String &message){
  ESP.restart();
}

void getStatusHandler(const String &message){
  int counters = 0;
  for (size_t i = 0; i < MAX_COUNTERS; i++)
  {
    if(impulseMeters[i] != NULL){
      counters++;
    }
  }

  char buff[80];
  snprintf(buff, 80, "%s\t%s\t%d\t%lu", FormatTime(DateTime.getTime()), FormatTime(DateTime.getBootTime()), counters, impulsesOverAll);
  string topic = "Status/";
  topic += MY_NAME;
  mqttClient.publish(topic.c_str(), buff);
}

// Install a counter to get the impulses.
// The message has a ID, SourceName and a intervall separated by TAB
void installCounterHandler(const String &message){
  try
  {
    vector<string> parameters = SPLIT_TAB(message.c_str());
    uint32_t counterId;
    istringstream(parameters[0]) >> counterId;
    string sourceName = parameters[1];
    uint32_t timerIntervallInSec;
    istringstream(parameters[2]) >> timerIntervallInSec;
    ImpulseMeter* existingImpulseMeter = impulseMeters[counterId];
    if(existingImpulseMeter != NULL){
      existingImpulseMeter->begin(counterId, timerIntervallInSec, sourceName.c_str(), plotImpulses, &logger);
      logger.printMessage("Update counterId: %u SourceName: %s timerIntervall %u\n", counterId, sourceName.c_str(), timerIntervallInSec);
    }else
    {
      ImpulseMeter* impulseMeter = new ImpulseMeter();
      impulseMeter->begin(counterId, timerIntervallInSec, sourceName.c_str(), plotImpulses, &logger);
      impulseMeters[counterId] = impulseMeter;
      logger.printMessage("Add counterId: %u SourceName: %s timerIntervall %u\n", counterId, sourceName.c_str(), timerIntervallInSec);
    }
  }
  catch(const std::exception& e)
  {
    logger.printError("Failed to install the counter: '%s'::  %s", message, e.what());
  }
}

void setupMqttSubscriber(){
  if(mqttClient.isConnected()){
    string myName = MY_NAME;
    string topic = myName + "/InstallCounter";
    mqttClient.subscribe(topic.c_str(), installCounterHandler);
    topic = myName + "/Restart";
    mqttClient.subscribe(topic.c_str(),restartHandler);
    topic = myName + "/GetStatus";
    mqttClient.subscribe(topic.c_str(),getStatusHandler);
  }
}
//***************** End MQTT *********************************


//***************** Begin ImpulseMeter *********************************
#define AUTO_TEST true              //If true, a signal will be generated in pin SIGNAL_PIN to simulate the meter 
//signal, this pin must be connected to pin METER_PIN.

#if AUTO_TEST
const int pwmPin = 4;  // 4 corresponds to GPIO16
// setting PWM properties
const int pwmFrequenz = 500;
const int pwmChannel = 0;
const int pwmResolution = 8;
#endif

#define UPDATE_PERIOD 1 * 1000 // 1 Sec
unsigned long _lastUpdate;

void plotImpulses(ImpulseMeterStatus status)
{
  char payload[40]; 
  logger.printMessage("%s - Source: %s; Intervall in sec: %d; Time: %s; Impulses: %lu\n", DateTime.toISOString().c_str(), status.sourceName, status.timerIntervallInSec, FormatTime(status.utcTime), status.impulse);
  snprintf(payload, sizeof(payload),"%s\t%lu", FormatTime(status.utcTime), status.impulse);
  mqttClient.publish(status.sourceName, payload, false);
  impulsesOverAll += status.impulse;
}

void setupMeter() {
#if AUTO_TEST
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, pwmFrequenz, pwmResolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(pwmPin, pwmChannel);
  ledcWrite(pwmChannel, 125);
#endif
}
//***************** End ImpulseMeter *********************************

void setupDateTime() {
  // setup this after wifi connected
  // Set TimeZone to UTC
  DateTime.setTimeZone(0);
  // this method config ntp and wait for time sync
  // default timeout is 10 seconds
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid()) {
    logger.printError("Failed to get time from server.");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  logger.begin(MY_NAME, &mqttClient);
  // Clear the array with the impulse meters.
  impulseMeters.fill(NULL);
  // Optionnal functionnalities of EspMQTTClient :
  mqttClient.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  mqttClient.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  mqttClient.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {
  setupDateTime();
  if(DateTime.isTimeValid() == false){
    logger.printError("Date/Time is not valid, restart the board...");
    ESP.restart();
  }

  logger.printMessage("Current UTC time: %s\n",DateTime.toISOString().c_str());
  setupMeter();
  setupMqttSubscriber();
  string topic = "GetCounterConfig/";
  topic += MY_NAME;
  mqttClient.publish(topic.c_str(),"", true);
}

void loop() {
  if (millis() - _lastUpdate >= UPDATE_PERIOD)
  {
    for (size_t i = 0; i < MAX_COUNTERS; i++)
    {
      if(impulseMeters[i] != NULL){
        impulseMeters[i]->update();
      }
    }
    
    _lastUpdate = millis();
  }

  delay(100);
  mqttClient.loop();
}
