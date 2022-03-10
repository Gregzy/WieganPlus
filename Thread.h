/*
 * Thread.h
 *
 *  Created on: Mar 31, 2014
 *      Author: jfgregoire
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>

class Thread {
public:
	Thread();
    int Start(void * arg);
    int Stop(void);
    int WaitForThreadStop( void){ return(pthread_join(ThreadId_, NULL));}
    int SetThreadPriority(int thePriority);
    int GetThreadPriority(void){return(myThreadPriority);}
	virtual ~Thread();
	bool IsRunning(void){return( !myTerminateApp);}

protected:
	bool myTerminateApp;
	int Run();
	static void * EntryPoint(void*);
	virtual void OnStart();
	virtual void OnStop();
	virtual void Execute(void*);
	void * Arg() const {return Arg_;}
	void Arg(void* a){Arg_ = a;}
private:
	int myThreadPriority;
	pthread_t ThreadId_;
	void * Arg_;
};

#endif /* THREAD_H_ */
