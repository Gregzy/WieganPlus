/*
 * EventQueue.cpp
 *
 *  Created on: Apr 15, 2014
 *      Author: jfgregoire
 */
#include <ctime>
#include "EventQueue.h"

Event::Event(int theEventType, int theEventCode, string theMessage){

	myDateTime = GetCurrentTime();
	myEventLevel = theEventType;
	myEventCode = theEventCode;
	myMessage = theMessage;
}

Event::Event(int theEventType, int theEventCode, string theMessage, const exception & theException){

	myDateTime = GetCurrentTime();
	myEventLevel = theEventType;
	myEventCode = theEventCode;
	myMessage = theMessage;
	myException = theException;
}


Event::~Event(){

}

struct tm Event::GetCurrentTime(void){
    time_t     now = time(0);
    struct tm  tstruct;
    tstruct = *localtime(&now);
    return(tstruct);
}

CardEvent::CardEvent( int theEventLevel, int theEventCode, Card & theCard):Event(theEventLevel, theEventCode, "No message"){
	myBitCount = theCard.GetNumOfBits();
	myBitBufferStr = theCard.GetBitBufferString();
	myTagId = theCard.GetTagString();
	myParityState = theCard.IsParityOk();
}



EventQueue::EventQueue(OperationMode theLogType, std::string  theFilename, char theDelimiter) {

	if( theLogType != NONE)
		myTransactionLog = new Logger( theLogType, theFilename, theDelimiter);
	else
		myTransactionLog = NULL;

}

EventQueue::~EventQueue() {
	if( myTransactionLog)
		delete myTransactionLog;
}

void EventQueue::InitQueue( void){
	myConditionSignal = PTHREAD_COND_INITIALIZER;
	myQueueMutex = PTHREAD_MUTEX_INITIALIZER;

}


void EventQueue::Add( Event theEvent){
	myQueue.push(theEvent);
}

int EventQueue::GetCount(){
	return( myQueue.size());
}

void EventQueue::OnStart(){

}
void EventQueue::OnStop(){

}
void EventQueue::Execute(void*){


	while( !myTerminateApp){

		pthread_mutex_lock(&myQueueMutex);

		while( myQueue.empty())
			pthread_cond_wait(&myConditionSignal, &myQueueMutex);

		//Event & myEvent = myQueue.front();




	}

}
