/*
 * wpp.h
 *
 *  Created on: Mar 19, 2014
 *      Author: jfgregoire
 */

#ifndef WPP_H_
#define WPP_H_

enum OperationMode {NONE, READ, WRITE, READ_TO_IP, WRITE_FROM_IP};
#define DEFAULT_IP_PORT "5566"
#define DEFAULT_lOG "wpp.log"

//IP transaction XML tags and attributes
// example <Wpp_trx Sequence=1 Time=2014-01-06 22h10 ID_WPP=Remote1 > <CardEvent Id_Tag="AA:12345" Format="26bit" bits=26 /> </Wpp_trx>

//Tag TRX (Transaction)
#define XML_TRX_TAG 			"Wpp_trx"
#define XML_TRX_ATTR_SEQ		"Sequence"
#define XML_TRX_ATTR_DATE_TIME	"Time"
#define XML_TRX_ATTR_ID_WPP		"ID_WPP"

//Trx Sub tag CardEvent attribute structure
#define XML_CARD_TAG_EVENT		"CardEvent"
#define XML_CARD_ATTR_ID		"ID_Tag"
#define XML_CARD_ATTR_FORMAT	"Format"
#define XML_CARD_ATTR_BITS		"bits"

//CardEvent sub tag raw binary data
#define XML_BINARY_TAG				"Binary"
#define XML_BINARY_ATTR_IN_DELAY	"Inter_Delay"
#define XML_BINARY_ATTR_PST_DELAY	"PostDelay"

//Trx Sub tag Output attribute structure
#define XML_OUTPUT_TAG_EVENT	"Output"
#define XML_OUTPUT_ATTR_ID		"Id"
#define XML_OUTPUT_ATTR_STATE	"State"
#define XML_OUTPUT_ATTR_DURAT	"Duration"
//Value for Output attribute ID
#define XML_OUTPUT_VALUE_ID_LED	"Led"
#define XML_OUTPUT_VALUE_ID_BUZ	"Buzzer"
#define XML_OUTPUT_VALUE_ID_REL	"Relay"

#define COMMAND_TAG	"Command"

//Define the GPIO pins (WiringPi lib numbering scheme, not Broadcom)
enum OUTPUT_DEF {	WIEG_GREEN = 0,
					WIEG_WHITE = 1,
					WIEG_LED_IN = 2,
					WIEG_BUZ_IN = 3,
					WIEG_RELAY = 4,
					BLUE_LED = 5,
					GREEN_LED = 6,
					PIEZO = 7};


long GetMilliseconds( struct timespec theTime);

#endif /* WPP_H_ */
