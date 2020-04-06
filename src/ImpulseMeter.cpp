#include "ImpulseMeter.h"

ImpulseMeter::~ImpulseMeter(){
    _disableInterrupt();
}

void ImpulseMeter::begin(uint8_t counterId, unsigned int timerIntervallInSec, char const sourceName[], callback_timerIntervallElapsed_t callbackTimerIntervallElapsed){
    if(counterId >= 0 && counterId <= MAX_COUNTERS){
        _pulses_pin = gpioPins[counterId];
        _timerIntervallInSec = timerIntervallInSec;
        _sourceName = sourceName;
        _callbackTimerIntervallElapsed = callbackTimerIntervallElapsed;
        pinMode(_pulses_pin, INPUT_PULLUP);
        _calcFirstCallbackTime();
        _enableInterrupt();
    #ifdef IMPULSE_METER_DEBUG    
        if(_isrInstalled){
            Serial.printf("Pin: %02d; Source: %s; Intervall in sec: %d; NextCallbackTime: %s\n", _pulses_pin, _sourceName.c_str(), _timerIntervallInSec, FormatTime(_nextCallbackTime));
        }
        else
        {
            Serial.printf("Pin: %02d is not supported. Source: %s\n", _pulses_pin, _sourceName.c_str());
        }
    #endif
    }
    else{
        Serial.printf("CounterId: %02d is not supported. Source: %s\n", counterId, _sourceName.c_str());
    }
}

void ImpulseMeter::update(){
    while (!_impulseQueue.empty()) {
        ImpulseContainer container = _impulseQueue.front();
        _impulseQueue.pop();

        if(_callbackTimerIntervallElapsed != NULL){
            ImpulseMeterStatus impulseMeterStatus;
            impulseMeterStatus.impulse = container.impulse;
            impulseMeterStatus.sourceName = _sourceName.c_str();
            impulseMeterStatus.utcTime = container.utcTime;
            impulseMeterStatus.timerIntervallInSec = _timerIntervallInSec;
            _callbackTimerIntervallElapsed(impulseMeterStatus);
        }
    }
}

void ImpulseMeter::_calcFirstCallbackTime(){
    time_t utcTime = DateTime.utcTime();
#ifdef IMPULSE_METER_DEBUG    
    Serial.printf("Current UTC time: %s\n",FormatTime(utcTime));
#endif
   	struct tm* startTime = gmtime(&utcTime);
    if (_timerIntervallInSec < 10)
	{
		// Ist kleiner als 10 Sekunden dann minimum 10 Sekunden einstellen
		_timerIntervallInSec = 10;
		startTime->tm_sec /= 10;
		startTime->tm_sec *= 10;
	}
	else if (_timerIntervallInSec < 60)
	{
		// Ist kleiner als 1 Minunte, dann auf 10 Sekunden runden
		if (_timerIntervallInSec % 10 > 0)
		{
			_timerIntervallInSec /= 10;
			_timerIntervallInSec *= 10; // Auf 10 sekunden schritte runden
		}
		startTime->tm_sec = startTime->tm_sec / _timerIntervallInSec * _timerIntervallInSec;
	}
	else
	{
		// Groesser als 60 Sekunden
		if (_timerIntervallInSec % 60 > 0)
		{
			// Ist groesser als 1 Minunte, dann auf 60 Sekunden runden.
			_timerIntervallInSec /= 60;
			_timerIntervallInSec *= 60;
		}
		startTime->tm_sec = 0;
		startTime->tm_min = startTime->tm_min / 10 * 10;
	}

   	_nextCallbackTime = mktime(startTime) + _timerIntervallInSec;
#ifdef IMPULSE_METER_DEBUG    
    Serial.printf("Pin: %02d; First Callback Time: %s\n",_pulses_pin, FormatTime(_nextCallbackTime));
#endif
}

void ImpulseMeter::_calcNextCallbackTime()
{
	time_t dif = DateTime.utcTime() - _nextCallbackTime;
	if(dif > _timerIntervallInSec)
	{
		_nextCallbackTime = _nextCallbackTime + dif / _timerIntervallInSec * _timerIntervallInSec + _timerIntervallInSec;
	}
    else
    {
    	_nextCallbackTime = _nextCallbackTime + _timerIntervallInSec;
    }

#ifdef IMPULSE_METER_DEBUG    
    Serial.printf("Pin: %02d; Next callback time: %s\n",_pulses_pin, FormatTime(_nextCallbackTime));
#endif
}

void ImpulseMeter::_isrCallback(){
    if(DateTime.utcTime() > _nextCallbackTime){
        ImpulseContainer container;
        container.impulse = _impulse;
        container.utcTime = _nextCallbackTime;
        _impulseQueue.push(container);
        _calcNextCallbackTime();
        _impulse = 0;
    }

    _impulse++;
}

bool ImpulseMeter::_supportsInterrupt()
{
    return (digitalPinToInterrupt(_pulses_pin) != NOT_AN_INTERRUPT);
}

