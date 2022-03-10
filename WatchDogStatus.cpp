/*
 * WatchDogStatus.cpp
 *
 *  Created on: Mar 31, 2014
 *      Author: jfgregoire
 */
#include <wiringPi.h>
#include <unistd.h>
#include <algorithm>
#include "WatchDogStatus.h"
#include <iostream>

using namespace std;

/***
 * PulseP constructor (and only member).
 * @param thePattern - int representing the pattern state (None, Off, On)
 * @param theDelay - int delay in milliseconds
 * @param thePulseName - Optional name given to this pulse (primarily use by Wiegand state)
 */
PulseP::PulseP(  int theState, unsigned long long theDelay, string thePulseName, int theId): myState(theState),
		myDelay(theDelay), myPulseName(thePulseName), myId(theId), myPreviousState(theState) {

	clock_gettime(CLOCK_MONOTONIC, &myLastActivityTime);
}

void PulseP::SetGpioState( int theState){
	myState = theState;
	clock_gettime(CLOCK_MONOTONIC, &myLastActivityTime);
}

/***
 * Constructor for the WatchDogStatus.
 * @param theThreadPriority - Int specify the priority of this thread. The default is 50 raising the thread above regular process (20)
 */
WatchDogStatus::WatchDogStatus( int theThreadPriority)  : myThreadPriority(theThreadPriority){

	myWatchDogLock = PTHREAD_MUTEX_INITIALIZER;

}

/***
 * Add an Hardware output definition (Gpio). This hardware output will be channeling output patterns.
 * @param theGpio - int the Gpio Id
 * @return - Return false if the Gpio has already been assigned
 */
bool WatchDogStatus::AddHwOutput( int theGpio){

	bool myGpioAlreadyExist = false;
	//Make sure we don't already have this Gpio setup
	std::for_each(myHdwOutList.begin(), myHdwOutList.end(), [&](HwOutput &i){
				if( i.GetGpioInfo() == theGpio){
					myGpioAlreadyExist = true;
				}
	});

	if( myGpioAlreadyExist)
		return(false); //Return that we did not add the hardware (false)

	pthread_mutex_lock(&myWatchDogLock);
	HwOutput myNewHwd(theGpio);
	myHdwOutList.push_back( myNewHwd);
	pthread_mutex_unlock(&myWatchDogLock);

	return(true);

}

/***
 * OnStart is automatically call by the parent class (Thread) when the thread is started,
 * Here we will only set our thread priority
 */
void WatchDogStatus::OnStart()
{
	int myResult = 0;

	myResult = this->SetThreadPriority(myThreadPriority);

	if( myResult == -1){
		//todo Throw an error instead of exiting like this
		cerr << "Error: Failed to assigned higher process priority";
		exit(1);

	}

}

/***
 * Stop() is called by the class Thread parent when it is stopped. Here we will make sure we stop any
 * output activity (led, piezo, ...)
 */
void WatchDogStatus::OnStop(){

	//Make sure we turn off everything before we exit the app
    this->StopAllOutputSignal();

}

/***
 * This generic method is called by the parent thread class when the thread start execution. This is our
 * main loop process. Here we will cycle the HdwOutputList vector containing the definition of all our Gpio
 * and check if there is a new Pattern to ouput.
 * @param arg - Argument passed by the thread creation
 */
void WatchDogStatus::Execute(void* arg)
{


    while(!myTerminateApp){
    	/*Cycle through all hardware output definition (if any), check if there is any
    	active pattern. If there is, check if we are ready to go to the next pulse state.*/
    	std::for_each(myHdwOutList.begin(), myHdwOutList.end(), [&](HwOutput &i){
    				struct timespec now;
    				clock_gettime(CLOCK_MONOTONIC, &now);
    				if( i.IsPattern()){
    					//Calculate the elapse time for this pulse within a pattern. If we are done with pass to the next
    					if( ( GetMillis(now) - i.SequenceStartTime()  ) > i.GetCurrSequenceDelay()){
    						i.GoNextSequence(GetMillis(now));
    					}
    				}
		});

    	//We yield for 25 milliseconds since we are now a critical process
		delay(25);
    }

}

/***
 * Submit a new output pattern to an hardware output
 * @param theOutput - Which output we want to send the pattern
 * @param thePattern - What static vector pattern we want to send
 * @param theRepeat - If true, repeat the pattern indifinitely (or until an new pattern is submitted)
 */
bool WatchDogStatus::SendOutputSignal( int theOutput, std::vector<PulseP> *thePattern, bool theRepeat){

	bool myOutputSentStatus = false;
	//Make sure we don't already have this Gpio setup
	std::for_each(myHdwOutList.begin(), myHdwOutList.end(), [&](HwOutput &i){
		if( i.GetGpioInfo() == theOutput){
			i.SendOutputPattern(thePattern, theRepeat);
			myOutputSentStatus = true;
		}
	});

	return(myOutputSentStatus);

}

