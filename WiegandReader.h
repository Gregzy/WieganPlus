/*
 * wpp.h
 *
 *  Created on: Mar 2, 2014
 *      Author: jfgregoire
 */

#ifndef WIEGANDREADER_H_
#define WIEGANDREADER_H_

#include "pugixml.hpp"
using namespace pugi;


using namespace std;

#include <arpa/inet.h>   //Resolve INET6_ADDRSTRLEN const (see below)
#include "Thread.h"
#include "BitBuffer.h"

#include "wpp.h"

class Card;
class Logger;



//Internal buffer dimension (see int internalBuffer)
#define BUFFER_SIZE_INT 10;


#define WIEGANDTIMEOUT 3000000 //200 milliseconds wait timeout between cards read. Previous value was 3000000

//Friend function to used with WirinPi since this is a C lib and we cannot pass methods to it
void ForwarderData0(void );  	//Incoming signal from Wiegand Reader Data 0 (Green) trigger from WiringPi
void ForwarderData1(void);		//Incoming signal from Wiegand Reader Data 1 (White) trigger from WiringPi

/**
 * WiegandReader class is implement a Wiegand reader object in the real world. The Pi will typically be hooked up to
 * an actual Wiegand reader device (such as a HID 26 bit). This class will read the data from its Data0 and data1 pins
 * and send over IP to another PI for example. The class will also write signal state to the physical devices. For example
 * it would flash lights or buzzer to indicate an acces granted or denied to the user.
 */
class WiegandReader : public Thread {
public:
	WiegandReader(int theThreadPriority = 50);
	WiegandReader(const string & theOutLog, int theThreadPriority = 50 );
	WiegandReader(const string & theOutLog, string theHost, string thePort, int theThreadPriority = 50);

	virtual ~WiegandReader();

	friend void ForwarderData0(void);
	friend void ForwarderData1(void);

	Card * wiegandReadData(void);
	int wiegandGetPendingBitCount();
	int GetLastNumBitsRead(void){return __wiegandBitCount;}
	std::string GetBitBufferString(void);
	void wiegandReset();

protected:
	void OnStart( void);
	void Execute( void *);

private:
	OperationMode myOperationMode;

	Logger *myLog;
	BitBuffer *myBinaryBuffer;
	int myThreadPrty;
	void dataPulse(OUTPUT_DEF theTriggerPin);

	unsigned long __wiegandBitCount;            // number of bits currently captured
	struct timespec __wiegandBitTime;        // timestamp of the last bit received (used for timeouts)

	void wiegandInit(void);

	//IP Related methods and properties
	void *get_in_addr(struct sockaddr *sa);
	bool IpInit(  bool theDoNotDisplayError = false);
	bool isConnected;
	int IpSend( Card *theCard);
	void DisplayRead();

	string myHost;
	string myPort;

    int mySocket;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    int myIpTrxSequence;

    struct xml_string_writer: pugi::xml_writer
    {
        std::string result;
        virtual void write(const void* data, size_t size)
        {
            result += std::string(static_cast<const char*>(data), size);
        }
    };


};


#endif /* WIEGANDREADER_H_ */
