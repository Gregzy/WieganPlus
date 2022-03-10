	/*
	 * WatchDogStatus.h
	 *
	 *  Created on: Mar 31, 2014
	 *      Author: jfgregoire
	 */

	#ifndef WATCHDOGSTATUS_H_
	#define WATCHDOGSTATUS_H_

	#include"Thread.h"
	#include <vector>
	#include <iterator>

	#define NANOS 1000000000LL	//Conversion factor for nano seconds to milliseconds

	//Pulse state (see PulseP)
	#define O_NONE 	2
	#define O_ON 	1
	#define O_OFF 	0


	/***
	 * PulseP stand for Pulse Pattern element. It is part of the timed patterns that are output to the Gpio (see WDS_XXXXX below).
	 */
	class PulseP{
	public:
		PulseP( int theState = 0, unsigned long long theDelay = 0, std::string theName = "", int theId = 0);
		unsigned int GetGpioState( void){ return(myState);}
		void SetGpioState( int theState);
		unsigned long long GetDelay( void){ return( myDelay);}
		void SetDelay( unsigned int theDelay) {myDelay = theDelay;}
		std::string GetName(void){return( myPulseName);}
		int GetIdGpio( void) const {return( myId);}
		int GetId( void){return myId;}
		struct timespec GetLastActiveTime( void){ return( myLastActivityTime);}


	private:
		int myState;
		unsigned long long myDelay;
		std::string myPulseName;
		int myId;
		int myPreviousState;
		struct timespec myLastActivityTime;
	};

	/***
	 * The WatchDog Status (WDS) pattern are vectors containing Pulse object. A pulse object is simply a state (None, On, Off) and
	 * a duration. You can have one or hundreds of pulse in a pattern
	 */
	static std::vector<PulseP> WDS_WRITE_PATTERN = {PulseP(O_ON, 100), PulseP(O_OFF, 100)};
	static std::vector<PulseP> WDS_READ_PATTERN = {PulseP(O_ON, 100), PulseP(O_OFF, 100), PulseP(O_ON, 100), PulseP(O_OFF, 100)};
	static std::vector<PulseP> WDS_READ_ERROR = {PulseP(O_ON, 1000)};
	static std::vector<PulseP> WDS_WRITE_NET_CONNECT = {PulseP(O_ON, 1000)};


	/***
	 * The HwOutput class represent a GPIO Pin typically used to send a LED or Piezo pattern to the
	 * user. This HardWare Output class will store one pattern (struct OUTPUT_PATTERN). The pattern
	 * is basically a list of ON or OFF state associated with a length of time. Once the pattern is completed
	 * it will be deleted unless the repeat flag of the OUTPUT_PATTERN is true, in that case it will start
	 * over again. The pattern sequence is received and ran ran by the WatchDogStatus class.
	 */
	class HwOutput{
	public:
		HwOutput(int theGpio);
		void SendOutputPattern( std::vector<PulseP> *thePattern, bool theRepeat = false);
		void ClearOutputPattern( void);
		bool IsPattern( void){ return( (myPattern != nullptr ? true : false));}
		int GetCurrSequenceDelay( void);
		long SequenceStartTime( void){return( myElapseTime);}
		void GoNextSequence( long theElapse);
		int GetGpioInfo( void) {return myGpio;}
	private:
		pthread_mutex_t myPatternMutext;
		std::vector<PulseP> *myPattern;
		unsigned int myCurrentSequence;
		bool myRepeat;
		long long myElapseTime;
		int myGpio;
	};

	/**
	 * The WatchDogStatus class take care of confirming to the HW Watchdog that the software is not stock in a dead loop
	 * (*** to be implemented ***) but also to display the state of system transaction by outputing LED and sound pattern
	 * through the GPIOs (see also HwOutput)
	 */
	class WatchDogStatus : public Thread{
	public:
		WatchDogStatus(int theThreadPriority = 50);
		virtual ~WatchDogStatus();
		bool AddHwOutput(int theGpio);

		bool SendOutputSignal( int theOutput, std::vector<PulseP> *thePattern, bool theReapeat = false);
		bool StopOutputSignal( int theOutput);
		void StopAllOutputSignal( void);

	protected:
		void OnStart();
		void OnStop();
		void Execute(void*);
	private:
		long GetMillis( struct timespec theTime);
		std::vector<HwOutput> myHdwOutList;
		pthread_mutex_t myWatchDogLock;
		int myThreadPriority;
	};

	#endif /* WATCHDOGSTATUS_H_ */
