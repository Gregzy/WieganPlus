/*
 * wpp.cpp
 *
 *  Created on: Mar 2, 2014
 *      Author: jfgregoire
 */

#include <exception>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "BitBuffer.h"
#include "Card.h"
#include "WiegandReader.h"
#include "Logger.h"
#include "pugixml.hpp"
#include "WatchDogStatus.h"
#include <signal.h>

using namespace pugi;

extern WatchDogStatus *myWatchDog;
extern WiegandReader *myReader;


/***
 * Default constructor and will typically write the output to the screen of the Wiegand data it receive from the physical device
 * @param theThreadPriority - The default thread priority will be set at 50 (standard would be 20)
 */
WiegandReader::WiegandReader(int theThreadPriority) {

	myThreadPrty = theThreadPriority;

	wiegandInit();
	myLog = nullptr;
	myBinaryBuffer = new BitBuffer(); //Default 10 x Int32

	this->wiegandReset();

}


/***
 * Same as the default constructor, it will typically output text to the screen of the physical device. But it will also log the
 * data to a file
 * @param theOutLog - The log file that will be created and where all readings will be saved
 * @param theThreadPriority - The default thread priority will be set at 50 (standard would be 20)
 */
WiegandReader::WiegandReader(const string & theOutLog, int theThreadPriority) : myThreadPrty(theThreadPriority) {

	wiegandInit();

	myLog = new Logger(READ, theOutLog);
	myBinaryBuffer = new BitBuffer(); //Default 10 x Int32

	this->wiegandReset();

}

/***
 *  Write the output to the screen of the Wiegand data it receive from the physical device and send it to the destination
 *  IP adress. At the other end a WEX software with the WiegandWriter class will be listening
 * @param theOutLog - The log file that will be created and where all readings will be saved
 * @param theHost - Destination IP
 * @param thePort - Destination server port
 * @param theThreadPriority - The default thread priority will be set at 50 (standard would be 20)
 */
WiegandReader::WiegandReader(const string & theOutLog, string theHost, string thePort, int theThreadPriority)
										: myThreadPrty(theThreadPriority), myHost(theHost), myPort(thePort) {
	myOperationMode = READ_TO_IP;
	wiegandInit();

	myLog = new Logger(READ, theOutLog);
	myBinaryBuffer = new BitBuffer(); //Default 10 x Int32

	this->wiegandReset();

}


/***
 * Cleanup the binary buffer and other objects.
 */
WiegandReader::~WiegandReader() {
	if( myLog )
		delete myLog;

	delete(myBinaryBuffer);

	if( myOperationMode == READ_TO_IP)
		close( mySocket);
}

/***
 * Call by the parent init thread when we are about to start the execution
 */
void WiegandReader::OnStart(){
	int myResult = 0;

	myResult = this->SetThreadPriority(myThreadPrty);

	if( myResult == -1){
		//todo Throw an error instead of exiting like this
		cerr << "Error: Failed to assigned higher process priority";
		exit(1);

	}

	if( myOperationMode == READ_TO_IP){
		if( !IpInit())
			exit(1);
	}

}

void WiegandReader::DisplayRead() {
	cout << Logger::GetCurrentDateTime() << " Read " << GetLastNumBitsRead()
			<< " bits:";
	cout << GetBitBufferString();
}

void WiegandReader::Execute(void *){

	while(!myTerminateApp)
	{
		int bitLen = wiegandGetPendingBitCount();
		if (bitLen == 0) {
			delayMicroseconds(3000); //Initial value was 5000
		}
		else
		{
			Card *myCard = NULL;
			myCard = wiegandReadData();
			DisplayRead();
			wiegandReset(); //Clear the reader for the next read

			if( myCard )
			{
				//Display appropriate led pattern
				if( myCard->IsParityOk())
					myWatchDog->SendOutputSignal(BLUE_LED, &WDS_READ_PATTERN);
				else
					myWatchDog->SendOutputSignal(BLUE_LED, &WDS_READ_ERROR);

				//Show it on the console in text for those of us who do not like morse code (led pattern flashing!)
				cout << " Card->" << myCard->GetTagString() << (myCard->IsParityOk() ? " OK" : " Error Parity") << endl;
				if( myLog)
					myLog->WriteLog(myCard);

				if(myOperationMode == READ_TO_IP){
					//If we were not connected the last time around, try to reconnect before attempting this new transmission
					if( !isConnected){
						cout << "Trying to reconnect to " << myHost << ":" << myPort <<  " ..." <<endl;;
						//Reconnect
						IpInit(true);
						if( IpSend(myCard) <= 0){
							//Still not working? We'll try to fix it the next time around
							cerr << "Failed to send. Connection lost!" << endl;
							myWatchDog->StopOutputSignal(GREEN_LED);
							close( mySocket);
							isConnected = false;
						}
					}else{
						//Otherwise, everything is fine and just send
						if( IpSend(myCard) <= 0){
							//Houston we have a problem
							cerr << "Failed to send. Connection lost!" << endl;
							close( mySocket);
							isConnected = false;
							myWatchDog->StopOutputSignal(GREEN_LED);
						}
					}
				}
				delete myCard;
			}else
				cout << " Error reading Card" << endl;
		}
	}
}


/***
 * Called by the WiringPi friend function every time there is a voltage drop on pin 0 or 1 (see Wiegand electric protocol)
 * @param theTriggerPin - The pin that dropped voltage
 */
void WiegandReader::dataPulse(OUTPUT_DEF theTriggerPin) {
	if( theTriggerPin == WIEG_GREEN){
		__wiegandBitCount++;
	    clock_gettime(CLOCK_MONOTONIC, &__wiegandBitTime);
	}else if(theTriggerPin == WIEG_WHITE ){
		//Otherwise we received WIEG_WHITE
		myBinaryBuffer->SetBit(__wiegandBitCount);
		__wiegandBitCount++;
	    clock_gettime(CLOCK_MONOTONIC, &__wiegandBitTime);
	}else{
		//Log an error
		//todo Include in error level
		cout << "Error receiving pin trigger Wiegand data ";
	}
}


