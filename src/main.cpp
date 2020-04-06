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
const char SubscribeTopicCounter[] = "ESP1/Counter/#";

EspMQTTClient mqttClient(
  "PothornFunk",
  "Sguea@rbr",
  "192.168.2.20",   // MQTT Broker server ip on OpenHab PI
  "",             // Can be omitted if not needed
  "",             // Can be omitted if not needed
  MY_NAME         // Client name that uniquely identify your device
);


void installCounter(const String &message){
  logger.printMessage("Get install counter message: %s\n",message.c_str());
  vector<string> parameters = SPLIT_TAB(message.c_str());
  uint32_t counterId;
  istringstream(parameters[0]) >> counterId;
  string sourceName = parameters[1];
  uint32_t timerIntervallInSec;
  istringstream(parameters[2]) >> timerIntervallInSec;
  logger.printMessage("Get counterId: %u SourceName: %s timerIntervall %u\n", counterId, sourceName.c_str(), timerIntervallInSec);
}

void setupMqttSubscriber(){
  if(mqttClient.isConnected()){
    mqttClient.subscribe(SubscribeTopicCounter, installCounter);
  }
}
//***************** End MQTT *********************************


//***************** Begin ImpulseMeter *********************************
#define METER1_ID 0
#define METER2_ID 3
#define AUTO_TEST true              //If true, a signal will be generated in pin SIGNAL_PIN to simulate the meter 
//signal, this pin must be connected to pin METER_PIN.

ImpulseMeter meter1;
ImpulseMeter meter2;

#if AUTO_TEST
const int pwmPin = 4;  // 4 corresponds to GPIO16
// setting PWM properties
const int pwmFrequenz = 500;
const int pwmChannel = 0;
const int pwmResolution = 8;
#endif

#define UPDATE_PERIOD 25 * 1000 // 25 Sec
unsigned long _lastUpdate;

void plotImpulses(ImpulseMeterStatus status)
{
  char payload[40]; 
  logger.printMessage("%s - Source: %s; Intervall in sec: %d; Time: %s; Impulses: %lu\n", DateTime.toISOString().c_str(), status.sourceName, status.timerIntervallInSec, FormatTime(status.utcTime), status.impulse);
  snprintf(payload, sizeof(payload),"%s\t%lu", FormatTime(status.utcTime), status.impulse);
  mqttClient.publish(status.sourceName, payload, false);
}

void setupMeter() {
  meter1.begin(METER1_ID, 60,"Engergy/Wohnzimmer", plotImpulses);
  meter2.begin(METER2_ID, 60,"Engergy/Arbeit", plotImpulses);
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
}

void loop() {
  if (millis() - _lastUpdate >= UPDATE_PERIOD)
  {
    meter1.update();
    meter2.update();
    _lastUpdate = millis();
  }

  delay(100);
  mqttClient.loop();
}
