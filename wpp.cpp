#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#include <exception>

#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include "WiegandReader.h"
#include "WiegandWriter.h"
#include "Card.h"
#include "WatchDogStatus.h"

#include "wpp.h"

using namespace std;


WiegandReader *myReader = NULL; //This global var is used within class WiegandReader
WiegandWriter *myWriter = NULL;
WatchDogStatus *myWatchDog = NULL;
Card *myStartCard = NULL;
Card *myEndCard = NULL;

OperationMode myOpMode;

void DisplayHelp(void);
void CleanUP(int s);


int main (int argc, char* argv[])
{

	myOpMode = NONE;

	//Signal handler setup. We use it to be notified if the user decide to do an harakiri on this software by pressing CTRL-C
	//The handler will allow us to neatly clean up the software before exiting (bad, bad user ;)
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = CleanUP;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);


   //Init and default argument values
	unsigned int myDelayBetweenTrx = 10;
	string myOutLog;
	string myPort;	//Listening or writing ip port
	string myDestIp; //Destination IP

    cout << "wex - Wiegand Extension v0.83.63" << endl;
    cout << "by JF \"Gregzy\" Grégoire (2014-12-13)" << endl;
    cout << "======================================" << endl;

    if (argc < 2) { // Check the value of argc. If not enough parameters have been passed, inform user and exit.
    	DisplayHelp();
        exit(0);
    } else { // if we got enough parameters...
        for (int i = 1; i < argc; i++) { /* We will iterate over argv[] to get the parameters stored inside.
                                          * Note that we're starting on 1 because we don't need to know the
                                          * path of the program, which is stored in argv[0] */
			if (string(argv[i]) == "-r") {
				if( myOpMode == NONE)
					myOpMode = READ;
				else
				{
					cout << "You can only select one operation mode"<< endl;
					DisplayHelp();
					exit(1);
				}
			} else if (string(argv[i]) == "-w") {
				if( myOpMode == NONE)
					myOpMode = WRITE;
				else
				{
					cout << "You can only select one operation mode"<< endl;
					DisplayHelp();
					exit(1);
				}
			} else if (string(argv[i]) == "-o") {
				myOutLog = argv[i + 1];
				i++;
			} else if (string(argv[i]) == "-i") {
				//todo remove myFile = argv[i + 1];
			} else if (string(argv[i]) == "-g") {
				unsigned int myPosSplit = 0;
				string myRangeStart;
				string myRangeEnd;
		    	string myParsedArg;

				myParsedArg.assign(argv[i + 1]);
				myPosSplit = myParsedArg.find_first_of("-", 0);
				if( myPosSplit != std::string::npos ){   //npos define 'Not Found'
					try {
						myRangeStart = myParsedArg.substr(0, myPosSplit);
						myStartCard = new Card(myRangeStart, WIEGAND26);
						myRangeEnd = myParsedArg.substr(myPosSplit + 1, 8);
						myEndCard = new Card(myRangeEnd, WIEGAND26);

					} catch (exception &e) {
						cout << e.what() << endl;
						exit(1);
					}

					i++; //Skip the argument data of -g the next time around

				}else{
					std::cout << "Error: " << myParsedArg << " does not specify a valid range\n";
					DisplayHelp();
					exit(1);
				}

			} else if (string(argv[i]) == "-p") {

				try {
					myDelayBetweenTrx = atoi(argv[i + 1]);
					i++;
				} catch (exception& e) {
					cout << e.what() << endl;
					exit(1);
				}
			} else if (string(argv[i]) == "-n") {
				try {
					//If we were in WRITE mode, it means we are expecting a port value next
					//and the mode is now IP based
					if( myOpMode == WRITE){
						myOpMode = WRITE_FROM_IP;

						int myPortValTest = atoi(argv[i + 1]);
						if( myPortValTest){
							myPort = argv[i + 1];
							i++;
						}else{
							std::cout << "Error: " << argv[i + 1] << " is not a valid IP port\n";
							DisplayHelp();
							exit(1);
						}

					}else if( myOpMode == READ){
						myOpMode = READ_TO_IP;

						unsigned int myPosSplit = 0;
				    	string myParsedArg;

						myParsedArg.assign(argv[i + 1]);
						myPosSplit = myParsedArg.find_first_of(":", 0);
						if( myPosSplit != std::string::npos ){   //npos define 'Not Found'
							try {
								myDestIp = myParsedArg.substr(0, myPosSplit);
								myPort = myParsedArg.substr(myPosSplit + 1, 8);

							} catch (exception &e) {
								cout << e.what() << endl;
								exit(1);
							}

							i++; //Skip the argument data of -g the next time around

						}else{
							if( myParsedArg.length() > 0){
								myDestIp = myParsedArg;
								myPort = DEFAULT_IP_PORT;
							}
							else{
								std::cout << "Error: " << myParsedArg << " does not specify a valid destination\n";
								DisplayHelp();
								exit(1);
							}
						}



					}

				} catch (exception& e) {
					cout << e.what() << endl;
					exit(1);
				}
			} else {
				std::cout << "Error: Not enough or invalid arguments, please try again.\n";
				DisplayHelp();
				exit(1);

			}
        }
    }

    if( myOpMode == NONE){
	    cout << "You must select an operation mode (-r Read or -w Write)"<< endl;
	    exit(1);

    }

    // Setup wiringPi
    wiringPiSetup() ;

    myWatchDog =  new WatchDogStatus( );

    if( !myWatchDog->AddHwOutput(BLUE_LED) || !myWatchDog->AddHwOutput(GREEN_LED))
    	cout << "Error: Gpio already in use \n";

    int myStart = 3;
    myWatchDog->Start(&myStart);
    int myStopError = 0;


    if( myOutLog.empty())
    	myOutLog.assign(DEFAULT_lOG);

    switch (myOpMode) {
		case READ:
			if( !myOutLog.empty())
				myReader = new WiegandReader(myOutLog);
			else
				myReader = new WiegandReader(); //Otherwise use the default


			myReader->Start(NULL);

			myReader->WaitForThreadStop();

			delete myReader;

			break;
		case WRITE:
			myWriter = new WiegandWriter (myStartCard, myEndCard, myOutLog, myDelayBetweenTrx, 50);

			myWriter->Start(NULL);

			myStopError = myWriter->WaitForThreadStop();
			if( myStopError != 0)
				cout << "Error: Unable to stop the Writer thread with error code #" << myStopError << endl;
			break;

		case WRITE_FROM_IP:
			if( !myOutLog.empty())
				myWriter = new WiegandWriter(myPort, myOutLog);
			else
				myWriter = new WiegandWriter(myPort);

			myWriter->Start(NULL);

			myStopError = myWriter->WaitForThreadStop();
			if( myStopError != 0)
				cout << "Error: Unable to stop the Writer thread with error code #" << myStopError << endl;
			break;
		case READ_TO_IP:
			myReader = new WiegandReader(myOutLog, myDestIp, myPort);
			myReader->Start(NULL);

			myReader->WaitForThreadStop();

			delete myReader;


			break;
		default:
			break;
	}


	//CleanUP takes care of deleting the allocation and is alternatively called by a signal handler SIGINT if user press CTL-C
	//But if we got here its not the case, the process finished without interruptions.
	CleanUP(0);

    return 0;
 }

