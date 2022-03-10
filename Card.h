/*
 * Card.h
 *
 *  Created on: Mar 2, 2014
 *      Author: jfgregoire
 */

#ifndef CARD_H_
#define CARD_H_

#include "WiegandReader.h"

using namespace std;

#define RAW_ARRAY_LENGTH 		2
#define MAX_WIEGAND_TAG_VALUE 	65535
#define MAX_WIEGAND_SITE_VALUE 	255

enum CARD_TYPE {WIEGAND26= 26, WIEGAND32 = 32, WIEGAND37 = 37};

class Card : public BitBuffer {
public:

	Card(int theInternalBuffSize = 10);
	Card(int *theRawData, CARD_TYPE theCardType, int theInternalBuffSize = 10);
	Card(std::string theCardSiteTag, CARD_TYPE theCardType);
	Card(char theSiteCode, int theTagId,  CARD_TYPE theCardType);
	virtual ~Card();


	inline bool operator==(const Card& rhs){ return( (siteCode==rhs.siteCode ) && (tagID==rhs.tagID) ); }
	inline bool operator!=(const Card& rhs){return !operator==(rhs);}
	inline bool operator< (const Card& rhs){  return(  (siteCode < rhs.siteCode) || ((siteCode <= rhs.siteCode) && (tagID < rhs.tagID)) );  }
	inline bool operator> (const Card& rhs){  return(  (siteCode > rhs.siteCode) || ((siteCode >= rhs.siteCode) && (tagID > rhs.tagID)) ); }
	inline bool operator<=(const Card& rhs){return !operator> (rhs);}
	inline bool operator>=(const Card& rhs){return !operator< (rhs);}
	char GetSiteCode(){return siteCode;}
	int GettagID(){return tagID;}
	bool IsParityOk(void){return(oddParity && !evenParity ? true:false);}
	string GetTagString();
	std::string GetBitBufferString(void);
	CARD_TYPE GetCardType(void){return(cardType);}
	int GetNumOfBits(void) {return( (int)cardType  );}
	bool GetBit(int theIndex){ return(TestBit(theIndex));}
private:
	char siteCode;
	int tagID;
	CARD_TYPE cardType;
	bool evenParity; 	//Leading parity
	bool oddParity; 	//trailing parity

	void InitCard(void);
	void ExtractCard();
	void EncodeCard(void);
	bool CalculateParity( unsigned int theStartIndex, unsigned int theEndIndex);
};

class Cardexception: public exception
{
  virtual const char* what() const throw()
  {
    return "Card exception";
  }
};


#endif /* CARD_H_ */
