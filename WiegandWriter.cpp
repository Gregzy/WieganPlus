	/*
	 * WiegandWriter.cpp
	 *
	 *  Created on: Mar 19, 2014
	 *      Author: jfgregoire
	 */
	#include <time.h>
	#include <iostream>
	#include <unistd.h>
	#include <wiringPi.h>
	#include <stdio.h>
	#include <errno.h>
	#include <stdlib.h>
	#include <time.h>
	#include <unistd.h>
	#include <memory.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <sys/wait.h>
	#include "wpp.h"
	#include "Card.h"
	#include "Thread.h"
	#include "WiegandWriter.h"
	#include "WiegandReader.h"
	#include "Logger.h"
	#include "pugixml.hpp"
	#include "WatchDogStatus.h"
	#include <algorithm>
	#include <string>
	#include <sstream>
	#include <iomanip>

	using namespace pugi;
	using namespace std;

	extern WatchDogStatus *myWatchDog;
	extern WiegandWriter *myWriter;

	WiegandWriter::WiegandWriter(Card *theStartCard, Card *theEndCard, const string &theLog, int theDelay, int theThreadPriority) {

		//todo Argument validation with exception class

		MyOperationMode = WRITE;
		myStartCard = theStartCard;
		myEndCard = theEndCard;
		myTrxDelay = theDelay;
		myThreadPriorityValue = theThreadPriority;

		WiegandInit();

		myLog = new Logger(WRITE, theLog);
	}


	WiegandWriter::WiegandWriter(Card *theStartCard, Card *theEndCard, int theDelay, int theThreadPriority) {

		//todo Argument validation with exception class

		MyOperationMode = WRITE;
		myStartCard = theStartCard;
		myEndCard = theEndCard;
		myTrxDelay = theDelay;
		myThreadPriorityValue = theThreadPriority;

		WiegandInit();

		myLog = nullptr;

	}

	WiegandWriter::WiegandWriter( string thePort,  int theThreadPriority): myPort(thePort), myThreadPriorityValue(theThreadPriority){
		MyOperationMode = WRITE_FROM_IP;
		WiegandInit();
	}

	WiegandWriter::WiegandWriter( string thePort,  const string &theLog, int theThreadPriority): myPort(thePort), myThreadPriorityValue(theThreadPriority){
		MyOperationMode = WRITE_FROM_IP;
		WiegandInit();

		myLog = new Logger(WRITE, theLog);
	}



	WiegandWriter::~WiegandWriter() {
		if( myLog)
			delete myLog;
	}

	void WiegandWriter::WiegandInit(void) {
		 // Setup wiringPi
			pinMode(WIEG_GREEN, OUTPUT); 	//Wiegand Data 0 (from reader)
			pinMode(WIEG_WHITE, OUTPUT); 	//Wiegand Data 1 (from reader)
			pinMode(WIEG_LED_IN, INPUT); 	//Wiegand LED feedback (from controller)
			pinMode(WIEG_BUZ_IN, INPUT); 	//Wiegand Buzzer feedback (from controller)
			pinMode(WIEG_RELAY, INPUT);		//Door relay
			pullUpDnControl( WIEG_RELAY, PUD_UP);

			//By default, Wiegand maintain voltage on pins. Only edge falling indicate data presence
			digitalWrite(WIEG_GREEN, HIGH);
			digitalWrite(WIEG_WHITE, HIGH);

			//Enable event trigger on the GPIOs. Each time the signal is falling, it means we got a bit (see Wiegand electric protocol).
			wiringPiISR(WIEG_LED_IN, INT_EDGE_BOTH, ForwarderDataInput);
			wiringPiISR(WIEG_BUZ_IN, INT_EDGE_BOTH, ForwarderDataInput);
			wiringPiISR(WIEG_RELAY, INT_EDGE_BOTH, ForwarderDataInput);

			myCtrlStates = {PulseP(O_OFF, 0, "LED", WIEG_LED_IN), PulseP(O_OFF, 0, "BZR", WIEG_BUZ_IN), PulseP(O_OFF, 0, "RLY", WIEG_RELAY)};

			myStateMutex = PTHREAD_MUTEX_INITIALIZER;

	}


	void WiegandWriter::OnStart( void){

		int myResult = 0;

		myResult = this->SetThreadPriority(myThreadPriorityValue);

		if( myResult == -1){
			//todo Throw an error instead of exiting like this
			cerr << "Error: Failed to assigned higher process priority";
			exit(1);

		}

	}

	void WiegandWriter::GenerateCards() {
		Card* myCard2Write = new Card(myStartCard->GetSiteCode(),
				myStartCard->GettagID(), WIEGAND26);
		while ((*myCard2Write <= *myEndCard) && !myTerminateApp) {
			WiegandWriteData(myCard2Write);
			cout << " Write bits raw:" << myCard2Write->GetBitBufferString();
			cout << " Card->" << myCard2Write->GetTagString()
					<< (myCard2Write->IsParityOk() ? " OK" : " Error Parity")
					<< endl;
			if (myLog)
				myLog->WriteLog(myCard2Write);

			// todo remove sleep(myTrxDelay);
			delay(myTrxDelay * 1000);

			Card *myOldCard = myCard2Write;

			if (myOldCard->GettagID() < MAX_WIEGAND_TAG_VALUE)
				myCard2Write = new Card(myOldCard->GetSiteCode(),
						myOldCard->GettagID() + 1, WIEGAND26);
			else if (myOldCard->GetSiteCode() < MAX_WIEGAND_SITE_VALUE) {
				myCard2Write = new Card(myOldCard->GetSiteCode() + 1, 0, WIEGAND26);
			} else {
				cout << "Error: Range error" << myOldCard->GetTagString();
				exit(1);
			}

			delete myOldCard;

		}
		delete myCard2Write;
	}

	void WiegandWriter::Execute(void * theArgData){

		if( MyOperationMode == WRITE_FROM_IP){
			IpInit();
			IpServer();
		}else{
			GenerateCards();
		}
	}

	void WiegandWriter::WiegandWriteData( Card *theCard){

		delay(2); //Wait 2 Milliseconds. No rush!

		for(int i=0; i < theCard->GetNumOfBits(); ++i){
			if( theCard->GetBit(i)){
				SendPulseBit(WIEG_WHITE);
			}else{
				SendPulseBit(WIEG_GREEN);
			}

			delay(2);
		}

		myWatchDog->SendOutputSignal(BLUE_LED, &WDS_WRITE_PATTERN);

		delay(2);

	}

	/**
	 * SendPulse basically change the normal high voltage state to a low voltage for about
	 * 40 microseconds. The argument indicate on which pin we must pulse. On Wiegand standard,
	 * if we want to send a '1' we must pulse GPIO pin 1 (DATA1) and if we want to send a '0'
	 * we must pulse GPIO pin 0 (DATA0).
	 * @param thePin - Indicate on which GPIO we must pulse
	 */
	void WiegandWriter::SendPulseBit(int thePin){

		digitalWrite(thePin, LOW);
		delayMicroseconds(40);
		digitalWrite(thePin, HIGH);

	}

	/**
	 * This method will evaluate the type of address we are dealing with IPv4 or IPv6
	 * @param sa - the socket address structure
	 */
	void *WiegandWriter::get_in_addr(struct sockaddr *sa){
		if (sa->sa_family == AF_INET) {
			return &(((struct sockaddr_in*)sa)->sin_addr);
		}

		return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}


	int WiegandWriter::IpInit(){
		struct addrinfo hints, *servinfo, *addrPtr;
		int yes=1;

		FD_ZERO(&master);    // clear the master and temp sets
		FD_ZERO(&read_fds);

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // use my IP

		if ((rv = getaddrinfo(NULL, myPort.c_str(), &hints, &servinfo)) != 0) {
			cerr <<  "Get address info failed with error code:" << gai_strerror(rv) << endl;
			return 1;
		}

		// loop through all the results and bind to the first we can
		for(addrPtr = servinfo; addrPtr != NULL; addrPtr = addrPtr->ai_next) {
			if ((mySocketPortListener = socket(addrPtr->ai_family, addrPtr->ai_socktype,addrPtr->ai_protocol)) == -1) {
				cerr << "Error socket" << strerror(errno) << endl;
				continue;
			}

			if (setsockopt(mySocketPortListener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
				cerr << "Error setsockopt" << strerror(errno) << endl;
				exit(1);
			}

			if (bind(mySocketPortListener, addrPtr->ai_addr, addrPtr->ai_addrlen) == -1) {
				close(mySocketPortListener);
				cerr << "Error bind" << strerror(errno) << endl;
				continue;
			}

			break;
		}

		if (addrPtr == NULL)  {
			cerr << "server: failed to bind\n";
			exit(2);
		}

		freeaddrinfo(servinfo); // all done with this structure

		if (listen(mySocketPortListener, BACKLOG) == -1) {
			cerr << "listen" << strerror(errno) << endl;
			exit( 3);
		}

		// add the listener to the master set
		FD_SET(mySocketPortListener, &master);

		// keep track of the biggest file descriptor
		fdmax = mySocketPortListener; // so far, it's this one

		cout << "Binding completed, waiting for connections...\n";
		return(0);
	}

	void WiegandWriter::DisplayWrite(xml_node myNode, int i, Card& myNewCard) {
		cout << Logger::GetCurrentDateTime() << " Write " << myNewCard.GetNumOfBits() << " bits raw:"
				<< myNewCard.GetBitBufferString() << " Card->"
				<< myNode.attribute(XML_CARD_ATTR_ID).value() << " IP: "
				<< GetRemoteIp(i) << endl;
	}

	int WiegandWriter::IpServer(){
		struct sockaddr_storage remoteaddr; // client address
		socklen_t addrlen;
		int newfd;        // newly accept()ed socket descriptor
		char remoteIP[INET6_ADDRSTRLEN];
		int nbytes;
		char buf[MAX_IP_BUFFER];    // buffer for client data

		while ( !myTerminateApp) {
			read_fds = master; // copy it
			if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
				perror("select");
				exit(4);
			}

			// run through the existing connections looking for data to read
			for(int i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &read_fds)) { // we got one!!
					if (i == mySocketPortListener) {
						// handle new connections
						addrlen = sizeof remoteaddr;
						newfd = accept(mySocketPortListener, (struct sockaddr *)&remoteaddr, &addrlen);

						if (newfd == -1) {
							cerr << "Error accept " << strerror(errno) << endl;
						} else {
							FD_SET(newfd, &master); // add to master set
							if (newfd > fdmax) {    // keep track of the max
								fdmax = newfd;
							}
							cout << "Accepting new connection from " << inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
									remoteIP, INET6_ADDRSTRLEN) << " ID:"<< newfd << endl;

							myWatchDog->SendOutputSignal(GREEN_LED, &WDS_WRITE_NET_CONNECT, true);
						}
					}else {
						// handle data from a client
						if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
							// got error or connection closed by client
							if (nbytes == 0) {
								// connection closed
								cout << "Connection socket ID:" << i << " hung up. Bye, we will miss you." << endl;
								myWatchDog->StopOutputSignal(GREEN_LED);
							} else {
								cerr << "Error recv " << strerror(errno) << endl;
							}
							close(i); // bye!
							FD_CLR(i, &master); // remove from master set
						} else {


							xml_document myDoc;
							xml_parse_result result = myDoc.load_buffer_inplace(buf, MAX_IP_BUFFER);

							if( result.status == status_ok){
								xml_node myNode =  myDoc.child(XML_TRX_TAG).child(XML_CARD_TAG_EVENT);
								if( myNode){
									string myCardTag = myNode.attribute(XML_CARD_ATTR_ID).value();
									Card myNewCard( myCardTag, WIEGAND26);

									DisplayWrite(myNode, i, myNewCard);
									WiegandWriteData(&myNewCard);
									if( myLog)
										myLog->WriteLog(&myNewCard);

								}
								else
									cerr << "Error: No xml node to process" << endl;
							}else{
								cerr << "Error parsing: " << result.description() << endl;
							}



							// we got some data from a client
							/*for(int j = 0; j <= fdmax; j++) {
								// send to everyone!
								if (FD_ISSET(j, &master)) {
									// except the listener and ourselves
									if (j != mySocketPortListener && j != i) {
										if (send(j, buf, nbytes, 0) == -1) {
											cerr << "Error send " << strerror(errno) << endl;
										}
									}
								}
							}*/
						}
					} // END handle data from client
				} // END got new incoming connection
			} // END looping through file descriptors
		}

		close(mySocketPortListener);

		return(0);
	}

	string WiegandWriter::GetRemoteIp( int theSocket){
		socklen_t myLen;
		struct sockaddr_storage myAddr;
		char myIpStr[INET6_ADDRSTRLEN];

		myLen = sizeof myAddr;
		getpeername(theSocket, (struct sockaddr*)&myAddr, &myLen);

		// deal with both IPv4 and IPv6:
		if (myAddr.ss_family == AF_INET) {
			struct sockaddr_in *myStructAddress = (struct sockaddr_in *)&myAddr;
			inet_ntop(AF_INET, &myStructAddress->sin_addr, myIpStr, sizeof myIpStr);
		} else { // AF_INET6
			struct sockaddr_in6 *myStructAddress = (struct sockaddr_in6 *)&myAddr;
			inet_ntop(AF_INET6, &myStructAddress->sin6_addr, myIpStr, sizeof myIpStr);
		}

		return( string(myIpStr));
	}

	/***
	 * The DataPulse method is called by the ForwardDataInput friend function. This last function is registered
	 * with WiringPi to be triggered every time an input (High or Low) is detected on multiple GPIOs (see WiegandInit
	 * method for the list of defined GPIOs).
	 *
	 * Note: Initially a function was defined for each GPIOs. Unfortunately if 2 inputs were detected at the same time
	 * we could lose one of the two. So this method will check all the relevant GPIOs for a change state.
	 */
	void WiegandWriter::DataPulse(){

		pthread_mutex_lock(&myStateMutex);

		bool myNewState = false;
		//string myOutput = "";
		ostringstream myOutput;

		struct timespec myTimeNow;
		clock_gettime(CLOCK_MONOTONIC, &myTimeNow);

		//myOutput += Logger::GetCurrentDateTime() + string(" State:");
		myOutput << Logger::GetCurrentDateTime() << " State:";

		std::for_each(myCtrlStates.begin(), myCtrlStates.end(), [&](PulseP &i){
			unsigned int myLiveState = digitalRead(i.GetId());
			if( i.GetGpioState() != myLiveState ){
				i.SetDelay( GetMilliseconds(myTimeNow) - GetMilliseconds(i.GetLastActiveTime()));
				i.SetGpioState( myLiveState);
				myNewState = true;
				myOutput << "\033[1;33m";
			}
			myOutput << "(" << i.GetName() << i.GetIdGpio() << "=" << i.GetGpioState() << " " << setw(3)<< FrmtTime(i.GetDelay()) << ") ";
			myOutput << "\033[0m";

		});


		//Write on screen only if we got at least one state modification. Unfortunately, since WiringPi is single threaded
		//for message dispatch we can lose messages if they occurred at the same time. We solve this by having this method
		//check all states disregarding where it came from. But some time it does work so we get redundant calls (and we simply
		//do not show those).
		if( myNewState)
			cout << myOutput.str() << "\n";

		if( myLog && myNewState)
			myLog->WriteLog(myCtrlStates);

		pthread_mutex_unlock(&myStateMutex);

	}

	string WiegandWriter::FrmtTime( unsigned long long theTime){
		ostringstream myReturnSuffx;
		const int SECONDS = 1000, MINUTES = 60000, DAYS = 360000;

		if( theTime > SECONDS ){
			if( theTime > MINUTES){
				if( theTime > DAYS){
					myReturnSuffx << setw(3) << to_string( theTime / DAYS) << "day";
				}
				else{
					myReturnSuffx  << setw(3) <<  to_string( theTime / MINUTES) << "min";
				}
			}else{
				myReturnSuffx  << setw(3) <<  to_string( theTime / SECONDS) << "sec";
			}
		}else{
			myReturnSuffx  << setw(3) <<  to_string( theTime) << "mls";
		}

		return( myReturnSuffx.str());
	}



	void ForwarderDataInput()
	{
		myWriter->DataPulse();
	}


