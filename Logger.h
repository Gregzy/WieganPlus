/*
 * Logger.h
 *
 *  Created on: Apr 7, 2014
 *      Author: jfgregoire
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>
#include <string>
#include "WatchDogStatus.h"
#include "wpp.h"

using namespace std;

class Logger {
public:
	Logger(OperationMode theLogType, std::string  theFilename = "default.csv", char theDelimiter = ',');
	void WriteLog(Card *theCard);
	void WriteLog( std::vector<PulseP> &theStateList);

	virtual ~Logger();
	static const std::string GetCurrentDateTime();


private:
	std::ofstream myFile;
	string myFileName;
	char myDlm;					// Delimiter var
	OperationMode myCardLogType;
	static const std::string myDefaultHeader[9];

	void WriteFileHeader();
	void WriteColumnHeader();
	int Open();
	int Close();

	pthread_mutex_t myWriteLogMutex;


};

#endif /* LOGGER_H_ */
