/*
 * EventQueue.h
 *
 *  Created on: Apr 15, 2014
 *      Author: jfgregoire
 */

#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include "Thread.h"
#include <string>
#include <queue>
#include "Card.h"
#include "Logger.h"


using namespace std;

//Event code definition
#define APP_START		100
#define APP_STOP		101
#define WIEGAND_READ 	1000
#define WIEGAND_WRITE 	2000
#define ERROR_UNKNOWN	9000

//Event level
#define LEVEL_INFORMATION		0
#define LEVEL_WARNING			1
#define LEVEL_ERROR				3


class Event {
public:
	Event(int theEventLevel, int theEventCode, string theMessage);
	Event(int theEventLevel, int theEventCode, string theMessage, const exception & theException);
	virtual ~Event();

protected:
	struct tm myDateTime;
	unsigned int myEventLevel;
	unsigned int myEventCode;
	string myMessage;
	exception myException;

	struct tm GetCurrentTime(void);
};

class CardEvent : public Event {
public:
	CardEvent(int theEventLevel, int theEventCode, Card & theCard);

protected:
	int myBitCount;
	string myBitBufferStr;
	string myTagId;
	bool myParityState;
};

class EventQueue: public Thread {
public:
	EventQueue(OperationMode theLogType, std::string  theFilename = "default.csv", char theDelimiter = ',');
	virtual ~EventQueue();

	void Add( Event theEvent);
	int GetCount( void);
	void ProcessedEvent();
private:
	std::queue<Event> myQueue;
	Logger *myTransactionLog;
	pthread_cond_t myConditionSignal;
	pthread_mutex_t myQueueMutex;

	void InitQueue(void);
protected:
	virtual void OnStart();
	virtual void OnStop();
	virtual void Execute(void*);


};




#endif /* EVENTQUEUE_H_ */