/***
 * Stopped (and clear) the specified output
 * @param theOutDestination - The hardware ouput we want to stop and cleart the pattern
 */
bool WatchDogStatus::StopOutputSignal( int theOutDestination){
	if( myHdwOutList.size() >= (unsigned int)theOutDestination + 1)
		myHdwOutList[theOutDestination].ClearOutputPattern();

	bool myOutputSentStatus = false;
	//Make sure we don't already have this Gpio setup
	std::for_each(myHdwOutList.begin(), myHdwOutList.end(), [&](HwOutput &i){
		if( i.GetGpioInfo() == theOutDestination){
			i.ClearOutputPattern();
			myOutputSentStatus = true;
			return;
		}
	});

	return(myOutputSentStatus);


}

/***
 * Stop and clear all output pattern. This is typically use when we stop the software
 */
void WatchDogStatus::StopAllOutputSignal( void){

	std::for_each(myHdwOutList.begin(), myHdwOutList.end(), [&](HwOutput &i){
				if( i.IsPattern()){
					i.ClearOutputPattern();
				}
	});
}

/***
  * Tool to convert a timespec structure in milliseconds
 * @param theTime - The time structure to convert
 * @return - The time in milliseconds
 */
long WatchDogStatus::GetMillis( struct timespec theTime){
	return(theTime.tv_sec * 1000) + (theTime.tv_nsec / 1000000);
}


WatchDogStatus::~WatchDogStatus() {

	//This version does not currently allocate dynamically the vector output. Eventually we
	// would delete it here.

}


/***
 * This is the constructor for the Hardware Gpio abstraction
 * @param theGpio - The id of the Gpio. The Id is using the WiringPi numbering scheme (not the Broadcom board definition)
 */
HwOutput::HwOutput(int theGpio):myGpio(theGpio){

	myCurrentSequence = 0;
	myElapseTime = 0;
	myRepeat = false;
	myPattern = nullptr;

    pinMode(myGpio, OUTPUT);

    //By default, we turn off the LED
    digitalWrite(myGpio, LOW);

    myPatternMutext = PTHREAD_MUTEX_INITIALIZER;

}

/***
 * Set a new Output pattern. This will only activate the first sequence. The subsequent sequences (if any) will
 * be activated by subsequent calls to GoNextSequence()
 * @param thePattern - The pattern object (a vector of PulseP)
 * @param theRepeat - If true will repeat indefinitely or until a new pattern is introduced
 */
void HwOutput::SendOutputPattern( std::vector<PulseP> *thePattern, bool theRepeat){
	struct timespec now;

	pthread_mutex_lock(&myPatternMutext);

	myPattern = thePattern;
	myRepeat = theRepeat;

    //Set the intial start time
	clock_gettime(CLOCK_MONOTONIC, &now);
	myElapseTime = (now.tv_sec * 1000) + (now.tv_nsec / 1000000);

	//Do the first output activation (if required)
	if( myPattern->at(myCurrentSequence).GetGpioState() != O_NONE)
		digitalWrite(myGpio, myPattern->at(myCurrentSequence).GetGpioState() );

	pthread_mutex_unlock(&myPatternMutext);

}

/***
 * The main loop will call this method at fix interval (current sequence delay value) to go to the next sequence.
 * @param theElapse - Current time (in milliseconds since a fix point in time)
 */
void HwOutput::GoNextSequence( long theElapse)
{

	myElapseTime = theElapse;

	//If the pattern was not delete
	if( myPattern){
		//If there is a next pattern
		if( myPattern->size() > myCurrentSequence + 1){
			pthread_mutex_lock(&myPatternMutext);
			myCurrentSequence++;
			pthread_mutex_unlock(&myPatternMutext);
		}
		else if(myRepeat){ //if not, check if we need the pattern list we just finished
			pthread_mutex_lock(&myPatternMutext);
			myCurrentSequence = 0;
			pthread_mutex_unlock(&myPatternMutext);
		}else {
			ClearOutputPattern(); //Otherwise clean up and exit
			return;
		}

		//Only activate or deactivate the output if required
		if( myPattern->at(myCurrentSequence).GetGpioState() != O_NONE){
			pthread_mutex_lock(&myPatternMutext);
			digitalWrite(myGpio, myPattern->at(myCurrentSequence).GetGpioState() );
			pthread_mutex_unlock(&myPatternMutext);
		}
	}

}

/***
 * Just get the current delay of the active sequence pulse
 * @return
 */
int HwOutput::GetCurrSequenceDelay( void){
	if( myPattern){
		return( myPattern->at(myCurrentSequence).GetDelay());
	}
	return(0);
}

/***
 * Clear the current output pulse pattern and set the Gpio state off
 */
void HwOutput::ClearOutputPattern( void)
{

	pthread_mutex_lock(&myPatternMutext);

	myCurrentSequence = 0;
	myElapseTime = 0;
	if( myPattern )
		myPattern = nullptr;

    digitalWrite(myGpio, LOW);

    pthread_mutex_unlock(&myPatternMutext);

}