void WiegandReader::wiegandInit(void) {
 // Setup wiringPi
    pinMode(WIEG_GREEN, INPUT);
    pinMode(WIEG_WHITE, INPUT);

    //Enable event trigger on the GPIOs. Each time the signal is falling, it means we got a bit (see Wiegand electric protocol).
    wiringPiISR(WIEG_GREEN, INT_EDGE_FALLING, ForwarderData0);
    wiringPiISR(WIEG_WHITE, INT_EDGE_FALLING, ForwarderData1);
}

void WiegandReader::wiegandReset() {

	myBinaryBuffer->ClearBuffer();
	__wiegandBitCount = 0;

}

int WiegandReader::wiegandGetPendingBitCount() {
    struct timespec now, delta;
    clock_gettime(CLOCK_MONOTONIC, &now);
    delta.tv_sec = now.tv_sec - __wiegandBitTime.tv_sec;
    delta.tv_nsec = now.tv_nsec - __wiegandBitTime.tv_nsec;

    if ((delta.tv_sec > 1) || (delta.tv_nsec > WIEGANDTIMEOUT))
        return __wiegandBitCount;

    return 0;
}

/*
 * wiegandReadData is a simple, non-blocking method to retrieve the last code
 * processed by the API.
 */
Card * WiegandReader::wiegandReadData(void) {
    if (wiegandGetPendingBitCount() > 0) {

    	Card *myCard = new Card(myBinaryBuffer->GetInternalBuffer(), WIEGAND26);

        return myCard;
    }
    return( NULL);
}

/***
 * Translate the binary buffer in a human readable 1 and 0.
 * @return The string with the binary representation
 */
std::string WiegandReader::GetBitBufferString(){

	string myReturnVal;

    for( unsigned int i = 0; i < __wiegandBitCount;i++)
    {
    	myReturnVal += (myBinaryBuffer->TestBit(i) ? "1":"0");
    }

    return( myReturnVal);
}


/**
 * This method will evaluate the type of address we are dealing with IPv4 or IPv6
 * @param sa - the socket address structure
 */
void *WiegandReader::get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


bool WiegandReader::IpInit( bool theDoNotDisplayError){

	myIpTrxSequence = 0;
    struct addrinfo hints, *servinfo, *addrPtr;
    int rv;
    char myDestinationIP[INET6_ADDRSTRLEN];
    isConnected = false;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(myHost.c_str(), myPort.c_str(), &hints, &servinfo)) != 0) {
    	if( !theDoNotDisplayError)
    		cout << "getaddrinfo: " << gai_strerror(rv) << endl;
        return(false);
    }

    // loop through all the results and connect to the first we can
    for(addrPtr = servinfo; addrPtr != NULL; addrPtr = addrPtr->ai_next) {
        if ((mySocket = socket(addrPtr->ai_family, addrPtr->ai_socktype, addrPtr->ai_protocol)) == -1) {
        	if( !theDoNotDisplayError)
        		cerr << "Socket error " << errno << endl;
            continue;
        }

        if (connect(mySocket, addrPtr->ai_addr, addrPtr->ai_addrlen) == -1) {
            close(mySocket);
        	if( !theDoNotDisplayError)
        		cerr << "Error connecting " << errno << endl;
            continue;
        }

        break;
    }

    if (addrPtr == NULL) {
    	if( !theDoNotDisplayError)
    		cerr << "client: failed to connect\n";
        return(false);
    }

    myWatchDog->SendOutputSignal(GREEN_LED, &WDS_WRITE_NET_CONNECT, true);

    cout << "Connected to " << inet_ntop(addrPtr->ai_family, get_in_addr((struct sockaddr *)addrPtr->ai_addr),
    						myDestinationIP, INET6_ADDRSTRLEN) << endl;
    isConnected = true;

    //Just in case we lose connection in the future, disable the broken pipe SIGNAL. We will handle connection lost through the return value
    // of the send() function.
    signal(SIGPIPE, SIG_IGN);

    freeaddrinfo(servinfo); // all done with this structure

	return(true);
}




int WiegandReader::IpSend( Card *theCard){
	int myReturnVal = 0;

	xml_document myDoc;

	//Create the transaction
	xml_node myNodeTrx = myDoc.append_child(XML_TRX_TAG);
	myNodeTrx.append_attribute(XML_TRX_ATTR_SEQ).set_value(++myIpTrxSequence);
	//And its child node
	xml_node mySubNode = myNodeTrx.append_child(XML_CARD_TAG_EVENT);
	mySubNode.append_attribute(XML_CARD_ATTR_ID).set_value(theCard->GetTagString().c_str());
	mySubNode.append_attribute(XML_CARD_ATTR_FORMAT).set_value(WIEGAND26);
	mySubNode.append_attribute(XML_CARD_ATTR_BITS).set_value(theCard->GetNumOfBits());

	xml_node binNode = mySubNode.append_child(XML_BINARY_TAG);
	binNode.text().set( theCard->GetBitBufferString().c_str() );

	xml_string_writer myWriter;
	myDoc.print(myWriter);

	string myStringToSend = myWriter.result;

	myReturnVal = send(mySocket, myStringToSend.c_str(), myStringToSend.size(), 0 );
	return(myReturnVal);
}




void ForwarderData0()
{
	myReader->dataPulse(WIEG_GREEN);
}

void ForwarderData1()
{
	myReader->dataPulse(WIEG_WHITE);
}





