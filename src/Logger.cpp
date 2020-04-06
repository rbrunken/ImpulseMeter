#include <stdio.h>
#include "Logger.h"

void Logger::begin(char *myName, EspMQTTClient *mqttClient){
    if(myName == NULL || mqttClient == NULL){
        return;
    }

    _mqttClient = mqttClient;
    _errorTopic += myName;
    _messageTopic += myName;
}

size_t Logger::printError(const char *format, ...){
    char loc_buf[100];
    char * temp = loc_buf;
    va_list arg;
    va_start(arg, format);
    int len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(arg);
    if(len < 0 || len >= sizeof(loc_buf)) {
        return 0;
    };

    Serial.write((uint8_t*)temp, len);
    Serial.println();
    errorToMqtt(temp, len);
    return len;
}

size_t Logger::printMessage(const char *format, ...){
    char loc_buf[100];
    char * temp = loc_buf;
    va_list arg;
    va_start(arg, format);
    int len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(arg);
    if(len < 0 || len >= sizeof(loc_buf)) {
        return 0;
    };

    Serial.write((uint8_t*)temp, len);
    Serial.println();
    messageToMqtt(temp, len);
    return len;
}

void Logger::messageToMqtt(char *buff, size_t len){
    if(_mqttClient != NULL && _mqttClient->isConnected()){
        _mqttClient->publish(_messageTopic.c_str(), buff);
    }
}

void Logger::errorToMqtt(char *buff, size_t len){
    if(_mqttClient != NULL && _mqttClient->isConnected()){
        _mqttClient->publish(_errorTopic.c_str(), buff);
    }
}