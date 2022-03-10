/*
 * Thread.cpp
 *
 *  Created on: Mar 31, 2014
 *      Author: jfgregoire
 */
#include <stddef.h>
#include <wiringPi.h>
#include "Thread.h"

/**
 * Thread is a simple wrapper around the Posix thread. You must derived from this class
 * in order to use it.
  */
Thread::Thread() {
	ThreadId_ = -1;  //Just so the compiler shut up about this var not being initialized
	myTerminateApp = true;
	Arg_ = NULL;
	myThreadPriority = 0;


}

/**
 * Creating the object does not initiate the thread. You must call this Start method to do so. Please
 * do not forget to derived Thread with the public keyword to access this member. You should not
 * have to override this method.
 * @param arg - Argument is going to be store within the class for a subsequent call within EntryPoint
 * @return Return the thread creation status
 */
int Thread::Start(void * arg)
{
   Arg(arg); // store user data

   //Note that usually, pthread_create does not accept a c++ method. However, in this case
   //EntryPoint() is static, so we can get away with it. We pass 'this' object so we can
   //feed it our argument
   int code = pthread_create(&ThreadId_, NULL, Thread::EntryPoint, this);

   return code;
}

int Thread::Stop(){
	int myReturnedVal = 0; //0 is the default success value

	if( !myTerminateApp){
		myTerminateApp = true;
		OnStop(); //Let the derived object do any thread related cleanup
		myReturnedVal = pthread_join(ThreadId_, NULL);
	}

	return( myReturnedVal);

}


int Thread::SetThreadPriority(int thePriority){
	int myResult = 0;
	myThreadPriority = thePriority;

	//It is important we set in thread priority in this method since at this execution point we are in the new thread
	//The constructor of this object is still sitting in the calling thread
	myResult = piHiPri(myThreadPriority);

	return(myResult);
}


/**
 * This protected method is call by the static EntryPoint method. It will call the virtual Setup() and Execute() method
 * in the derived code. You should not tampered with this guy.
 * @return None of your business
 */
int Thread::Run()
{
	myTerminateApp = false;

	OnStart();
	Execute( Arg() );

	//If Execute terminated by itself, we can change the status to stopped (myTerminateApp = true)
	if( !myTerminateApp)
	   myTerminateApp = true;

   return(0);
}

/**
 * Static method passed to the pthread_create function. Since it is static, pthread_create will accept it
 * but it would not accept a c++ object method. The Start method pass a this pointer so we can actually
 * initiate the code from the object. EntryPoint is only used to pass the buck
 * @param pthis
 */
void * Thread::EntryPoint(void * pthis)
{
   Thread * pt = (Thread*)pthis;
   pt->Run( );

   return(0);
}

/**
 * Virtual method where the derived class will perform any required setup for its own process
 */
void Thread::OnStart()
{
        // Do any setup here in the derived version
}

/**
 * Virtual method where the derived class will perform any cleanup for its own process. This method is called
 * by the stop method
 */
void Thread::OnStop()
{
        // Do any cleanup here in the derived version
}



/**
 * Virtual method that will be executed in the thread.
 * @param arg - The argument to be processed
 */
void Thread::Execute(void* arg)
{
        // Your code goes here (in the derived version).
}



Thread::~Thread() {
	//Just make sure the thread is stopped before we leave
	if( !myTerminateApp){
		Stop();
	}
}

