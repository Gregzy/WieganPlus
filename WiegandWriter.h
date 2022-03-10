/*
 * WiegandWriter.h
 *
 *  Created on: Mar 19, 2014
 *      Author: jfgregoire
 */

#ifndef WIEGANDWRITER_H_
#define WIEGANDWRITER_H_

#include <arpa/inet.h>   //Resolve INET6_ADDRSTRLEN const (see below)
#include "Thread.h"
#include "WatchDogStatus.h"
#include "wpp.h"

class Logger;

#define BACKLOG 10 // how many pending connections queue will hold

#define GENERATE_MODE 0
#define IP_MODE 1

#define MAX_IP_BUFFER 1024

//Friend function to used with WirinPi since this is a C lib and we cannot pass methods to it
//These functions are called whenever edge is detected on the respective GPIOs they monitor
void ForwarderDataInput(void);  	//Incoming signal from Controller Data 2 (Orange) trigger from WiringPi


/**
 * The WiegandWriter class purpose is to write to the GPIO using the Wiegand pulse protocol. It can be typically
 * use to act as a server, listening from Wiegand request from a remote IP address (from a WiegandReader class
 * attached to Access control reader) and writing the Wiegand swipe to the local GPIO port. This ways, you just
 * created a Wiegand reader erange extension.
 *
 * Another use could be to attached the GPIO output directly to the input of another device and simulate Access traffic.
 */
class WiegandWriter : public Thread {
public:
	WiegandWriter(Card *theStartCard, Card *theEndCard, const string &theLog, int myDelay = 15, int theThreadPriority = 60 );
	WiegandWriter(Card *theStartCard, Card *theEndCard, int myDelay = 15, int theThreadPriority = 60);
	WiegandWriter(string thePort,  int theThreadPriority = 60 );
	WiegandWriter(string thePort,  const string &theLog, int theThreadPriority = 60 );
	virtual ~WiegandWriter();
	void WiegandWriteData( Card *theCard);
	void SendPulseBit(int thePin);

	friend void ForwarderDataInput(void);
	friend void ForwarderData3(void);

	void OnStart( void);
	void Execute( void *);
private:
	OperationMode MyOperationMode;
	Logger *myLog;

	Card *myStartCard;
	Card *myEndCard;
	int myTrxDelay;
	std::vector<PulseP> myCtrlStates;
	pthread_mutex_t myStateMutex;

	void GenerateCards();

	void WiegandInit(void);
	void wiegandReset();

	void DataPulse();

	//IP Related methods and properties
	void *get_in_addr(struct sockaddr *sa);
	int IpInit();
	int IpServer();
	string GetRemoteIp( int theSocket);
	void DisplayWrite(xml_node myNode, int i, Card& myNewCard);
	string FrmtTime( unsigned long long theTime);

	string myPort;
    int mySocketPortListener, mySocketNewWppClient;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int rv;

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int myThreadPriorityValue;

};


#endif /* WIEGANDWRITER_H_ */
