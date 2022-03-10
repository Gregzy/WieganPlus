/*
 * Logger.cpp
 *
 *  Created on: Apr 7, 2014
 *      Author: jfgregoire
 */
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <time.h>
#include "Card.h"
#include "wpp.h"
#include "WatchDogStatus.h"
#include "Logger.h"


const string Logger::myDefaultHeader[9] = {"Date", "Led", "Bzr", "Rly", "Operation", "size", "Raw bits", "Wiegand tag", "Parity"};


Logger::Logger(OperationMode theLogType, std::string theFilename, char theDelimiter) {

	myFileName = theFilename;
	myFile.open(myFileName.c_str(), ios::out | ios::trunc);
	myDlm = theDelimiter;
	myCardLogType = theLogType;

	myWriteLogMutex = PTHREAD_MUTEX_INITIALIZER;

	if( myFile.is_open())
		WriteColumnHeader();
}



void Logger::WriteFileHeader(){

}


void Logger::WriteColumnHeader(){
	unsigned int myArraySize = (sizeof( myDefaultHeader)/ sizeof(string));
	//Put the column header on top
	for(unsigned int i = 0; i < myArraySize; i++  ){
		myFile << myDefaultHeader[i];

		//Place a delimiter except at the end of the last column
		if( i < (myArraySize - 1))
			myFile << myDlm;
	}
	myFile << endl;
}


void Logger::WriteLog(Card *theCard){

	string myCardOpStr[] = {"none", "read", "write"};

	pthread_mutex_lock(&myWriteLogMutex);
	myFile <<  GetCurrentDateTime() << myDlm << myDlm << myDlm << myDlm << "" << myCardOpStr[myCardLogType] << myDlm << theCard->GetNumOfBits() <<
			myDlm << theCard->GetBitBufferString() << myDlm << theCard->GetTagString() << myDlm <<
			(theCard->IsParityOk()?"OK":"ERROR" ) << "\n"; 	//we used traditional end of line instead of endl since it would also flush
															//the buffer each time
	pthread_mutex_unlock(&myWriteLogMutex);

}

void Logger::WriteLog( std::vector<PulseP> &theStateList){


	pthread_mutex_lock(&myWriteLogMutex);

	myFile <<  GetCurrentDateTime() << myDlm ;
	for(std::vector<PulseP>::iterator it = theStateList.begin(); it != theStateList.end(); ++it){
		myFile << it->GetName() << "=" << it->GetGpioState() << " (" << it->GetDelay() << "ms)"<< myDlm;
	}
	myFile << "\n";

	pthread_mutex_unlock(&myWriteLogMutex);

}


Logger::~Logger() {

	myFile.close();
}

const std::string Logger::GetCurrentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