void ImpulseMeter::_enableInterrupt()
{
    _isrInstalled = false;

    if (!_supportsInterrupt()){
#ifdef IMPULSE_METER_DEBUG 
    Serial.printf("PIN %d is not supported\n", _pulses_pin);
#endif       
        return;        
    }

    for (uint8_t i = 0; i < MAX_PORT_COUNT; i++)
    {
        if(_instances[i] == NULL){
            _instances[i] = this;
            attachInterrupt(digitalPinToInterrupt(_pulses_pin), _isrCallbacks[i], RISING);
            #ifdef IMPULSE_METER_DEBUG 
                Serial.printf("PIN %d is installed to instance %d\n", _pulses_pin, i);
            #endif       
            _isrInstalled = true;
            break;
        }
    }

#ifdef IMPULSE_METER_DEBUG 
    if(_isrInstalled == false) Serial.printf("PIN %d is not setup because there is no free ISR function\n", _pulses_pin);
#endif       

}

void ImpulseMeter::_disableInterrupt()
{
    detachInterrupt(digitalPinToInterrupt(_pulses_pin));
    _isrInstalled = false;
}

#define ISR_CALLBACK(x) if (ImpulseMeter::_instances [x] != NULL) ImpulseMeter::_instances [x]->_isrCallback();

ImpulseMeter * ImpulseMeter::_instances[ImpulseMeter::MAX_PORT_COUNT] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
void ImpulseMeter::_isrExt0 () {ISR_CALLBACK(0)} 
void ImpulseMeter::_isrExt1 () {ISR_CALLBACK(1)}
void ImpulseMeter::_isrExt2 () {ISR_CALLBACK(2)}
void ImpulseMeter::_isrExt3 () {ISR_CALLBACK(3)}
void ImpulseMeter::_isrExt4 () {ISR_CALLBACK(4)}
void ImpulseMeter::_isrExt5 () {ISR_CALLBACK(5)}
void ImpulseMeter::_isrExt6 () {ISR_CALLBACK(6)}
void ImpulseMeter::_isrExt7 () {ISR_CALLBACK(7)}
void ImpulseMeter::_isrExt8 () {ISR_CALLBACK(8)}
void ImpulseMeter::_isrExt9 () {ISR_CALLBACK(9)}
void ImpulseMeter::_isrExt10 () {ISR_CALLBACK(10)}
void ImpulseMeter::_isrExt11 () {ISR_CALLBACK(11)}
void ImpulseMeter::_isrExt12 () {ISR_CALLBACK(12)}
void ImpulseMeter::_isrExt13 () {ISR_CALLBACK(13)}
void ImpulseMeter::_isrExt14 () {ISR_CALLBACK(14)}
void ImpulseMeter::_isrExt15 () {ISR_CALLBACK(15)}
void ImpulseMeter::_isrExt16 () {ISR_CALLBACK(16)}
void ImpulseMeter::_isrExt17 () {ISR_CALLBACK(17)}
void ImpulseMeter::_isrExt18 () {ISR_CALLBACK(18)}
void ImpulseMeter::_isrExt19 () {ISR_CALLBACK(19)}
void ImpulseMeter::_isrExt20 () {ISR_CALLBACK(20)}
void ImpulseMeter::_isrExt21 () {ISR_CALLBACK(21)}
void ImpulseMeter::_isrExt22 () {ISR_CALLBACK(22)}
void ImpulseMeter::_isrExt23 () {ISR_CALLBACK(23)}
void ImpulseMeter::_isrExt24 () {ISR_CALLBACK(24)}
void ImpulseMeter::_isrExt25 () {ISR_CALLBACK(25)}
void ImpulseMeter::_isrExt26 () {ISR_CALLBACK(26)}
void ImpulseMeter::_isrExt27 () {ISR_CALLBACK(27)}
void ImpulseMeter::_isrExt28 () {ISR_CALLBACK(28)}
void ImpulseMeter::_isrExt29 () {ISR_CALLBACK(29)}

ImpulseMeter::callback_isr_t ImpulseMeter::_isrCallbacks[ImpulseMeter::MAX_PORT_COUNT] 
    = {ImpulseMeter::_isrExt0, ImpulseMeter::_isrExt1, ImpulseMeter::_isrExt2, ImpulseMeter::_isrExt3, ImpulseMeter::_isrExt4, ImpulseMeter::_isrExt5, ImpulseMeter::_isrExt6, ImpulseMeter::_isrExt7, ImpulseMeter::_isrExt8, ImpulseMeter::_isrExt9
     , ImpulseMeter::_isrExt10, ImpulseMeter::_isrExt11, ImpulseMeter::_isrExt12, ImpulseMeter::_isrExt13, ImpulseMeter::_isrExt14, ImpulseMeter::_isrExt15, ImpulseMeter::_isrExt16, ImpulseMeter::_isrExt17, ImpulseMeter::_isrExt18, ImpulseMeter::_isrExt19
     , ImpulseMeter::_isrExt20, ImpulseMeter::_isrExt21, ImpulseMeter::_isrExt22, ImpulseMeter::_isrExt23, ImpulseMeter::_isrExt24, ImpulseMeter::_isrExt25, ImpulseMeter::_isrExt26, ImpulseMeter::_isrExt27, ImpulseMeter::_isrExt28, ImpulseMeter::_isrExt29};