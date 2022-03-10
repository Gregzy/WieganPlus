/*
 * BitBuffer.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: jfgregoire
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <exception>
#include <memory.h>
#include "BitBuffer.h"

BitBuffer::BitBuffer( int theSizeInternalBuff){

	internalBuffer = NULL;
	sizeOfBuffer = theSizeInternalBuff;

	//(32x10 = 320 bits) Size of a int by the array size
	try {
		internalBuffer = new int[sizeOfBuffer];

	} catch (exception& e) {
		cout << e.what() << endl;
		exit(1);
	}

	ClearBuffer();
}

BitBuffer::~BitBuffer(){
	if( internalBuffer)
		delete internalBuffer;
}

void BitBuffer::ClearBuffer(void){
	for( int i=0; i < sizeOfBuffer; i++)
		internalBuffer[i] = 0;
}


/* void SetBit( int theBitIndex)
 *
 * Argument:
 * 		theBitIndex - Absolute Position of the bit in the array internal
 *
 * Return:
 * 		void
 *
 * Note: a more concise way of coding would have been
 *  A[k/32] |= 1 << (k%32);  // Set the bit at the k-th position in A[i]
 *
 * Base on the code from Shun Yan Cheung of Emory university
 * */
void BitBuffer::SetBit( int theBitIndex)
{
    int i = theBitIndex/INT_SIZE;
    int pos = theBitIndex%INT_SIZE;

    unsigned int flag = 1;   // flag = 0000.....00001

    flag = flag << pos;      // flag = 0000...010...000   (shifted k positions)

    internalBuffer[i] = internalBuffer[i] | flag;      // Set the bit at the k-th position in A[i]
}


/* bool TestBit( int theBitIndex)
 *
 * Argument:
 * 		theBitIndex - Absolute Position of the bit in the array internal
 *
 * Return:
 * 		True (1) or False (0)
 *
 * Note: a more concise way of coding would have been
 *  return ( (A[k/32] & (1 << (k%32) )) != 0 ) ;
 *
 * Base on the code from Shun Yan Cheung of Emory university
 * */
bool BitBuffer::TestBit(int theBitIndex){
    int i = theBitIndex/INT_SIZE;
    int pos = theBitIndex%INT_SIZE;

    unsigned int flag = 1;  // flag = 0000.....00001

    flag = flag << pos;     // flag = 0000...010...000   (shifted k positions)

    if ( internalBuffer[i] & flag )      // Test the bit at the k-th position in A[i]
       return 1;
    else
       return 0;
}

void BitBuffer::ClearBit( int theBitIndex)
{
    int i = theBitIndex/INT_SIZE;
    int pos = theBitIndex%INT_SIZE;

    unsigned int flag = 1;  // flag = 0000.....00001

    flag = flag << pos;     // flag = 0000...010...000   (shifted k positions)
    flag = ~flag;           // flag = 1111...101..111

    internalBuffer[i] = internalBuffer[i] & flag;     // RESET the bit at the k-th position in A[i]
}