void DisplayHelp(void){
    std::cout << "wpp [-r] [-w] [-g] <from-to> [-p] <pause in sec.> [-in] <infile> [-o] <outlog>\n"; // Inform the user of how to use the program
    std::cout << "		-r Read Wiegand data from GPIO \n";
	std::cout << "			-n Send the wiegand data read from the GPIO to this IP address (ex.: -n 192.168.1.10:6777) \n";
    std::cout << "		-w Write Wiegand data to GPIO pins\n";
    std::cout << "			-g, Generate cards with range. You must specify a range (ex.: aa:10000-ff:65535)  \n";
	std::cout << "			-p, Specify the number of seconds to pause between each write transaction  \n";
	std::cout << "			-i, Input delimited file used to write the data \n";
	std::cout << "			-n Write wiegand data to the GPIO by listening to the following IP port (ex.: -n 6777)   \n";
	std::cout << "		-o Output log file (optional) \n";
}


void CleanUP(int s){

	if( s == SIGINT)
		cout << endl << "Answering user abort request. Please wait..." << endl;

	cout << endl << "Stopping Whatchdog...";
	myWatchDog->Stop();
	cout << "Done!" << endl;
	delete myWatchDog;

	if( myOpMode == WRITE || myOpMode == WRITE_FROM_IP){
		if( myWriter ){
			//We could actually just call delete, it would stop and join the thread, but the following is more transparent (same goes to myReader)
			if( myWriter->IsRunning()){
				cout << "Stopping the Wiegand Writer ...";
				myWriter->Stop();
				cout << "Done!" << endl;
			}

			delete myWriter;
			delete myStartCard;
			delete myEndCard;
		}
	}else if( myOpMode == READ || myOpMode == READ_TO_IP){
		if( myReader){
			if( myReader->IsRunning()){
				cout << "Stopping the Wiegand Reader...";
				myReader->Stop();
				cout << "Done!" << endl;
			}
			delete myReader;
		}
	}

	cout << "Bye!" << endl;
           exit(0);

}


long GetMilliseconds( struct timespec theTime){
	return(theTime.tv_sec * 1000) + (theTime.tv_nsec / 1000000);
}


