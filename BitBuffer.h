/*
 * BitBuffer.h
 *
 *  Created on: Apr 2, 2014
 *      Author: jfgregoire
 */

#ifndef BITBUFFER_H_
#define BITBUFFER_H_

using namespace std;

#define INT_SIZE 32;


class BitBuffer{
public:
	BitBuffer(int theSizeInternalBuff = 10);
	int *GetInternalBuffer(void){return(internalBuffer);}
	void SetBit( int theBitIndex );
	void ClearBit( int theBitIndex);
	bool TestBit( int theBitIndex);
	void ClearBuffer( void);
	~BitBuffer();
protected:
	int *internalBuffer;
	int sizeOfBuffer;
};



#endif /* BITBUFFER_H_ */
