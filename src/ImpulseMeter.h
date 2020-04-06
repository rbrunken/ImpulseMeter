#ifndef IMPULSE_METER_H
#define IMPULSE_METER_H
#include <Arduino.h>
#include <queue>
#include <MyDateTime.h>
#include "Logger.h"

//#define IMPULSE_METER_DEBUG

// Status of the impulse meter.
struct ImpulseMeterStatus
{
	time_t utcTime;						// The end time in UTC of the collected impulses
	unsigned long impulse;				// The collected impulses
	const char*  sourceName;			// The name of the impulse source
	unsigned int timerIntervallInSec;	// The intervall of the collection
};

// The mapping of the CounterId to the GPIO Pin. The Index is the CounterID and the value is the GPIO Pin.
const static int MAX_COUNTERS = 22;
const static int gpioPins[MAX_COUNTERS] = {2, 4, 5, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};

// Collect impulses from a GPIO Pin for a specific time intervall and give the collect impulses and 
// the end time of the collection back to a callback function. After that starts a new collection of impulses.
class ImpulseMeter
{
public:
	typedef void(*callback_timerIntervallElapsed_t)(ImpulseMeterStatus impulseMeterStatus);

	//**** ctors / destructor
    ~ImpulseMeter();

	//**** user functions
	// Setup the instance
	void begin(uint8_t counterId, unsigned int timerIntervallInSec, char const sourceName[], callback_timerIntervallElapsed_t callbackTimerIntervallElapsed, Logger* logger);
	// Check if the  callback function should be called and if necessary call it with the collected impules.
	// update should be called in the loop() function of main.c													
	void update();													

private:
	const static int MAX_PORT_COUNT = 30;
    typedef void(*callback_isr_t)();
	// Store the impulses and time of a given time intervall.
	struct ImpulseContainer
	{
		time_t utcTime;						// The end time in UTC of the collected impulses
		unsigned long impulse;				// The collected impulses
	};

	uint8_t _pulses_pin;											// Pin from wich the impulses will be get from.
    bool _isrInstalled;                                             // True, if the ISR is enabled
    volatile unsigned long _impulse;		    					// Impulse recived since start or last call of the timer intervall elapsed callback function
	time_t _nextCallbackTime;										// Time to call the timer intervall elapsed callback
    unsigned int _timerIntervallInSec;                              // The intervall to call the timer intervall elapsed callback function
    String _sourceName;                                             // The name of this impulse source
	std::queue<ImpulseContainer> _impulseQueue;

	Logger* _logger;

	//functions
	// Check if the GPIO supports a interrupt
	bool _supportsInterrupt();
	// Calculate the next callback time with the current time and the timer intervall.
	void _calcNextCallbackTime();
	// Calculate the first callback time with the current time and the timer intervall.
	void _calcFirstCallbackTime();

	//**** interrupt functions
	// Enable interrupt if pin allow it
	void _enableInterrupt();
	// Disable interrupt
	void _disableInterrupt();

	// Callback for the ISR.
    void _isrCallback();
    // Callback of the client of this instance.
    callback_timerIntervallElapsed_t _callbackTimerIntervallElapsed;

	// ISR function can´t be a member function, therfor use this static extended functions.
	// The static extended ISR functions.
	static void _isrExt0();
	static void _isrExt1();
	static void _isrExt2();
	static void _isrExt3();
	static void _isrExt4();
	static void _isrExt5();
	static void _isrExt6();
	static void _isrExt7();
	static void _isrExt8();
	static void _isrExt9();
	static void _isrExt10();
	static void _isrExt11();
	static void _isrExt12();
	static void _isrExt13();
	static void _isrExt14();
	static void _isrExt15();
	static void _isrExt16();
	static void _isrExt17();
	static void _isrExt18();
	static void _isrExt19();
	static void _isrExt20();
	static void _isrExt21();
	static void _isrExt22();
	static void _isrExt23();
	static void _isrExt24();
	static void _isrExt25();
	static void _isrExt26();
	static void _isrExt27();
	static void _isrExt28();
	static void _isrExt29();

	// A array with the installed instances of this class which is called by the static extended ISR function.
	static ImpulseMeter * _instances[MAX_PORT_COUNT];
	// A array with the static extended ISR functions.
	static callback_isr_t _isrCallbacks[MAX_PORT_COUNT];
};

#endif 