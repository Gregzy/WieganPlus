/*
 * Card.cpp
 *
 *  Created on: Mar 2, 2014
 *      Author: jfgregoire
 */

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cstdio>
#include "WiegandReader.h"
#include "BitBuffer.h"
#include "Card.h"

Card::Card( int theInternalBuffSize) : BitBuffer(theInternalBuffSize)
{
	InitCard();
}

Card::Card(int *theRawData, CARD_TYPE theCardType, int theInternalBuffSize)  : BitBuffer(theInternalBuffSize)
{
	InitCard();

	memcpy(internalBuffer, theRawData, theInternalBuffSize);
	cardType = theCardType;

	ExtractCard();
}

/**
 * Constructor using a string representation of a card
 * @param theCardSiteTag - The string representation ex aa:56021
 * @param theCardType - The Card type ex: WIEGAND26
 */
Card::Card( std::string theCardSiteTag, CARD_TYPE theCardType){

	InitCard();
	cardType = theCardType;

	std::vector<std::string>elems;
	std::stringstream ss(theCardSiteTag);
	std::string item;

	while(std::getline(ss, item, ':')){
		elems.push_back(item);
	}

	if( elems.size() == 2){

		siteCode = strtoul(elems[0].c_str(), NULL, 16); 	//Revert to good old C since the compiler
															//does not support STD for this (maybe we should use BOOST lib)

		std::stringstream sconvert2;
		sconvert2 << elems[1];
		sconvert2 >> tagID;

	}else{
		Cardexception myException;
		throw myException;
	}

	// todo Throw an an excpetion if the site or the tag is exceding possible values
	EncodeCard();

}

Card::Card(char theSiteCode, int theTagId,  CARD_TYPE theCardType){

	InitCard();
	cardType = theCardType;

	siteCode = theSiteCode;
	tagID = theTagId;

	EncodeCard();
}

Card::~Card() {
	// TODO Auto-generated destructor stub
}

void Card::InitCard(void)
{
	siteCode = 0;
	tagID = 0;
	evenParity = false, oddParity = false;
}

/**
 * Encode a card from the siteCode and tagID member variable
 */
void Card::EncodeCard(void){

	//Site code processing
	unsigned char flag = 0x80;  //Set the binary mask to 10000000
	for( int i = 1; i < 9; i++){
		if(flag & siteCode )
			SetBit(i);

		flag >>= 1;				//Move it to the right to check every bit to 01000000
	}

	//Tag ID processing
	int myInt16Flag = 0x8000; //Set the binary mask to 1000000000000000
	for( int j = 9; j < 26; j++){
		if(myInt16Flag & tagID )
			SetBit(j);

		myInt16Flag >>= 1;		//SAme operation a Site code
	}


	//Calculate leading Even parity. For example, if the result of the first 12 (starting at index 1) data cumulative XOR is 1,
	//set the leading parity (index 0) to 1 so the XOR of the first 13 (from index 0 to 12) will be 0 (even)
	if( CalculateParity(1, 12)){
		this->SetBit(0);   	//We must be even (0), so set the first bit to 1
	}
	else{
		this->ClearBit(0); 	//Already even, we don't actually need to do anything but clear the bit just to be consistent with the code
	}

	evenParity = CalculateParity(0, 13);

	//Calculate the Odd trailing parity. For example, if the result of the last 12 bit (starting from bit 25 in reverse order) data
	//cumulative XOR is 0, set the trailing parity (index 25) to 1 so the XOR of the last 13 bit (index 13 to 25) is 1 (Odd).
	if( CalculateParity(13, 12)){
		this->ClearBit(25);
	}else{
		this->SetBit(25);
	}

	oddParity = CalculateParity(13, 13);

}

void Card::ExtractCard()
{

	for( int i=0; i < (int)cardType;i++)
	{

	    if( i > 0 && i <= 8)
	    {
	    	if( TestBit( i) )
	    	{
	    		siteCode <<= 1;
	    		siteCode = siteCode | 1;
	    	}
	    	else
	    		siteCode <<= 1;

	    }else if( i >= 9 && i <= 24)
	    {

	        if( TestBit(i))
	        {
	        	tagID <<= 1;
	        	tagID = tagID | 1;
	        }
	        else
	        	tagID <<= 1;

	    }
	}

	//Calculate even (leading) and odd (trailing) parity.
	evenParity = CalculateParity(0, 13);
	oddParity = CalculateParity(13, 13);

}

/**
 * Calculate the Wiegand parity. the leading parity bit (even) is linked to the first 12 data bits.  If the 12 data bits result in an odd
 * number, the parity bit is set to one to make the 13-bit total come out even.  The final 13 bits are similarly set to an odd total.
 *
 * @param theStartIndex - First bit to start the calculation (ex.: start at 0 to evaluate and at 1 to create a parity)
 * @param theNbrOfBit - How many bit to do a chain XOR
 * @return Binary 1 or 0
 */
bool Card::CalculateParity( unsigned int theStartIndex, unsigned int theNbrOfBit){

	//bool result = TestBit( theStartIndex);
	bool result = 0;

	for (unsigned int i = theStartIndex; i < (theStartIndex + theNbrOfBit); i++) {
		result ^= TestBit( i);
	}

	return(result);
}




std::string Card::GetTagString()
{
	char myBuffer[10];;

	sprintf(myBuffer, "%2X:%05d", siteCode, tagID);
	std::string myString(myBuffer);

	return( myString);
}


std::string Card::GetBitBufferString(){
	string myReturnVal;
	unsigned int mySizeBuffer = (unsigned int) cardType; //The card type will tell us the size of the binary buffer ex.: Wiegand 26(bit)

    for( unsigned int i = 0; i < mySizeBuffer;i++)
    {
    	myReturnVal += (TestBit(i) ? "1":"0");
    }

    return( myReturnVal);


}


